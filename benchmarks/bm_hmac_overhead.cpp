/// @file bm_hmac_overhead.cpp
/// @brief Benchmark HMAC-SHA256 operations and nonce generation.

#include <benchmark/benchmark.h>

#include <modbus_pp/core/types.hpp>
#include <modbus_pp/security/hmac_auth.hpp>

#include <array>
#include <cstdint>
#include <vector>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// Fixed-size key used across all benchmarks (32 bytes)
// ---------------------------------------------------------------------------

static std::vector<byte_t> make_key() {
    std::vector<byte_t> key(32);
    for (std::size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<byte_t>(i ^ 0x5A);
    }
    return key;
}

static std::vector<byte_t> make_payload(std::size_t size) {
    std::vector<byte_t> payload(size);
    for (std::size_t i = 0; i < size; ++i) {
        payload[i] = static_cast<byte_t>(i & 0xFF);
    }
    return payload;
}

// ---------------------------------------------------------------------------
// BM_HMAC_Compute: compute HMAC on a 64-byte payload
// ---------------------------------------------------------------------------

static void BM_HMAC_Compute(benchmark::State& state) {
    auto key     = make_key();
    auto payload = make_payload(64);

    for (auto _ : state) {
        auto digest = HMACAuth::compute(payload, key);
        benchmark::DoNotOptimize(digest);
    }

    state.SetBytesProcessed(state.iterations() * 64);
}
BENCHMARK(BM_HMAC_Compute);

// ---------------------------------------------------------------------------
// BM_HMAC_Verify: verify HMAC (compute + constant-time compare)
// ---------------------------------------------------------------------------

static void BM_HMAC_Verify(benchmark::State& state) {
    auto key     = make_key();
    auto payload = make_payload(64);
    auto digest  = HMACAuth::compute(payload, key);

    for (auto _ : state) {
        bool ok = HMACAuth::verify(payload, key, digest);
        benchmark::DoNotOptimize(ok);
    }

    state.SetBytesProcessed(state.iterations() * 64);
}
BENCHMARK(BM_HMAC_Verify);

// ---------------------------------------------------------------------------
// BM_HMAC_ComputeVaryingSize: parameterized by payload size
// ---------------------------------------------------------------------------

static void BM_HMAC_ComputeVaryingSize(benchmark::State& state) {
    auto payload_size = static_cast<std::size_t>(state.range(0));
    auto key          = make_key();
    auto payload      = make_payload(payload_size);

    for (auto _ : state) {
        auto digest = HMACAuth::compute(payload, key);
        benchmark::DoNotOptimize(digest);
    }

    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(payload_size));
}
BENCHMARK(BM_HMAC_ComputeVaryingSize)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

// ---------------------------------------------------------------------------
// BM_Nonce_Generate: generate a cryptographically random 16-byte nonce
// ---------------------------------------------------------------------------

static void BM_Nonce_Generate(benchmark::State& state) {
    for (auto _ : state) {
        auto nonce = HMACAuth::generate_nonce();
        benchmark::DoNotOptimize(nonce);
    }
}
BENCHMARK(BM_Nonce_Generate);
