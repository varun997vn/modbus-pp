---
sidebar_position: 1
---

# Core Concepts Overview

Before diving into code, let's establish a shared understanding of Modbus and what modbus_pp solves.

## What is Modbus?

**Modbus** is a 40-year-old industrial protocol (defined in 1979) for reading and writing data on devices. It:

- Uses **registers** (16-bit values) as the fundamental data unit
- Defines **function codes** (FC 01–16) for read/write operations
- Supports **TCP** (network) and **RTU** (serial) transports
- Has **no authentication** — any device can impersonate any address
- Is **vendor-neutral** — standard across the industry

### Why It's Still Used

- **Simple** — easy to implement
- **Ubiquitous** — every industrial device speaks Modbus
- **Reliable** — 40 years of battle-tested specs
- **Real-time** — low overhead for IoT/embedded

### Why It Falls Short

- Over-simplified for modern needs
- No type system (everything is raw uint16)
- No security built-in
- No event notifications (poll-only)
- Byte-order chaos across vendors

## The 10 Limitations & Solutions

| #   | Problem                    | modbus_pp Solution                               |
| --- | -------------------------- | ------------------------------------------------ |
| 1   | **No authentication**      | HMAC-SHA256 with challenge-response              |
| 2   | **Weak data types**        | Compile-time typed register descriptors          |
| 3   | **Poll-based only**        | Event-driven Pub/Sub with dead-band              |
| 4   | **253-byte payload limit** | Extended payloads (KC kilobytes)                 |
| 5   | **No timestamps**          | Microsecond-precision timestamps in frame header |
| 6   | **Sequential throughput**  | Pipelining with correlation IDs (8x faster)      |
| 7   | **Poor error codes**       | 25+ error codes + std::error_code integration    |
| 8   | **Manual discovery**       | Broadcast scanner with capability detection      |
| 9   | **Batch inflexibility**    | Automatic range merging for batch reads          |
| 10  | **Byte order chaos**       | 4 byte orders, compile-time selection            |

## Modbus_pp Design Philosophy

**Wire compatibility first** — Standard Modbus frames pass through unchanged. A modbus_pp client can talk to any standard Modbus device.

**Compile-time safety** — Zero runtime cost for features you don't use. Register overlaps caught by `static_assert`.

**Zero-overhead abstractions** — No exceptions in the hot path. Monadic error handling with `Result<T>`.

## Next Steps

Ready to go deeper?

- **[Modbus Fundamentals](./modbus-fundamentals.md)** — PDU, registers, function codes
- **[Problems Solved (Deep Dive)](./problems-solved.md)** — Each of the 10 in detail
- **[Module Tour](./module-tour.md)** — What each module does

Or jump straight to:

- **[Quick examples](../examples/basic-client-server.md)**
- **[Architecture overview](../architecture/overview.md)**
