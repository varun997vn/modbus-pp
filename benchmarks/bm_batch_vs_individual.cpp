/// @file bm_batch_vs_individual.cpp
/// @brief THE KEY BENCHMARK: batch read vs individual register reads.
///
/// Demonstrates fewer round trips when using BatchRequest to aggregate
/// multiple register reads into a single extended frame, compared to
/// issuing individual ReadHoldingRegisters calls for each range.

#include <benchmark/benchmark.h>

#include <modbus_pp/client/client.hpp>
#include <modbus_pp/client/server.hpp>
#include <modbus_pp/core/function_codes.hpp>
#include <modbus_pp/core/pdu.hpp>
#include <modbus_pp/core/timestamp.hpp>
#include <modbus_pp/core/types.hpp>
#include <modbus_pp/register_map/batch_request.hpp>
#include <modbus_pp/transport/frame_codec.hpp>
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
// BM_Individual: N separate read_holding_registers calls (N round trips)
// ---------------------------------------------------------------------------

static void BM_Individual(benchmark::State& state) {
    auto num_ranges = static_cast<int>(state.range(0));

    auto pair = LoopbackTransport::create_pair();
    auto ct = std::shared_ptr<LoopbackTransport>(std::move(pair.first));
    auto st = std::shared_ptr<LoopbackTransport>(std::move(pair.second));
    ct->connect();
    st->connect();

    std::atomic<bool> running{true};
    std::thread srv_thread([&] {
        Server server(ServerConfig{st, 1, false});
        server.on_read_holding_registers(
            [](address_t addr, quantity_t count) -> Result<std::vector<reg_t>> {
                std::vector<reg_t> regs(count);
                for (quantity_t i = 0; i < count; ++i)
                    regs[i] = static_cast<reg_t>(addr + i);
                return regs;
            });
        while (running.load(std::memory_order_relaxed))
            server.process_one();
    });

    Client client(ClientConfig{ct, nullptr, {}, false,
                                std::chrono::milliseconds{500}});

    for (auto _ : state) {
        for (int i = 0; i < num_ranges; ++i) {
            auto addr = static_cast<address_t>(i * 10);
            auto result = client.read_holding_registers(1, addr, 4);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetItemsProcessed(state.iterations() * num_ranges);

    running = false;
    srv_thread.join();
}
BENCHMARK(BM_Individual)->Arg(5)->Arg(10)->Arg(20)
    ->Unit(benchmark::kMicrosecond);

// ---------------------------------------------------------------------------
// BM_Batch: single batch_read (1 round trip regardless of N ranges)
// ---------------------------------------------------------------------------

static void BM_Batch(benchmark::State& state) {
    auto num_ranges = static_cast<int>(state.range(0));

    auto pair = LoopbackTransport::create_pair();
    auto ct = std::shared_ptr<LoopbackTransport>(std::move(pair.first));
    auto st = std::shared_ptr<LoopbackTransport>(std::move(pair.second));
    ct->connect();
    st->connect();

    // Responder: reads extended batch request, builds BatchResponse, sends back
    std::atomic<bool> running{true};
    std::thread responder([&] {
        while (running.load(std::memory_order_relaxed)) {
            auto recv = st->receive(std::chrono::milliseconds{5});
            if (!recv) continue;

            auto unwrap = FrameCodec::unwrap_tcp(
                span_t<const byte_t>{recv.value()});
            if (!unwrap) continue;

            auto& [unit, pdu, txn_id] = unwrap.value();

            BatchResponse resp;
            if (pdu.is_extended()) {
                auto batch_result = BatchRequest::deserialize(pdu.payload());
                if (batch_result) {
                    for (const auto& item : batch_result.value().items()) {
                        std::vector<reg_t> regs(item.count);
                        for (quantity_t i = 0; i < item.count; ++i)
                            regs[i] = static_cast<reg_t>(item.start_address + i);
                        resp.add_result(item.start_address, std::move(regs));
                    }
                }
            }

            FrameHeader hdr;
            hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
            auto resp_pdu = PDU::make_extended(hdr, resp.serialize());
            auto frame = FrameCodec::wrap_tcp(unit, resp_pdu, txn_id);
            st->send(span_t<const byte_t>{frame});
        }
    });

    // Pre-build the optimized batch request
    BatchRequest batch;
    for (int i = 0; i < num_ranges; ++i) {
        auto addr = static_cast<address_t>(i * 10);
        batch.add(addr, 4, RegisterType::UInt16, ByteOrder::ABCD);
    }
    auto optimized = batch.optimized();

    Client client(ClientConfig{ct, nullptr, {}, false,
                                std::chrono::milliseconds{500}});

    for (auto _ : state) {
        auto result = client.batch_read(1, optimized);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * num_ranges);

    running = false;
    responder.join();
}
BENCHMARK(BM_Batch)->Arg(5)->Arg(10)->Arg(20)
    ->Unit(benchmark::kMicrosecond);

// ---------------------------------------------------------------------------
// BM_BatchOptimize: measure optimization (merge) overhead alone
// ---------------------------------------------------------------------------

static void BM_BatchOptimize(benchmark::State& state) {
    auto num_ranges = static_cast<int>(state.range(0));

    BatchRequest batch;
    for (int i = 0; i < num_ranges; ++i) {
        auto addr = static_cast<address_t>(i * 10);
        batch.add(addr, 4, RegisterType::Float32, ByteOrder::ABCD);
    }

    for (auto _ : state) {
        auto optimized = batch.optimized();
        benchmark::DoNotOptimize(optimized);
    }
}
BENCHMARK(BM_BatchOptimize)->Arg(5)->Arg(10)->Arg(20)->Arg(50);

// ---------------------------------------------------------------------------
// BM_BatchSerialize: measure serialization overhead alone
// ---------------------------------------------------------------------------

static void BM_BatchSerialize(benchmark::State& state) {
    auto num_ranges = static_cast<int>(state.range(0));

    BatchRequest batch;
    for (int i = 0; i < num_ranges; ++i) {
        auto addr = static_cast<address_t>(i * 10);
        batch.add(addr, 4, RegisterType::UInt16, ByteOrder::ABCD);
    }
    auto optimized = batch.optimized();

    for (auto _ : state) {
        auto bytes = optimized.serialize();
        benchmark::DoNotOptimize(bytes);
    }
}
BENCHMARK(BM_BatchSerialize)->Arg(5)->Arg(10)->Arg(20);
