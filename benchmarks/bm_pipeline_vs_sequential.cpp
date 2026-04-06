/// @file bm_pipeline_vs_sequential.cpp
/// @brief THE KEY BENCHMARK: pipelined vs sequential request throughput.
///
/// Demonstrates throughput improvement when using Pipeline to submit
/// multiple requests concurrently over a loopback transport, compared to
/// traditional one-at-a-time Client::read_holding_registers calls.

#include <benchmark/benchmark.h>

#include <modbus_pp/client/client.hpp>
#include <modbus_pp/client/server.hpp>
#include <modbus_pp/core/function_codes.hpp>
#include <modbus_pp/core/pdu.hpp>
#include <modbus_pp/core/types.hpp>
#include <modbus_pp/pipeline/pipeline.hpp>
#include <modbus_pp/transport/loopback_transport.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

// ---------------------------------------------------------------------------
// BM_Sequential: N requests one-at-a-time via Client
// ---------------------------------------------------------------------------

static void BM_Sequential(benchmark::State& state) {
    auto num_requests = static_cast<int>(state.range(0));

    // Setup once
    auto pair = LoopbackTransport::create_pair();
    auto ct = std::shared_ptr<LoopbackTransport>(std::move(pair.first));
    auto st = std::shared_ptr<LoopbackTransport>(std::move(pair.second));
    ct->connect();
    st->connect();

    std::atomic<bool> running{true};
    std::thread srv_thread([&] {
        Server server(ServerConfig{st, 1, false});
        server.on_read_holding_registers(
            [](address_t, quantity_t count) -> Result<std::vector<reg_t>> {
                return std::vector<reg_t>(count, 42);
            });
        while (running.load(std::memory_order_relaxed))
            server.process_one();
    });

    Client client(ClientConfig{ct, nullptr, {}, false,
                                std::chrono::milliseconds{500}});

    for (auto _ : state) {
        for (int i = 0; i < num_requests; ++i) {
            auto result = client.read_holding_registers(1, 0, 10);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetItemsProcessed(state.iterations() * num_requests);

    running = false;
    srv_thread.join();
}
BENCHMARK(BM_Sequential)->Arg(1)->Arg(4)->Arg(8)->Arg(16)
    ->Unit(benchmark::kMicrosecond);

// ---------------------------------------------------------------------------
// BM_Pipelined: submit all N requests at once, then poll
// ---------------------------------------------------------------------------

static void BM_Pipelined(benchmark::State& state) {
    auto num_requests = static_cast<int>(state.range(0));

    // Setup once — raw responder echoes extended PDUs with correlation IDs
    auto pair = LoopbackTransport::create_pair();
    auto ct = std::shared_ptr<LoopbackTransport>(std::move(pair.first));
    auto st = std::shared_ptr<LoopbackTransport>(std::move(pair.second));
    ct->connect();
    st->connect();

    std::atomic<bool> running{true};
    std::thread responder([&] {
        while (running.load(std::memory_order_relaxed)) {
            auto recv = st->receive(std::chrono::milliseconds{5});
            if (!recv) continue;
            auto pdu = PDU::deserialize(span_t<const byte_t>{recv.value()});
            if (!pdu) continue;
            auto resp_bytes = pdu.value().serialize();
            st->send(span_t<const byte_t>{resp_bytes});
        }
    });

    PipelineConfig pipe_cfg;
    pipe_cfg.max_in_flight = 32;
    pipe_cfg.default_timeout = std::chrono::milliseconds{500};
    Pipeline pipeline(ct, pipe_cfg);

    for (auto _ : state) {
        std::atomic<int> completed{0};

        for (int i = 0; i < num_requests; ++i) {
            std::vector<byte_t> data = {0x00, 0x00, 0x00, 0x0A};
            auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                           std::move(data));
            pipeline.submit(
                std::move(pdu),
                [&completed](Result<PDU> r) {
                    benchmark::DoNotOptimize(r);
                    completed.fetch_add(1, std::memory_order_relaxed);
                },
                std::chrono::milliseconds{500});
        }

        while (completed.load(std::memory_order_relaxed) < num_requests) {
            pipeline.poll();
        }
    }

    state.SetItemsProcessed(state.iterations() * num_requests);

    running = false;
    responder.join();
}
BENCHMARK(BM_Pipelined)->Arg(1)->Arg(4)->Arg(8)->Arg(16)
    ->Unit(benchmark::kMicrosecond);
