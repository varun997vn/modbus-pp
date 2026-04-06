---
sidebar_position: 5
---

# Module Tour

A guided overview of what each module does and when to use it.

## Core Module (`core/`)

**The foundation.** Used by everything else.

- **types.hpp** — Primitives: `byte_t`, `register_t`, `unit_id_t`, and `StrongType<Tag, T>`
- **error.hpp** — 25+ error codes + `std::error_category` integration
- **result.hpp** — `Result<T>` monadic error type
- **pdu.hpp** — PDU (Protocol Data Unit) serialization/deserialization
- **endian.hpp** — 4 byte order conversions (ABCD, DCBA, BADC, CDAB)
- **timestamp.hpp** — Microsecond timestamps + stamped values
- **function_codes.hpp** — Modbus function code enums (FC 0x01–0x10, 0x2B, 0x6E)

**When to use:** Always. Core types in every modbus_pp call.

**Key class:**

```cpp
`Result<T>`  // Holds either T or std::error_code
```

---

## Transport Module (`transport/`)

**Abstraction for physical communication** — TCP, RTU (serial), or loopback.

- **transport.hpp** — Abstract base class
- **tcp_transport.hpp** — POSIX sockets, MBAP framing
- **rtu_transport.hpp** — Serial (termios), CRC16 framing
- **loopback_transport.hpp** — In-process paired endpoints (testing)
- **frame_codec.hpp** — ADU wrapping (MBAP header or CRC16)

**When to use:** Pick one transport based on your deployment (TCP for networks, RTU for serial, Loopback for testing).

**Key interface:**

```cpp
class Transport {
    virtual Result<void> send(span_t<const byte_t> frame) = 0;
    virtual Result<std::vector<byte_t>> receive(milliseconds timeout) = 0;
    virtual Result<void> connect() = 0;
    virtual void disconnect() = 0;
};
```

---

## Register Map Module (`register_map/`)

**Compile-time type safety** for register access.

- **register_descriptor.hpp** — Define a single typed register (address, type, byte order)
- **register_map.hpp** — Bundle and validate multiple descriptors (no overlaps)
- **type_codec.hpp** — Encode/decode Float32, Int64, String, etc.
- **batch_request.hpp** — Aggregate multiple reads + auto-merge contiguous ranges

**When to use:** When you want type-safe register access (instead of raw uint16 arrays).

**Key pattern:**

```cpp
using Temp = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;
using Pressure = RegisterDescriptor<Float32, 0x0002, 2, ByteOrder::ABCD>;
using Map = RegisterMap<Temp, Pressure>;  // static_assert: no overlaps

auto temp = TypeCodec::decode<Temp>(data);  // Returns Result<float>
```

---

## Security Module (`security/`)

**HMAC-SHA256 authentication** with challenge-response handshake.

- **hmac_auth.hpp** — HMAC computation, nonce generation, constant-time verification
- **session.hpp** — Challenge-response handshake protocol
- **credential_store.hpp** — Store and retrieve shared secrets

**When to use:** When you need authentication (client/server verification, integrity checking).

**Key flow:**

```
Client → Server: nonce Nc
Server → Client: nonce Ns + HMAC(Nc||Ns, K)
Client → Server: HMAC(Ns||Nc, K)
Server → Client: session key
```

---

## Pipeline Module (`pipeline/`)

**Pipelined transactions** — multiple requests in flight simultaneously.

- **pipeline.hpp** — Main pipelining coordinator
- **correlation_id.hpp** — Atomic ID generator
- **request_queue.hpp** — Thread-safe in-flight request tracking

**When to use:** When you have high-latency links (satellite, cellular) and need throughput.

**Benefit:** 8x speedup for 16 requests over 10ms-latency link (160ms → 20ms).

**Key method:**

```cpp
Result<CorrelationID> submit(PDU request, callback);
Result<PDU> submit_sync(PDU request, timeout);
```

---

## Pub/Sub Module (`pubsub/`)

**Event-driven push notifications** with dead-band filtering.

- **subscription.hpp** — Trigger modes, dead-band parameters
- **publisher.hpp** — Server-side change detection + notification dispatch
- **subscriber.hpp** — Client-side event handling

**When to use:** When you want **push-based** updates instead of polling.

**Benefit:** Reduces bandwidth and latency (server pushes only on meaningful changes).

---

## Discovery Module (`discovery/`)

**Broadcast device scanning** with capability detection.

- **device_info.hpp** — Device metadata (vendor, model, capabilities)
- **scanner.hpp** — Parallel probing of unit addresses

**When to use:** When you need to auto-discover devices on a bus.

**Key method:**

```cpp
auto devices = scanner.scan();  // Returns vector<DeviceInfo>
```

---

## Client/Server Facades (`client/`)

**High-level user API** — compose all subsystems.

- **client.hpp** — Unified client interface (read/write, pipeline, auth, discovery)
- **server.hpp** — Unified server interface (handler registration, pub/sub)

**When to use:** Always (in user code). These are the primary APIs.

**Key methods (Client):**

```cpp
Result<vector<register_t>> read_holding_registers(unit_id, addr, count);
Result<void> write_multiple_registers(unit_id, addr, values);
CorrelationID submit_pipelined(pdu, callback);
```

**Key methods (Server):**

```cpp
void on_read_holding_registers(Handler);
void on_write_multiple_registers(Handler);
Publisher& publisher();  // For pub/sub
```

---

## Dependency Graph

```
                ┌──────────┐
                │ Client / │
                │ Server   │
                │ Facades  │
                └────┬─────┘
       ┌─────────────┼─────────────┬──────────┐
       │             │             │          │
   ┌───▼──┐      ┌──▼──┐      ┌──▼──┐   ┌──▼──┐
   │Pipeline   │PubSub│      │Security  │Discovery
   ├─────┤      ├─────┤      ├───────┤  ├─────┤
   │      ├─────┤Transport├──┤           │
   └──┬──┘      └──┬──┘      └───┬───┘  └──┬──┘
      │            │             │         │
      └────────────┴─────────────┴─────────┴──┐
                                              │
                                    ┌─────────▼────────┐
                                    │ Core (types,     │
                                    │ error, result,   │
                                    │ pdu, endian...)  │
                                    └──────────────────┘
```

**Every module depends on `Core`.** No circular dependencies. Minimal interdependencies between extensions.

---

## Recommended Module Order (for learning)

1. **Start here:** `core/` — understand types, errors, PDU structure
2. **Then pick a transport:** `transport/` — TCP or RTU or Loopback
3. **High-level API:** `client/` & `server/` — write your core logic
4. **Add features as needed:**
   - Type safety? → `register_map/`
   - Authentication? → `security/`
   - High throughput? → `pipeline/`
   - Push notifications? → `pubsub/`
   - Auto-discovery? → `discovery/`

---

**Next:** Ready to see code? Jump to [Examples](../examples/basic-client-server.md) or [Architecture](../architecture/overview.md).
