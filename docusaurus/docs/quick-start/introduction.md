---
sidebar_position: 1
---

# Introduction to modbus_pp

Welcome to **modbus_pp** — a modern C++17 Modbus library that solves 10 fundamental limitations of the 1979 Modbus protocol **while keeping 100% backward compatibility** with standard Modbus devices.

## What is modbus_pp?

modbus_pp is a production-ready library for building Modbus clients and servers with:

- ✅ **Type-safe register access** — compile-time validation of register addresses and byte orders
- ✅ **Zero-overhead abstractions** — pay only for what you use; all unused features compile away
- ✅ **Wire compatibility** — talk to any standard Modbus device unchanged
- ✅ **10 protocol enhancements** — security, pipelining, pub/sub, discovery, and more
- ✅ **~4,300 lines** of modern C++ — no Boost, no exceptions in the data path
- ✅ **Production-tested** — 13 test suites, 7 examples, 5 benchmarks

## The 10 Problems We Solve

| Problem                   | Solution                                                               |
| ------------------------- | ---------------------------------------------------------------------- |
| **No authentication**     | HMAC-SHA256 with challenge-response handshake                          |
| **Weak data types**       | Compile-time typed register descriptors (Float32, Int64, String, etc.) |
| **Poll-based only**       | Event-driven Pub/Sub with dead-band filtering                          |
| **253-byte limit**        | Extended payloads (KC kilobytes in single frame)                       |
| **No timestamps**         | Microsecond-precision timestamps in every frame                        |
| **Sequential throughput** | Pipelining with correlation IDs (8x speedup on high-latency links)     |
| **Poor error codes**      | 25+ error codes with std::error_code integration                       |
| **Manual discovery**      | Broadcast device scanning with capability detection                    |
| **Batch inflexibility**   | Automatic range merging for batch operations                           |
| **Byte order chaos**      | 4 byte orders, compile-time selection, zero runtime cost               |

## Quick Facts

- **Language**: C++17
- **Size**: ~4,300 lines
- **Tests**: 13 suites (unit + integration)
- **Examples**: 7 complete examples
- **Benchmarks**: 5 comparison benchmarks
- **License**: MIT (permissive, commercial-friendly)
- **Latest Release**: v1.0.0

## Who Should Use This?

- **IoT platforms** needing lightweight, type-safe protocol handling
- **Industrial automation** requiring security and reliability
- **Embedded systems** with memory constraints (no Boost)
- **Real-time systems** avoiding exceptions in the data path
- **Teams migrating** from standard Modbus, wanting backward compatibility

## Next Steps

1. **Ready to code?** → See [Installation](./installation.md) and [Hello World](./hello-world.md)
2. **Want the big picture first?** → Jump to [Core Concepts](../concepts/overview.md)
3. **Prefer learning by example?** → Browse the [7 Examples](../examples/basic-client-server.md)
4. **Diving deep?** → Read the [Architecture & Design](../architecture/overview.md)

---

**Let's get started!** 🚀
