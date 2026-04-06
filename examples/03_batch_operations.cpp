/// @file 03_batch_operations.cpp
/// @brief BatchRequest optimization and batch reads over loopback.
///
/// Demonstrates how BatchRequest aggregates multiple heterogeneous register
/// reads into fewer operations by merging contiguous address ranges.
///
/// Modbus disadvantage addressed: standard Modbus requires a separate
/// request/response round-trip for every register range. For 10 ranges,
/// that is 10 round-trips. BatchRequest merges contiguous ranges and
/// sends them in a single extended frame, cutting latency dramatically.

#include "modbus_pp/modbus_pp.hpp"

#include <atomic>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

int main() {
    std::cout << "=== modbus_pp Example 03: Batch Operations ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Build a batch request with 10 individual register ranges
    // -----------------------------------------------------------------------
    std::cout << "--- Building Batch Request (10 individual ranges) ---\n";
    BatchRequest batch;

    // Simulate a real-world scenario: 10 sensors scattered across the register map,
    // some contiguous, some with gaps.
    struct SensorDef {
        const char* name;
        address_t   addr;
        quantity_t  count;
        RegisterType type;
        ByteOrder   order;
    };

    SensorDef sensors[] = {
        {"Temperature A",  0x0000, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Temperature B",  0x0002, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Pressure",       0x0004, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Flow rate",      0x0006, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Vibration X",    0x0008, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Vibration Y",    0x000A, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Motor RPM",      0x0020, 2, RegisterType::UInt32,  ByteOrder::ABCD},
        {"Motor current",  0x0022, 2, RegisterType::Float32, ByteOrder::ABCD},
        {"Valve position", 0x0030, 1, RegisterType::UInt16,  ByteOrder::ABCD},
        {"Status word",    0x0031, 1, RegisterType::UInt16,  ByteOrder::ABCD},
    };

    for (const auto& s : sensors) {
        batch.add(s.addr, s.count, s.type, s.order);
        std::cout << "  add(" << std::setw(14) << std::left << s.name
                  << "  addr=0x" << std::hex << std::setfill('0')
                  << std::setw(4) << s.addr << std::dec << std::setfill(' ')
                  << "  count=" << s.count << ")\n";
    }

    std::cout << "\n  Before optimization: " << batch.size() << " items, "
              << batch.total_registers() << " total registers\n\n";

    // -----------------------------------------------------------------------
    // 2. Optimize the batch
    // -----------------------------------------------------------------------
    std::cout << "--- After Optimization ---\n";
    auto optimized = batch.optimized();

    for (const auto& item : optimized.items()) {
        std::cout << "  range: addr=0x" << std::hex << std::setfill('0')
                  << std::setw(4) << item.start_address
                  << "  count=" << std::dec << std::setfill(' ') << item.count
                  << "  (covers registers 0x" << std::hex << std::setfill('0')
                  << std::setw(4) << item.start_address << " - 0x"
                  << std::setw(4) << (item.start_address + item.count - 1)
                  << ")" << std::dec << std::setfill(' ') << "\n";
    }

    std::cout << "\n  After optimization:  " << optimized.size() << " items, "
              << optimized.total_registers() << " total registers\n";
    std::cout << "  Reduction: " << batch.size() << " requests -> "
              << optimized.size() << " requests ("
              << (100 - 100 * optimized.size() / batch.size()) << "% fewer round-trips)\n\n";

    // -----------------------------------------------------------------------
    // 3. Serialize / deserialize the batch (for transport)
    // -----------------------------------------------------------------------
    std::cout << "--- Serialization ---\n";
    auto serialized = batch.serialize();
    std::cout << "  Serialized batch to " << serialized.size() << " bytes\n";

    auto deserialized = BatchRequest::deserialize(serialized);
    if (deserialized) {
        std::cout << "  Deserialized back: " << deserialized.value().size()
                  << " items (round-trip OK)\n\n";
    } else {
        std::cerr << "  Deserialization failed: "
                  << deserialized.error().message() << "\n\n";
    }

    // -----------------------------------------------------------------------
    // 4. Execute a batch read over loopback
    // -----------------------------------------------------------------------
    std::cout << "--- Batch Read Over Loopback ---\n";

    // Set up loopback transport
    auto [client_transport, server_transport] = LoopbackTransport::create_pair();
    client_transport->connect();
    server_transport->connect();

    std::shared_ptr<Transport> srv_tp = std::move(server_transport);
    std::shared_ptr<Transport> cli_tp = std::move(client_transport);

    // Server with a register file containing known values
    std::vector<reg_t> register_file(0x0040, 0);
    std::mutex reg_mutex;

    // Fill with sensor values
    auto encode_at = [&](address_t addr, float val) {
        auto regs = TypeCodec::encode_float<ByteOrder::ABCD>(val);
        register_file[addr]     = regs[0];
        register_file[addr + 1] = regs[1];
    };

    encode_at(0x0000, 85.3f);    // Temperature A
    encode_at(0x0002, 91.7f);    // Temperature B
    encode_at(0x0004, 1015.2f);  // Pressure
    encode_at(0x0006, 42.5f);    // Flow rate
    encode_at(0x0008, 0.012f);   // Vibration X
    encode_at(0x000A, 0.008f);   // Vibration Y
    // Motor RPM as uint32 = 3600
    auto rpm_regs = TypeCodec::encode_uint32<ByteOrder::ABCD>(3600u);
    register_file[0x0020] = rpm_regs[0];
    register_file[0x0021] = rpm_regs[1];
    encode_at(0x0022, 15.7f);    // Motor current
    register_file[0x0030] = 75;  // Valve position (75%)
    register_file[0x0031] = 0x00FF;  // Status word

    ServerConfig srv_cfg{srv_tp, 1, true};
    Server server(std::move(srv_cfg));

    server.on_read_holding_registers(
        [&register_file, &reg_mutex](address_t addr, quantity_t count)
            -> Result<std::vector<reg_t>> {
            std::lock_guard<std::mutex> lock(reg_mutex);
            if (addr + count > register_file.size()) {
                return ErrorCode::IllegalDataAddress;
            }
            return std::vector<reg_t>(
                register_file.begin() + addr,
                register_file.begin() + addr + count);
        });

    std::atomic<bool> server_running{true};
    std::thread server_thread([&server, &server_running]() {
        while (server_running.load()) {
            server.process_one();
        }
    });

    // Client batch read
    ClientConfig cli_cfg{cli_tp, nullptr, PipelineConfig{}, true,
                         std::chrono::milliseconds{1000}};
    Client client(std::move(cli_cfg));

    auto batch_result = client.batch_read(1, batch);
    if (batch_result) {
        auto& response = batch_result.value();
        std::cout << "  Batch read returned " << response.size() << " results\n";
        std::cout << "  All succeeded: " << (response.all_succeeded() ? "yes" : "no") << "\n\n";

        // Decode and print each result
        for (std::size_t i = 0; i < response.results().size() && i < 10; ++i) {
            const auto& item = response.results()[i];
            std::cout << "  [0x" << std::hex << std::setfill('0')
                      << std::setw(4) << item.address << std::dec
                      << std::setfill(' ') << "] ";

            if (item.registers.size() == 2 && i < 8) {
                if (i == 6) {
                    // Motor RPM as uint32
                    auto val = TypeCodec::decode_uint32<ByteOrder::ABCD>(item.registers.data());
                    std::cout << std::setw(14) << std::left << sensors[i].name
                              << " = " << val << "\n";
                } else {
                    float val = TypeCodec::decode_float<ByteOrder::ABCD>(item.registers.data());
                    std::cout << std::setw(14) << std::left << sensors[i].name
                              << " = " << val << "\n";
                }
            } else if (item.registers.size() == 1) {
                std::cout << std::setw(14) << std::left << sensors[i].name
                          << " = " << item.registers[0] << "\n";
            }
        }
    } else {
        std::cerr << "  Batch read failed: " << batch_result.error().message() << "\n";
    }

    // Shutdown
    server_running.store(false);
    server_thread.join();

    std::cout << "\n[done] Batch operations example complete.\n";
    return 0;
}
