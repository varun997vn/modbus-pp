# Architecture

This document describes the design rationale, wire format, module structure, and key implementation decisions of the modbus_pp library.

## Design Philosophy

modbus_pp follows three core principles:

1. **Wire compatibility first** — Standard Modbus frames (FC 0x01–0x10) pass through the library unchanged. A modbus_pp client can communicate with any standard Modbus device using standard function codes. Extended features only activate when both endpoints support them.

2. **Compile-time safety** — Register address overlaps are caught by `static_assert`. Byte order selection compiles down to direct byte-swap instructions via `constexpr if`. Strong type wrappers prevent accidental parameter mixing.

3. **Zero-overhead abstractions** — The library pays no runtime cost for features you don't use. `Result<T>` avoids exceptions in the data path. Template-based codecs generate optimal code per byte order at compile time.

## Module Dependency Graph

```
                    ┌──────────┐
                    │  client/  │  High-level facades
                    │  server   │
                    └────┬─────┘
           ┌─────────┬──┴──┬──────────┬───────────┐
           │         │     │          │           │
     ┌─────▼──┐ ┌───▼───┐ │   ┌──────▼───┐ ┌────▼─────┐
     │pipeline│ │pubsub │ │   │discovery │ │security  │
     └────┬───┘ └───┬───┘ │   └────┬─────┘ └────┬─────┘
          │         │     │        │             │
          └─────┬───┘     │        │             │
                │   ┌─────▼────┐   │             │
                │   │transport │◄──┘             │
                │   └─────┬────┘                 │
                │         │                      │
          ┌─────▼─────────▼──────────────────────▼─┐
          │              core/                       │
          │  types, pdu, error, result, endian,      │
          │  timestamp, function_codes               │
          └──────────────────────────────────────────┘
```

Every module depends only on `core/` and (optionally) `transport/`. There are no circular dependencies. The `client/` and `server/` facades compose all other modules into a unified API.

## Wire Format

### Standard Modbus ADU (TCP/MBAP)

```
┌──────────────┬────────────┬────────┬─────────┬──────────┐
│ Transaction  │ Protocol   │ Length │ Unit ID │   PDU    │
│   ID (2B)    │   ID (2B)  │  (2B)  │  (1B)   │ (N bytes)│
└──────────────┴────────────┴────────┴─────────┴──────────┘
```

### Standard Modbus ADU (RTU)

```
┌─────────┬──────────┬──────────┐
│ Unit ID │   PDU    │ CRC16    │
│  (1B)   │ (N bytes)│  (2B)    │
└─────────┴──────────┴──────────┘
```

### Standard PDU

```
┌───────────────┬───────────────┐
│ Function Code │     Data      │
│     (1B)      │  (0-252 B)    │
└───────────────┴───────────────┘
```

### Extended PDU (FC = 0x6E)

```
┌──────┬─────────┬───────┬──────────────┬────────────┬───────┬────────────┬─────────┬──────┐
│ 0x6E │ Version │ Flags │ Correlation  │ Timestamp? │ ExtFC │ PayloadLen │ Payload │HMAC? │
│ (1B) │  (1B)   │ (2B)  │   ID (2B)    │   (8B)     │ (1B)  │   (2B)     │  (N B)  │(32B) │
└──────┴─────────┴───────┴──────────────┴────────────┴───────┴────────────┴─────────┴──────┘
```

**Why FC 0x6E?** The Modbus specification reserves function codes 0x41–0x48 and 0x64–0x6E for user-defined use. We use 0x6E as the escape code for all extended operations, with sub-function codes in the ExtFC field.

**Optional fields:** The Timestamp (8 bytes) and HMAC (32 bytes) fields are only present when their respective flags are set. This minimizes overhead when these features are not needed.

## Module Details

### core/types.hpp — Type Foundation

```cpp
using byte_t     = std::uint8_t;
using register_t = std::uint16_t;
using unit_id_t  = std::uint8_t;
using address_t  = std::uint16_t;
using quantity_t = std::uint16_t;
```

**StrongType<Tag, T>** prevents accidental mixing of semantically distinct values:

```cpp
RegisterAddress addr(100);
RegisterCount   count(10);
// addr + count → compile error (different tags)
```

**span_t<T>** is a minimal non-owning view that replaces gsl::span/std::span (C++20) without introducing a dependency. It constructs implicitly from `std::vector`, `std::array`, and C arrays.

### core/endian.hpp — Zero-Cost Byte Order

Different Modbus device vendors use different byte orderings for multi-register values:

| Order | Meaning | Common Vendors |
|-------|---------|----------------|
| ABCD | Big-endian (Modbus default) | Siemens, Honeywell |
| DCBA | Little-endian | Some Emerson devices |
| BADC | Mid-big / byte-swapped | Schneider Electric, ABB |
| CDAB | Mid-little / word-swapped | Daniel flow computers |

The `constexpr if` dispatch in `detail::reorder<Order>()` compiles down to a single byte-swap instruction sequence for each order — there is no runtime switch/case or vtable involved.

### core/error.hpp — std::error_code Integration

Standard Modbus defines 9 exception codes (1–11). modbus_pp extends this with 16 additional error codes (0x80+) for extended features:

```cpp
enum class ErrorCode : int {
    // Standard Modbus (1-11)
    IllegalFunction = 1,
    IllegalDataAddress = 2,
    // ...

    // Extended (0x80+)
    AuthenticationFailed = 0x80,
    PipelineOverflow = 0x83,
    TimeoutExpired = 0x88,
    // ...
};
```

All error codes are registered with `std::error_category`, so they work with standard C++ error handling infrastructure (`std::error_code`, `std::system_error`).

### core/result.hpp — Monadic Error Type

`Result<T>` wraps `std::variant<T, std::error_code>` with a monadic API:

```cpp
Result<PDU> parse_frame(span_t<const byte_t> data) {
    if (data.empty()) return ErrorCode::InvalidFrame;
    return PDU::deserialize(data);
}

// Chainable operations
auto value = parse_frame(data)
    .and_then([](PDU& pdu) { return validate(pdu); })
    .map([](PDU& pdu) { return pdu.payload(); })
    .value_or(default_payload);
```

There is a `Result<void>` specialization for operations that succeed or fail without returning a value.

### register_map/ — Compile-Time Type Safety

**RegisterDescriptor** binds address, type, count, and byte order at compile time:

```cpp
using Temperature = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
using Status      = RegisterDescriptor<RegisterType::UInt16,  0x0002>;
using Pressure    = RegisterDescriptor<RegisterType::Float64, 0x0003>;
```

**RegisterMap** validates no overlapping addresses via `static_assert`:

```cpp
using SensorMap = RegisterMap<Temperature, Status, Pressure>;
// If Temperature (0x0000-0x0001) overlapped with Status (0x0002),
// compilation fails with: "Register descriptors have overlapping address ranges"
```

The overlap detection uses recursive template metaprogramming to check all pairs:

```cpp
template <typename First, typename Second, typename... Rest>
struct no_overlaps {
    static constexpr bool value =
        !descriptors_overlap<First, Second>::value &&
        no_overlaps<First, Rest...>::value &&
        no_overlaps<Second, Rest...>::value;
};
```

### transport/ — Abstract Transport Layer

```
Transport (abstract)
├── LoopbackTransport  — paired in-process (testing/benchmarking)
├── TCPTransport       — POSIX sockets, MBAP framing, TCP_NODELAY
└── RTUTransport       — termios serial, CRC16 framing
```

**LoopbackTransport** creates two linked endpoints. Data sent on one appears as received on the other. Supports simulated latency and error injection for realistic integration testing.

**FrameCodec** handles ADU wrapping/unwrapping for both TCP (MBAP header with transaction IDs) and RTU (CRC16 checksum with unit ID).

### security/ — HMAC-SHA256 Authentication

The 4-step challenge-response handshake:

```
Client                              Server
  │                                    │
  │──── AuthChallenge {Nc} ───────────▶│   Step 1: Client sends nonce
  │                                    │
  │◀─── {Ns, HMAC(Nc‖Ns, K)} ────────│   Step 2: Server proves knowledge of K
  │                                    │
  │──── AuthResponse {HMAC(Ns‖Nc, K)}─▶│   Step 3: Client proves knowledge of K
  │                                    │
  │◀─── {SessionToken, Expiry} ───────│   Step 4: Session established
  │                                    │
```

- **Nc, Ns** — Client and server nonces (16 bytes each, from `RAND_bytes`)
- **K** — Pre-shared secret
- **HMAC** — HMAC-SHA256 via OpenSSL
- **Constant-time comparison** — Prevents timing side-channel attacks on HMAC verification

After authentication, a session key is derived: `session_key = HMAC(Nc‖Ns, K)`. All subsequent extended frames can be signed with this key.

### pipeline/ — Pipelined Transactions

Standard Modbus is strictly sequential: send request, wait for response, repeat. Over a 10ms-latency link, 16 sequential requests take 160ms.

The Pipeline tags each request with a correlation ID in the extended frame header, sends multiple requests without waiting for responses, and matches responses to requests by correlation ID.

```
Sequential:  [Req1]──[Resp1]──[Req2]──[Resp2]──...──[Req16]──[Resp16]
              │◄──────────── 160ms ──────────────────────────────────►│

Pipelined:   [Req1][Req2]...[Req16]──[Resp1][Resp2]...[Resp16]
              │◄──────────── ~20ms ─────────────────────────►│
```

The `RequestQueue` is thread-safe and supports timeout expiry detection with automatic cleanup.

### pubsub/ — Event-Driven Push

Three trigger modes replace polling:

| Trigger | Description | Use Case |
|---------|-------------|----------|
| OnChange | Push when value changes beyond dead-band | Temperature monitoring |
| OnThreshold | Push when value crosses high/low bounds | Alarm conditions |
| Periodic | Push at fixed intervals | Data logging |

**Dead-band filtering** prevents noisy sensor data from flooding the bus. A dead-band of 5.0 means the server only pushes a notification when the value has changed by more than 5.0 units since the last notification.

### discovery/ — Device Scanning

The Scanner probes a range of unit addresses (default 1–247) with Read Device Identification requests (FC 0x2B/0x0E). For each responding device, it records:

- Unit ID, vendor, product code, firmware version
- Whether it supports modbus_pp extensions
- Capability bitfield (Pipeline, PubSub, BatchOps, HMACAuth, Timestamps)

## Key Implementation Decisions

### Why not Boost.Asio?

POSIX sockets with `poll()` are sufficient for Modbus TCP and avoid a heavy dependency. The library targets embedded-adjacent environments where Boost may not be available.

### Why variant-based Result<T> instead of exceptions?

Modbus communication fails routinely (device offline, timeout, CRC error). These are expected outcomes, not exceptional situations. `Result<T>` makes the error path explicit and avoids the overhead of exception handling in the hot path.

### Why OpenSSL instead of a standalone HMAC implementation?

OpenSSL's libcrypto is available on virtually every Linux system and provides audited, hardware-accelerated HMAC-SHA256. Rolling our own crypto would be a security liability.

### Why custom span_t instead of gsl::span?

A 15-line `span_t` template avoids pulling in the entire Guidelines Support Library. The API surface we need (data, size, subspan, begin/end, implicit construction from containers) is trivially implementable.
