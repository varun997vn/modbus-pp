/// @file bm_type_codec.cpp
/// @brief Benchmark type encoding/decoding for all byte orders.

#include <benchmark/benchmark.h>

#include <modbus_pp/core/endian.hpp>
#include <modbus_pp/core/types.hpp>
#include <modbus_pp/register_map/type_codec.hpp>

#include <array>
#include <string>
#include <vector>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// Float encode/decode — templated by ByteOrder
// ---------------------------------------------------------------------------

template <ByteOrder Order>
static void BM_Float_Encode(benchmark::State& state) {
    float value = 3.14159f;
    for (auto _ : state) {
        auto regs = TypeCodec::encode_float<Order>(value);
        benchmark::DoNotOptimize(regs);
    }
}

template <ByteOrder Order>
static void BM_Float_Decode(benchmark::State& state) {
    auto regs = TypeCodec::encode_float<Order>(2.71828f);
    for (auto _ : state) {
        float val = TypeCodec::decode_float<Order>(regs.data());
        benchmark::DoNotOptimize(val);
    }
}

BENCHMARK(BM_Float_Encode<ByteOrder::ABCD>);
BENCHMARK(BM_Float_Encode<ByteOrder::DCBA>);
BENCHMARK(BM_Float_Encode<ByteOrder::BADC>);
BENCHMARK(BM_Float_Encode<ByteOrder::CDAB>);

BENCHMARK(BM_Float_Decode<ByteOrder::ABCD>);
BENCHMARK(BM_Float_Decode<ByteOrder::DCBA>);
BENCHMARK(BM_Float_Decode<ByteOrder::BADC>);
BENCHMARK(BM_Float_Decode<ByteOrder::CDAB>);

// ---------------------------------------------------------------------------
// Double encode/decode — ABCD
// ---------------------------------------------------------------------------

static void BM_Double_Encode_ABCD(benchmark::State& state) {
    double value = 3.141592653589793;
    for (auto _ : state) {
        auto regs = TypeCodec::encode_double<ByteOrder::ABCD>(value);
        benchmark::DoNotOptimize(regs);
    }
}
BENCHMARK(BM_Double_Encode_ABCD);

static void BM_Double_Decode_ABCD(benchmark::State& state) {
    auto regs = TypeCodec::encode_double<ByteOrder::ABCD>(2.718281828459045);
    for (auto _ : state) {
        double val = TypeCodec::decode_double<ByteOrder::ABCD>(regs.data());
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Double_Decode_ABCD);

// ---------------------------------------------------------------------------
// UInt32 encode/decode — ABCD
// ---------------------------------------------------------------------------

static void BM_UInt32_Encode_ABCD(benchmark::State& state) {
    std::uint32_t value = 0xDEADBEEF;
    for (auto _ : state) {
        auto regs = TypeCodec::encode_uint32<ByteOrder::ABCD>(value);
        benchmark::DoNotOptimize(regs);
    }
}
BENCHMARK(BM_UInt32_Encode_ABCD);

static void BM_UInt32_Decode_ABCD(benchmark::State& state) {
    auto regs = TypeCodec::encode_uint32<ByteOrder::ABCD>(0xCAFEBABE);
    for (auto _ : state) {
        std::uint32_t val = TypeCodec::decode_uint32<ByteOrder::ABCD>(regs.data());
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_UInt32_Decode_ABCD);

// ---------------------------------------------------------------------------
// String encode/decode — 32-char string (16 registers)
// ---------------------------------------------------------------------------

static void BM_String_Encode(benchmark::State& state) {
    std::string str = "ModbusPP benchmark test string!!";  // exactly 32 chars
    for (auto _ : state) {
        auto regs = TypeCodec::encode_string(str, 16);
        benchmark::DoNotOptimize(regs);
    }
}
BENCHMARK(BM_String_Encode);

static void BM_String_Decode(benchmark::State& state) {
    std::string str = "ModbusPP benchmark test string!!";
    auto regs = TypeCodec::encode_string(str, 16);
    for (auto _ : state) {
        auto decoded = TypeCodec::decode_string(regs.data(), 16);
        benchmark::DoNotOptimize(decoded);
    }
}
BENCHMARK(BM_String_Decode);
