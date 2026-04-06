/// @file bm_pdu_serialize.cpp
/// @brief Benchmark PDU serialization/deserialization for standard and extended frames.

#include <benchmark/benchmark.h>

#include <modbus_pp/core/pdu.hpp>
#include <modbus_pp/core/function_codes.hpp>
#include <modbus_pp/core/timestamp.hpp>
#include <modbus_pp/core/types.hpp>
#include <modbus_pp/transport/frame_codec.hpp>

#include <array>
#include <cstdint>
#include <vector>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// Helpers — build representative PDUs once
// ---------------------------------------------------------------------------

static PDU make_read_holding_pdu() {
    // FC 0x03 ReadHoldingRegisters, start=0x0000, count=10
    std::vector<byte_t> data = {0x00, 0x00, 0x00, 0x0A};
    return PDU::make_standard(FunctionCode::ReadHoldingRegisters, std::move(data));
}

static PDU make_extended_pdu() {
    FrameHeader hdr;
    hdr.version           = 1;
    hdr.flags             = FrameFlag::HasTimestamp;
    hdr.correlation_id    = 42;
    hdr.timestamp         = Timestamp::now();
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
    hdr.payload_length    = 20;

    std::vector<byte_t> payload(20, 0xAB);
    return PDU::make_extended(std::move(hdr), std::move(payload));
}

// ---------------------------------------------------------------------------
// Standard PDU benchmarks
// ---------------------------------------------------------------------------

static void BM_StandardPDU_Serialize(benchmark::State& state) {
    auto pdu = make_read_holding_pdu();
    for (auto _ : state) {
        auto bytes = pdu.serialize();
        benchmark::DoNotOptimize(bytes);
    }
}
BENCHMARK(BM_StandardPDU_Serialize);

static void BM_StandardPDU_Deserialize(benchmark::State& state) {
    auto pdu       = make_read_holding_pdu();
    auto raw_bytes = pdu.serialize();
    for (auto _ : state) {
        auto result = PDU::deserialize(raw_bytes);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_StandardPDU_Deserialize);

// ---------------------------------------------------------------------------
// Extended PDU benchmarks
// ---------------------------------------------------------------------------

static void BM_ExtendedPDU_Serialize(benchmark::State& state) {
    auto pdu = make_extended_pdu();
    for (auto _ : state) {
        auto bytes = pdu.serialize();
        benchmark::DoNotOptimize(bytes);
    }
}
BENCHMARK(BM_ExtendedPDU_Serialize);

static void BM_ExtendedPDU_Deserialize(benchmark::State& state) {
    auto pdu       = make_extended_pdu();
    auto raw_bytes = pdu.serialize();
    for (auto _ : state) {
        auto result = PDU::deserialize(raw_bytes);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ExtendedPDU_Deserialize);

// ---------------------------------------------------------------------------
// FrameCodec benchmarks
// ---------------------------------------------------------------------------

static void BM_FrameCodec_WrapTCP(benchmark::State& state) {
    auto pdu = make_read_holding_pdu();
    for (auto _ : state) {
        auto frame = FrameCodec::wrap_tcp(1, pdu, 0x0001);
        benchmark::DoNotOptimize(frame);
    }
}
BENCHMARK(BM_FrameCodec_WrapTCP);

static void BM_FrameCodec_UnwrapTCP(benchmark::State& state) {
    auto pdu       = make_read_holding_pdu();
    auto tcp_frame = FrameCodec::wrap_tcp(1, pdu, 0x0001);
    for (auto _ : state) {
        auto result = FrameCodec::unwrap_tcp(tcp_frame);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FrameCodec_UnwrapTCP);

static void BM_FrameCodec_CRC16(benchmark::State& state) {
    std::vector<byte_t> data(256);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<byte_t>(i & 0xFF);
    }
    for (auto _ : state) {
        auto crc = FrameCodec::crc16(data);
        benchmark::DoNotOptimize(crc);
    }
}
BENCHMARK(BM_FrameCodec_CRC16);
