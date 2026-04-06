---
sidebar_position: 1
---

# Features Overview

modbus_pp provides 10 key features that solve protocol limitations. Each can be used independently or combined.

## Feature List

| Feature                                         | Problem Solved        | Complexity | When to Use                                    |
| ----------------------------------------------- | --------------------- | ---------- | ---------------------------------------------- |
| **[Typed Registers](./typed-registers.md)**     | Weak data types       | ⭐         | Always (recommended)                           |
| **[Security (HMAC)](./security-hmac.md)**       | No authentication     | ⭐⭐       | Production systems with untrusted networks     |
| **[Pipelining](./pipelining.md)**               | Sequential throughput | ⭐⭐       | High-latency links (satellite, cellular, etc.) |
| **[Pub/Sub](./pubsub.md)**                      | Poll-based only       | ⭐⭐       | Event-driven systems, battery-powered devices  |
| **[Discovery](./discovery.md)**                 | Manual discovery      | ⭐         | Auto-discovery of bus devices                  |
| **[Batch Operations](./batch-operations.md)**   | Batch inflexibility   | ⭐         | Reading/writing multiple register ranges       |
| **[Byte Order Safety](./byte-order-safety.md)** | Byte order chaos      | ⭐         | Multi-vendor device environments               |
| **[Extended Payloads](./extended-payloads.md)** | 253-byte limit        | ⭐⭐       | Large data transfers (arrays, strings)         |
| **[Timestamps](./timestamps.md)**               | No timestamps         | ⭐         | Debugging, time-series analysis                |
| **[Rich Errors](./error-handling.md)**          | Poor error codes      | ⭐         | Production debugging                           |

---

## Quick Examples

### Typed Registers (Type Safety)

```cpp
using Temp = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;
auto value = TypeCodec::decode<Temp>(registers);  // Returns Result<float>
```

### Security (HMAC)

```cpp
auto result = session.authenticate("shared_secret");
if (result) { /* now all frames are HMAC-authenticated */ }
```

### Pipelining (High Throughput)

```cpp
auto id = pipeline.submit(request1, [](Result<PDU> r) { ... });
auto id = pipeline.submit(request2, [](Result<PDU> r) { ... });
// Both in flight simultaneously → 8x faster!
```

### Pub/Sub (Push Notifications)

```cpp
subscriber.subscribe(0x0000, 10, SubscriptionMode::OnChange);
// Server pushes updates when data changes (with dead-band)
```

---

## Feature Combinations

You can use features independently or together:

- **Baseline:** Type-safe client/server with standard Modbus
- **Secure Baseline:** + Authentication
- **High-Performance:** + Pipelining
- **Event-Driven:** + Pub/Sub
- **Enterprise:** + Security + Pipelining + Pub/Sub + Discovery

---

**Pick a feature above to learn more!**
