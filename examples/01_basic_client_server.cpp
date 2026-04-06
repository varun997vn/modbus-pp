/// @file 01_basic_client_server.cpp
/// @brief Basic Client/Server communication over loopback transport.
///
/// Demonstrates the fundamental modbus_pp workflow: a Server handles
/// read/write requests while a Client reads and writes holding registers.
/// This is the simplest possible end-to-end example -- standard Modbus
/// operations over an in-process loopback transport.
///
/// Modbus disadvantage addressed: raw byte manipulation and error-prone
/// manual framing are replaced by a type-safe Client/Server API with
/// Result<T> error handling.

#include "modbus_pp/modbus_pp.hpp"

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;  // avoid POSIX register_t conflict

int main() {
    std::cout << "=== modbus_pp Example 01: Basic Client/Server ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Create loopback transport pair
    // -----------------------------------------------------------------------
    auto [client_transport, server_transport] = LoopbackTransport::create_pair();
    client_transport->connect();
    server_transport->connect();

    std::cout << "[setup] Loopback transport pair created and connected.\n\n";

    // Shared transport pointers (Server and Client take shared_ptr)
    std::shared_ptr<Transport> srv_tp = std::move(server_transport);
    std::shared_ptr<Transport> cli_tp = std::move(client_transport);

    // -----------------------------------------------------------------------
    // 2. Set up the server with simulated register storage
    // -----------------------------------------------------------------------
    // Simulated register file: 100 registers initialized with sensor-like data
    std::vector<reg_t> register_file(100, 0);
    std::mutex reg_mutex;

    // Pre-populate some "sensor" data
    // Registers 0-1: temperature as float (25.5 C) encoded in ABCD order
    auto temp_regs = TypeCodec::encode_float<ByteOrder::ABCD>(25.5f);
    register_file[0] = temp_regs[0];
    register_file[1] = temp_regs[1];
    // Register 2: status word
    register_file[2] = 0x0001;  // sensor online
    // Register 3: firmware version
    register_file[3] = 0x0102;  // v1.2

    ServerConfig srv_cfg{srv_tp, 1, true};
    Server server(std::move(srv_cfg));

    // Handler: read holding registers
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

    // Handler: write multiple registers
    server.on_write_multiple_registers(
        [&register_file, &reg_mutex](address_t addr, span_t<const reg_t> values)
            -> Result<void> {
            std::lock_guard<std::mutex> lock(reg_mutex);
            if (addr + values.size() > register_file.size()) {
                return ErrorCode::IllegalDataAddress;
            }
            for (std::size_t i = 0; i < values.size(); ++i) {
                register_file[addr + i] = values[i];
            }
            return Result<void>{};
        });

    // -----------------------------------------------------------------------
    // 3. Run server in a background thread
    // -----------------------------------------------------------------------
    std::atomic<bool> server_running{true};
    std::thread server_thread([&server, &server_running]() {
        while (server_running.load()) {
            server.process_one();
        }
    });

    std::cout << "[server] Running on unit ID 1 with 100 holding registers.\n\n";

    // -----------------------------------------------------------------------
    // 4. Client operations
    // -----------------------------------------------------------------------
    ClientConfig cli_cfg{cli_tp, nullptr, PipelineConfig{}, true,
                         std::chrono::milliseconds{1000}};
    Client client(std::move(cli_cfg));

    // --- Read temperature (2 registers starting at address 0) ---
    std::cout << "[client] Reading temperature (registers 0-1)...\n";
    auto read_result = client.read_holding_registers(1, 0, 2);
    if (read_result) {
        auto& regs = read_result.value();
        float temp = TypeCodec::decode_float<ByteOrder::ABCD>(regs.data());
        std::cout << "  Raw registers: [0x" << std::hex << std::setfill('0')
                  << std::setw(4) << regs[0] << ", 0x"
                  << std::setw(4) << regs[1] << "]\n" << std::dec;
        std::cout << "  Decoded temperature: " << temp << " C\n\n";
    } else {
        std::cerr << "  ERROR: " << read_result.error().message() << "\n\n";
    }

    // --- Read status register ---
    std::cout << "[client] Reading status (register 2)...\n";
    auto status_result = client.read_holding_registers(1, 2, 1);
    if (status_result) {
        auto& regs = status_result.value();
        std::cout << "  Status: 0x" << std::hex << std::setfill('0')
                  << std::setw(4) << regs[0] << std::dec
                  << (regs[0] == 1 ? " (sensor online)" : " (unknown)") << "\n\n";
    } else {
        std::cerr << "  ERROR: " << status_result.error().message() << "\n\n";
    }

    // --- Write a new temperature setpoint (registers 10-11) ---
    std::cout << "[client] Writing temperature setpoint = 72.0 to registers 10-11...\n";
    auto setpoint_regs = TypeCodec::encode_float<ByteOrder::ABCD>(72.0f);
    std::vector<reg_t> setpoint_vec(setpoint_regs.begin(), setpoint_regs.end());
    auto write_result = client.write_multiple_registers(1, 10, setpoint_vec);
    if (write_result) {
        std::cout << "  Write succeeded.\n\n";
    } else {
        std::cerr << "  ERROR: " << write_result.error().message() << "\n\n";
    }

    // --- Read back the setpoint to confirm ---
    std::cout << "[client] Reading back setpoint (registers 10-11)...\n";
    auto readback = client.read_holding_registers(1, 10, 2);
    if (readback) {
        auto& regs = readback.value();
        float setpoint = TypeCodec::decode_float<ByteOrder::ABCD>(regs.data());
        std::cout << "  Readback setpoint: " << setpoint << " (expected 72.0)\n\n";
    } else {
        std::cerr << "  ERROR: " << readback.error().message() << "\n\n";
    }

    // --- Write a single register ---
    std::cout << "[client] Writing single register: address 50 = 0xBEEF...\n";
    auto single_wr = client.write_single_register(1, 50, 0xBEEF);
    if (single_wr) {
        std::cout << "  Single write succeeded.\n\n";
    } else {
        std::cerr << "  ERROR: " << single_wr.error().message() << "\n\n";
    }

    // --- Error case: read beyond register file ---
    std::cout << "[client] Attempting to read beyond register file (addr=95, count=10)...\n";
    auto err_result = client.read_holding_registers(1, 95, 10);
    if (!err_result) {
        std::cout << "  Expected error received: " << err_result.error().message() << "\n\n";
    } else {
        std::cout << "  Unexpected success (register file may be larger).\n\n";
    }

    // -----------------------------------------------------------------------
    // 5. Shutdown
    // -----------------------------------------------------------------------
    server_running.store(false);
    server_thread.join();

    std::cout << "[done] Basic client/server example complete.\n";
    return 0;
}
