---
sidebar_position: 1
---

# Testing & Benchmarking

Overview of test coverage and performance measurements.

## Test Suites

modbus_pp includes **13 test suites** covering:

| Suite                  | Tests | Coverage                                 |
| ---------------------- | ----- | ---------------------------------------- |
| test_pdu.cpp           | 15    | PDU serialization, standard & extended   |
| test_error.cpp         | 8     | Error codes, std::error_code integration |
| test_result.cpp        | 12    | `Result<T>` monad operations               |
| test_endian.cpp        | 16    | All 4 byte orders                        |
| test_register_map.cpp  | 10    | Type safety, overlap detection           |
| test_type_codec.cpp    | 20    | Float, Int, String encoding              |
| test_frame_codec.cpp   | 8     | MBAP and CRC16 wrapping                  |
| test_tcp_transport.cpp | 10    | Socket operations, MBAP                  |
| test_rtu_transport.cpp | 8     | Serial, CRC16                            |
| test_loopback.cpp      | 6     | In-process transport                     |
| test_security.cpp      | 12    | HMAC, nonce, handshake                   |
| test_pipeline.cpp      | 14    | Correlation IDs, async submit            |
| test_client_server.cpp | 9     | Integration tests                        |

**Total:** ~150 test cases

## Running Tests

```bash
cd build
ctest --output-on-failure
```

## Benchmarks

5 key performance benchmarks:

### 1. Sequential vs. Pipelined (THE KEY BENCHMARK)

```
Scenario: 16 read requests over 10ms-latency loopback

Sequential:  160ms (wait for each response)
Pipelined:   20ms  (all in flight simultaneously)
Speedup:     8x
```

### 2. Byte-Order Performance

```
Encode/decode 1000 Float32 values:
- ABCD:      ~100µs  (no-op, big-endian native)
- DCBA:      ~110µs  (single BSWAP)
- BADC:      ~105µs  (byte shuffle)
- CDAB:      ~105µs  (word swap)

→ All within micro-optimization noise, zero practical difference
```

### 3. HMAC Overhead

```
1000 HMAC-SHA256 operations:
- Per operation: ~50µs (OpenSSL optimized)
- For typical 100-byte frame: negligible % overhead
```

### 4. Type Codec Performance

```
1000 Float32 encode/decode:
- No byte order: ~50ns (memcpy)
- BADC:         ~55ns (memcpy + reorder)
- Pipeline:     ~40ns (bulk throughput)

→ Codec essentially free
```

### 5. Throughput Benchmark

```
Register read throughput on loopback with pipelining:
- 100 requests:   ~10,000 req/sec
- 1000 requests:  ~15,000 req/sec (stabilized)

→ Bottleneck is loopback latency (µs), not codec
```

## Running Benchmarks

```bash
cd build/benchmarks
./bm_pipeline_vs_sequential
./bm_type_codec
./bm_lockfree_vs_mutex
./bm_throughput
./bm_latency
```

## Benchmark Results

See [Benchmarks Page](../benchmarks.md) for detailed charts and interpretation.

---

**Next:** [Test Coverage](./test-coverage.md) or [Benchmarks](../benchmarks.md)
