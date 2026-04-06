/// @file 04_pipeline_throughput.cpp
/// @brief Pipelining vs sequential request throughput comparison.
///
/// Sends 16 requests sequentially (wait for each response before sending
/// the next), then sends 16 requests pipelined (all in flight concurrently).
/// Measures wall-clock time for each approach and prints the speedup ratio.
///
/// Modbus disadvantage addressed: standard Modbus is strictly sequential --
/// send one request, wait for the response, then send the next. Over a
/// high-latency link (e.g., satellite, cellular), this destroys throughput.
/// Pipelining overlaps requests using correlation IDs.

#include "modbus_pp/modbus_pp.hpp"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

static constexpr int REQUEST_COUNT = 16;

int main() {
    std::cout << "=== modbus_pp Example 04: Pipeline Throughput ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Set up loopback with simulated latency
    // -----------------------------------------------------------------------
    auto [client_transport, server_transport] = LoopbackTransport::create_pair();

    // Add simulated latency to model a realistic network
    client_transport->set_simulated_latency(std::chrono::microseconds{500});
    server_transport->set_simulated_latency(std::chrono::microseconds{500});

    client_transport->connect();
    server_transport->connect();

    std::shared_ptr<Transport> srv_tp = std::move(server_transport);
    std::shared_ptr<Transport> cli_tp = std::move(client_transport);

    // Server: simple register file
    std::vector<reg_t> register_file(256, 0);
    std::mutex reg_mutex;
    for (std::size_t i = 0; i < register_file.size(); ++i) {
        register_file[i] = static_cast<reg_t>(i * 10);
    }

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

    ClientConfig cli_cfg{cli_tp, nullptr,
                         PipelineConfig{REQUEST_COUNT, std::chrono::milliseconds{2000}},
                         true, std::chrono::milliseconds{2000}};
    Client client(std::move(cli_cfg));

    std::cout << "[setup] Loopback with 500us simulated latency per direction.\n";
    std::cout << "[setup] " << REQUEST_COUNT << " read requests, each reading 4 registers.\n\n";

    // -----------------------------------------------------------------------
    // 2. Sequential mode: send one, wait, send next
    // -----------------------------------------------------------------------
    std::cout << "--- Sequential Mode ---\n";
    auto seq_start = std::chrono::steady_clock::now();

    int seq_success = 0;
    for (int i = 0; i < REQUEST_COUNT; ++i) {
        auto addr = static_cast<address_t>(i * 4);
        auto result = client.read_holding_registers(1, addr, 4);
        if (result) {
            ++seq_success;
        }
    }

    auto seq_end = std::chrono::steady_clock::now();
    auto seq_us = std::chrono::duration_cast<std::chrono::microseconds>(seq_end - seq_start).count();

    std::cout << "  Completed: " << seq_success << "/" << REQUEST_COUNT << " requests\n";
    std::cout << "  Wall time: " << seq_us << " us\n";
    std::cout << "  Avg per request: " << seq_us / REQUEST_COUNT << " us\n\n";

    // -----------------------------------------------------------------------
    // 3. Pipelined mode: submit all, then collect
    // -----------------------------------------------------------------------
    std::cout << "--- Pipelined Mode ---\n";
    auto& pipeline = client.pipeline();

    auto pipe_start = std::chrono::steady_clock::now();

    std::atomic<int> pipe_success{0};
    std::atomic<int> pipe_complete{0};

    // Build PDUs for each read request
    for (int i = 0; i < REQUEST_COUNT; ++i) {
        auto addr = static_cast<address_t>(i * 4);

        // Build a standard ReadHoldingRegisters PDU:
        // FC=0x03, [start_addr:2][quantity:2]
        std::vector<byte_t> data;
        data.push_back(static_cast<byte_t>(addr >> 8));
        data.push_back(static_cast<byte_t>(addr & 0xFF));
        data.push_back(0x00);
        data.push_back(0x04);  // count = 4

        auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters, std::move(data));

        auto submit_result = pipeline.submit(
            std::move(pdu),
            [&pipe_success, &pipe_complete, i](Result<PDU> result) {
                if (result) {
                    pipe_success.fetch_add(1);
                }
                pipe_complete.fetch_add(1);
            },
            std::chrono::milliseconds{2000});

        if (!submit_result) {
            std::cerr << "  Submit failed for request " << i << ": "
                      << submit_result.error().message() << "\n";
        }
    }

    // Poll until all responses arrive
    while (pipe_complete.load() < REQUEST_COUNT) {
        pipeline.poll();
    }

    auto pipe_end = std::chrono::steady_clock::now();
    auto pipe_us = std::chrono::duration_cast<std::chrono::microseconds>(pipe_end - pipe_start).count();

    std::cout << "  Completed: " << pipe_success.load() << "/" << REQUEST_COUNT << " requests\n";
    std::cout << "  Wall time: " << pipe_us << " us\n";
    std::cout << "  Avg per request: " << pipe_us / REQUEST_COUNT << " us\n\n";

    // -----------------------------------------------------------------------
    // 4. Compare results
    // -----------------------------------------------------------------------
    std::cout << "--- Comparison ---\n";
    double speedup = (seq_us > 0 && pipe_us > 0)
        ? static_cast<double>(seq_us) / static_cast<double>(pipe_us)
        : 0.0;

    std::cout << "  Sequential:  " << std::setw(8) << seq_us << " us\n";
    std::cout << "  Pipelined:   " << std::setw(8) << pipe_us << " us\n";
    std::cout << "  Speedup:     " << std::fixed << std::setprecision(2)
              << speedup << "x\n\n";

    if (speedup > 1.0) {
        std::cout << "  Pipelining is " << std::fixed << std::setprecision(1)
                  << speedup << "x faster for " << REQUEST_COUNT
                  << " concurrent requests.\n";
    } else {
        std::cout << "  Note: loopback latency is very low, so pipelining\n"
                  << "  overhead may dominate. Over a real network (TCP, serial),\n"
                  << "  pipelining typically yields 5-15x speedup.\n";
    }

    std::cout << "\n  Pipeline stats:\n";
    std::cout << "    Completed:      " << pipeline.completed_count() << "\n";
    std::cout << "    Avg latency:    " << pipeline.average_latency().count() << " us\n";
    std::cout << "    In flight now:  " << pipeline.in_flight_count() << "\n";

    // -----------------------------------------------------------------------
    // 5. Also demonstrate submit_sync for simple one-off pipelined reads
    // -----------------------------------------------------------------------
    std::cout << "\n--- Pipeline submit_sync (single request) ---\n";

    std::vector<byte_t> sync_data;
    sync_data.push_back(0x00);  // addr = 0x0000
    sync_data.push_back(0x00);
    sync_data.push_back(0x00);  // count = 2
    sync_data.push_back(0x02);

    auto sync_pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters, std::move(sync_data));
    auto sync_result = pipeline.submit_sync(std::move(sync_pdu), std::chrono::milliseconds{2000});
    if (sync_result) {
        std::cout << "  submit_sync succeeded, response payload: "
                  << sync_result.value().payload().size() << " bytes\n";
    } else {
        std::cout << "  submit_sync returned: " << sync_result.error().message() << "\n";
    }

    // Shutdown
    server_running.store(false);
    server_thread.join();

    std::cout << "\n[done] Pipeline throughput example complete.\n";
    return 0;
}
