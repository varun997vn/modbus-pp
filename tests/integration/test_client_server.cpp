#include <gtest/gtest.h>

#include <modbus_pp/client/client.hpp>
#include <modbus_pp/client/server.hpp>
#include <modbus_pp/transport/loopback_transport.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using namespace modbus_pp;

// Avoid POSIX register_t conflict.
using reg_t = modbus_pp::register_t;

// ---------------------------------------------------------------------------
// Helper: create connected loopback pair, wrap in shared_ptrs
// ---------------------------------------------------------------------------
struct LoopbackPair {
    std::shared_ptr<Transport> client_transport;
    std::shared_ptr<Transport> server_transport;
};

static LoopbackPair make_connected_pair() {
    auto [ct, st] = LoopbackTransport::create_pair();
    ct->connect();
    st->connect();
    return {std::shared_ptr<Transport>(ct.release()),
            std::shared_ptr<Transport>(st.release())};
}

// ---------------------------------------------------------------------------
// Helper RAII guard to run the server loop in a background thread
// ---------------------------------------------------------------------------
class ServerRunner {
public:
    explicit ServerRunner(Server& server)
        : server_(server), running_{true},
          thread_([this] {
              while (running_.load(std::memory_order_relaxed)) {
                  server_.process_one();
              }
          }) {}

    ~ServerRunner() {
        running_.store(false, std::memory_order_relaxed);
        thread_.join();
    }

    ServerRunner(const ServerRunner&) = delete;
    ServerRunner& operator=(const ServerRunner&) = delete;

private:
    Server& server_;
    std::atomic<bool> running_;
    std::thread thread_;
};

// ===========================================================================
// 1. Read holding registers — server returns known values, client verifies
// ===========================================================================
TEST(ClientServer, ReadHoldingRegisters) {
    auto pair = make_connected_pair();

    Server server(ServerConfig{pair.server_transport, 1, false});
    server.on_read_holding_registers(
        [](address_t addr, quantity_t count) -> Result<std::vector<reg_t>> {
            std::vector<reg_t> regs(count);
            for (quantity_t i = 0; i < count; ++i) {
                regs[i] = static_cast<reg_t>(addr + i);
            }
            return regs;
        });

    ServerRunner runner(server);

    ClientConfig cfg{pair.client_transport, nullptr, {}, false,
                     std::chrono::milliseconds{500}};
    Client client(std::move(cfg));

    auto result = client.read_holding_registers(1, 100, 5);
    ASSERT_TRUE(result);

    auto& regs = result.value();
    ASSERT_EQ(regs.size(), 5u);
    for (quantity_t i = 0; i < 5; ++i) {
        EXPECT_EQ(regs[i], static_cast<reg_t>(100 + i))
            << "mismatch at index " << i;
    }
}

// ===========================================================================
// 2. Write single register — verify server handler receives correct addr/val
// ===========================================================================
TEST(ClientServer, WriteSingleRegister) {
    auto pair = make_connected_pair();

    std::mutex mu;
    address_t captured_addr = 0;
    reg_t captured_value = 0;

    Server server(ServerConfig{pair.server_transport, 1, false});
    // write_single_register (FC 0x06) is dispatched through the write handler
    // as a single-element span.
    server.on_write_multiple_registers(
        [&](address_t addr,
            span_t<const reg_t> values) -> Result<void> {
            std::lock_guard<std::mutex> lock(mu);
            captured_addr = addr;
            if (values.size() >= 1) {
                captured_value = values[0];
            }
            return Result<void>{};
        });

    ServerRunner runner(server);

    ClientConfig cfg{pair.client_transport, nullptr, {}, false,
                     std::chrono::milliseconds{500}};
    Client client(std::move(cfg));

    auto result = client.write_single_register(1, 200, 0xCAFE);
    ASSERT_TRUE(result);

    // Allow a moment for the server thread to process the response.
    std::this_thread::sleep_for(std::chrono::milliseconds{50});

    std::lock_guard<std::mutex> lock(mu);
    EXPECT_EQ(captured_addr, 200);
    EXPECT_EQ(captured_value, 0xCAFE);
}

// ===========================================================================
// 3. Write multiple registers — server verifies all received values
// ===========================================================================
TEST(ClientServer, WriteMultipleRegisters) {
    auto pair = make_connected_pair();

    std::mutex mu;
    address_t captured_addr = 0;
    std::vector<reg_t> captured_values;

    Server server(ServerConfig{pair.server_transport, 1, false});
    server.on_write_multiple_registers(
        [&](address_t addr,
            span_t<const reg_t> values) -> Result<void> {
            std::lock_guard<std::mutex> lock(mu);
            captured_addr = addr;
            captured_values.assign(values.begin(), values.end());
            return Result<void>{};
        });

    ServerRunner runner(server);

    ClientConfig cfg{pair.client_transport, nullptr, {}, false,
                     std::chrono::milliseconds{500}};
    Client client(std::move(cfg));

    std::vector<reg_t> to_write = {0x0001, 0x0002, 0x0003, 0x0004};
    auto result = client.write_multiple_registers(
        1, 300, span_t<const reg_t>{to_write});
    ASSERT_TRUE(result);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});

    std::lock_guard<std::mutex> lock(mu);
    EXPECT_EQ(captured_addr, 300);
    ASSERT_EQ(captured_values.size(), 4u);
    EXPECT_EQ(captured_values[0], 0x0001);
    EXPECT_EQ(captured_values[1], 0x0002);
    EXPECT_EQ(captured_values[2], 0x0003);
    EXPECT_EQ(captured_values[3], 0x0004);
}

// ===========================================================================
// 4. Read error response — server returns IllegalDataAddress
// ===========================================================================
TEST(ClientServer, ReadErrorResponse) {
    auto pair = make_connected_pair();

    Server server(ServerConfig{pair.server_transport, 1, false});
    server.on_read_holding_registers(
        [](address_t /*addr*/,
           quantity_t /*count*/) -> Result<std::vector<reg_t>> {
            return ErrorCode::IllegalDataAddress;
        });

    ServerRunner runner(server);

    ClientConfig cfg{pair.client_transport, nullptr, {}, false,
                     std::chrono::milliseconds{500}};
    Client client(std::move(cfg));

    auto result = client.read_holding_registers(1, 9999, 1);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::IllegalDataAddress));
}

// ===========================================================================
// 5. Multiple sequential reads — 5 back-to-back reads all succeed
// ===========================================================================
TEST(ClientServer, MultipleSequentialReads) {
    auto pair = make_connected_pair();

    Server server(ServerConfig{pair.server_transport, 1, false});
    server.on_read_holding_registers(
        [](address_t addr, quantity_t count) -> Result<std::vector<reg_t>> {
            std::vector<reg_t> regs(count);
            for (quantity_t i = 0; i < count; ++i) {
                regs[i] = static_cast<reg_t>(addr * 10 + i);
            }
            return regs;
        });

    ServerRunner runner(server);

    ClientConfig cfg{pair.client_transport, nullptr, {}, false,
                     std::chrono::milliseconds{500}};
    Client client(std::move(cfg));

    for (int round = 0; round < 5; ++round) {
        auto addr = static_cast<address_t>(round * 10);
        auto result = client.read_holding_registers(1, addr, 3);
        ASSERT_TRUE(result) << "failed on round " << round;

        auto& regs = result.value();
        ASSERT_EQ(regs.size(), 3u);
        for (quantity_t i = 0; i < 3; ++i) {
            EXPECT_EQ(regs[i], static_cast<reg_t>(addr * 10 + i))
                << "mismatch round=" << round << " i=" << i;
        }
    }
}

// ===========================================================================
// 6. Broadcast unit id — server (unit 1) handles request sent to unit 0
// ===========================================================================
TEST(ClientServer, BroadcastUnitId) {
    auto pair = make_connected_pair();

    Server server(ServerConfig{pair.server_transport, 1, false});
    server.on_read_holding_registers(
        [](address_t addr, quantity_t count) -> Result<std::vector<reg_t>> {
            std::vector<reg_t> regs(count);
            for (quantity_t i = 0; i < count; ++i) {
                regs[i] = static_cast<reg_t>(addr + i + 1);
            }
            return regs;
        });

    ServerRunner runner(server);

    ClientConfig cfg{pair.client_transport, nullptr, {}, false,
                     std::chrono::milliseconds{500}};
    Client client(std::move(cfg));

    // Unit 0 is the Modbus broadcast address.
    auto result = client.read_holding_registers(0, 50, 3);
    ASSERT_TRUE(result);

    auto& regs = result.value();
    ASSERT_EQ(regs.size(), 3u);
    EXPECT_EQ(regs[0], 51);
    EXPECT_EQ(regs[1], 52);
    EXPECT_EQ(regs[2], 53);
}
