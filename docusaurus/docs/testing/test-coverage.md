---
sidebar_position: 2
---

# Test Coverage

Detailed mapping of test suites to features.

## Feature Coverage Matrix

| Feature                | Unit Tests                     | Integration | Coverage |
| ---------------------- | ------------------------------ | ----------- | -------- |
| **Core Types**         | test_types.cpp                 | ✓           | High     |
| **`Result<T>`** | test_result.cpp                | ✓           | High     |
| **Errors**             | test_error.cpp                 | ✓           | High     |
| **PDU (Standard)**     | test_pdu.cpp                   | ✓           | High     |
| **PDU (Extended)**     | test_pdu.cpp                   | ✓           | High     |
| **Byte Orders**        | test_endian.cpp (16 tests)     | ✓           | High     |
| **Register Maps**      | test_register_map.cpp          | ✓           | High     |
| **Type Codecs**        | test_type_codec.cpp (20 tests) | ✓           | High     |
| **MBAP Framing**       | test_frame_codec.cpp           | ✓           | High     |
| **CRC16 Framing**      | test_frame_codec.cpp           | ✓           | High     |
| **TCP Transport**      | test_tcp_transport.cpp         | ✓           | High     |
| **RTU Transport**      | test_rtu_transport.cpp         | ✓           | Medium   |
| **Loopback Transport** | test_loopback.cpp              | ✓           | High     |
| **Security (HMAC)**    | test_security.cpp (12 tests)   | ✓           | High     |
| **Pipelining**         | test_pipeline.cpp (14 tests)   | ✓           | High     |
| **Pub/Sub**            | test_pubsub.cpp                | ✓           | Medium   |
| **Discovery**          | test_discovery.cpp             | ✓           | Medium   |
| **Client/Server**      | test_client_server.cpp         | ✓           | High     |

## Critical Test Cases

### Pipelining (test_pipeline.cpp)

Tests correlation ID matching, in-flight tracking, timeouts:

```cpp
TEST(Pipeline, SubmitMultipleRequestsAndReceive) {
    // Submit 16 async requests
    // Receive responses in different order
    // Verify callbacks fired for correct correlation IDs
}

TEST(Pipeline, TimeoutExpiredRemovesStaleRequest) {
    // Submit request with 100ms timeout
    // Simulate no response for 200ms
    // Verify request cleaned up automatically
}
```

### Type Codec Tests (test_type_codec.cpp)

All 4 byte orders × all types:

```cpp
PARAMETRIZE(ByteOrder, {ABCD, DCBA, BADC, CDAB});
PARAMETRIZE(Type, {Float32, Float64, Int32, String});

TEST(TypeCodec, EncodeDecodeRoundtrip) {
    // For each combination, verify encode→decode = original value
}
```

### Integration Test (test_client_server.cpp)

End-to-end client-server communication:

```cpp
TEST(ClientServer, ReadWriteIntegration) {
    // Server: register read handler
    // Client: submit read request
    // Verify response matches server state
    // Repeat for write operations
}

TEST(ClientServer, PipelinedReadsWithServer) {
    // Server handles multiple concurrent requests
    // Client pipelines 16 reads
    // Verify all responses correct, no corruption
}
```

## Example Coverage

All **7 examples** are effectively integration tests:

| Example                  | Tests               | Coverage                     |
| ------------------------ | ------------------- | ---------------------------- |
| `01_simple_pipeline`     | Basic client/server | Wire protocol, facades       |
| `02_typed_registers`     | Type-safe access    | RegisterDescriptor, overflow |
| `03_failure_propagation` | Error handling      | `Result<T>`, error codes       |
| `04_pipeline_throughput` | Pipelining          | Pipeline, async callbacks    |
| `05_pubsub_events`       | Push notifications  | Publisher, dead-band         |
| `06_security_hmac`       | Authentication      | HMAC, handshake              |
| `07_endian_byte_orders`  | Byte order handling | All 4 orders                 |

## Test Infrastructure

### Google Test (gtest)

All tests use Google Test framework:

```bash
ctest --output-on-failure      # Run all
ctest -R pipeline              # Run tests matching "pipeline"
./test_pipeline --gtest_filter "*TimeoutExpired*"
```

### Loopback Transport for Isolation

Tests use `LoopbackTransport` to avoid:

- Network setup
- Serial port dependencies
- Timing flakiness
- Port conflicts

### RAII for Cleanup

All tests use RAII (Resource Acquisition Is Initialization):

```cpp
TEST_F(ClientServerFixture, ...) {
    SetUp();      // Create loopback pair, connect transports
    // Test code
    TearDown();   // Auto cleanup via destructors
}
```

---

**Next:** [Benchmarks](../benchmarks.md) or [Testing Overview](./overview.md)
