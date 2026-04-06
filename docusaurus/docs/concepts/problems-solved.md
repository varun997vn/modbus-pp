---
sidebar_position: 3
---

# The 10 Problems Solved

Let's dive deep into each limitation and how modbus_pp addresses it.

## 1. No Authentication

### The Problem

Standard Modbus has **zero security**. Any device on a network can:

- Impersonate any slave address
- Read any register
- Write arbitrary data
- Perform denial-of-service attacks

### The Solution: HMAC-SHA256 Authentication

modbus_pp adds optional **HMAC-SHA256 authentication** with a challenge-response handshake:

```
1. Client sends nonce Nc
2. Server responds with nonce Ns + HMAC(Nc||Ns, shared_key)
3. Client proves knowledge of key with HMAC(Ns||Nc, shared_key)
4. Session established with derived session key
```

**Key benefits:**

- Mutual authentication (both sides prove they know the shared secret)
- No password sent over the wire
- Constant-time comparison prevents timing attacks
- Standard **OpenSSL HMAC-SHA256**

**See also:** [Security Feature Deep Dive](../features/security-hmac.md)

---

## 2. Weak Data Types

### The Problem

Standard Modbus treats everything as `uint16`. To use Float32, you manually:

- Allocate 2 registers
- Select a byte order (big-endian? little-endian? byte-swapped?)
- Write encoding/decoding functions
- Remember what each register is

This is **error-prone and repetitive**.

### The Solution: Compile-Time Typed Registers

Define a register map with typed descriptors:

```cpp
using Temperature = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
using Pressure    = RegisterDescriptor<RegisterType::Float32, 0x0002, 2, ByteOrder::ABCD>;
using Status      = RegisterDescriptor<RegisterType::UInt16,  0x0004>;

using SensorMap = RegisterMap<Temperature, Pressure, Status>;
// static_assert checks for overlaps at compile time!
```

Then read/write with type safety:

```cpp
auto result = codec.decode<Temperature>(registers.data());
// Returns Either<float, error>
```

**Key benefits:**

- Overlaps detected at compile time (static_assert)
- Type information preserved
- Auto-generated encoding/decoding
- Zero runtime overhead

**See also:** [Typed Registers Feature](../features/typed-registers.md)

---

## 3. Poll-Based Only

### The Problem

Standard Modbus is **pull-based**. Clients must repeatedly ask:

> "Has anything changed at address X?"

Even if nothing changes, you're sending requests and waiting for "no change" responses. Wastes bandwidth and adds latency.

### The Solution: Pub/Sub with Dead-Band Filtering

modbus_pp servers can **push notifications** when data changes:

```cpp
publisher.register_data_source(0x0000, 100, []() {
    return get_sensor_readings();
});

publisher.scan_and_notify();  // Call periodically
```

Clients subscribe and receive push notifications:

```cpp
subscriber.subscribe(0x0000, 100, SubscriptionMode::OnChange, dead_band=5);
// Server pushes update only if value changes by ≥5
```

**Key benefits:**

- Event-driven (low latency)
- Dead-band filtering reduces noise from sensors
- Backward compatible (standard clients still work)

**See also:** [Pub/Sub Feature](../features/pubsub.md)

---

## 4. 253-Byte Payload Limit

### The Problem

Standard Modbus limits NDU payloads to 253 bytes, which means:

- Reading large arrays requires many round trips
- Serial communication (RTU) becomes slow
- Frame overhead multiplied

### The Solution: Extended Payloads

modbus_pp extended frames use function code **0x6E** (reserved for user-defined operations):

```
┌──────┬───────┬───────┬─────────┬────────┬───────┬──────────┬─────────┐
│ 0x6E │ Ver   │ Flags │ CorrID  │ TS?    │ ExtFC │ PayLen   │ Payload │
└──────┴───────┴───────┴─────────┴────────┴───────┴──────────┴─────────┘
                                    ↓ optional
```

Payloads can be **kilobytes** instead of limited to 253 bytes.

**Key benefits:**

- Fewer round trips
- Backward compatible (standard devices ignore 0x6E)
- Structured header for multiple field types

---

## 5. No Timestamps

### The Problem

Standard Modbus frames carry **no timestamp**. When data arrives:

- Can't tell when it was sampled
- Debugging multi-register operations is hard
- Hard to detect message reordering

### The Solution: Microsecond Timestamps

Extended frames optionally include a **microsecond-precision timestamp**:

```cpp
struct Timestamp {
    uint64_t us;  // Microseconds since epoch
};

FrameHeader hdr;
hdr.timestamp = std::chrono::system_clock::now();
auto pdu = PDU::make_extended(hdr, payload);
```

**Key benefits:**

- Know exact timing of each operation
- Detect delays/reordering
- Time-series analysis of data streams

---

## 6. Sequential Throughput

### The Problem

Standard Modbus is strictly request-response:

```
Send Req1 → Wait for Resp1 → Send Req2 → Wait for Resp2 → ...
```

Over a 10ms-latency link, 16 sequential requests take **160ms**. Wasteful!

### The Solution: Pipelining with Correlation IDs

modbus_pp allows **multiple in-flight requests**:

```
Send Req1, Req2, ..., Req16 → Receive Resp1, Resp2, ..., Resp16
```

Each request is tagged with a **correlation ID**:

```cpp
auto id = pipeline.submit(request, [](Result<PDU> resp) {
    // Callback fired when response arrives
});
```

The pipeline matches responses to requests using correlation IDs.

**Key benefits:**

- **8x speedup** on high-latency links (160ms → 20ms for 16 reqs)
- Thread-safe request queue
- Automatic timeout cleanup

**See also:** [Pipelining Feature](../features/pipelining.md) & [Example](../examples/pipeline-throughput.md)

---

## 7. Poor Error Codes

### The Problem

Standard Modbus defines only **11 error codes**:

- 1 = Illegal Function
- 2 = Illegal Data Address
- ...
- 11 = Gateway Target Failed

**Not granular enough** for debugging.

### The Solution: Std::error_code Integration

modbus_pp extends with **25+ error codes** and integrates with C++17 `std::error_code`:

```cpp
enum class ErrorCode : int {
    // Standard (1-11)
    IllegalFunction = 1,
    IllegalDataAddress = 2,
    // ...

    // Extended (0x80+)
    AuthenticationFailed = 0x80,
    PipelineOverflow = 0x83,
    TimeoutExpired = 0x88,
};
```

Use with standard C++ error handling:

```cpp
Result<PDU> result = call_something();
if (!result) {
    auto error = result.error();
    std::cout << error.message();  // "Authentication failed"
    std::cout << error.category();  // "modbus_pp"
}
```

**Key benefits:**

- Standard C++ error handling
- Granular error information
- Easy logging and debugging

---

## 8. Manual Discovery

### The Problem

To find devices on a Modbus bus, you must **manually probe each address**:

```cpp
for (int addr = 1; addr <= 247; ++addr) {
    // Try to read from addr...maybe it exists?
}
```

Slow, unreliable, and you don't know device capabilities.

### The Solution: Broadcast Discovery Scanner

modbus_pp includes a **scanner** that discovers devices and their capabilities:

```cpp
Scanner scanner(transport, {
    .scan_timeout = 500ms,
    .range_start = 1,
    .range_end = 247
});

auto devices = scanner.scan();
for (auto& dev : devices) {
    std::cout << "Device " << dev.unit_id << ": "
              << dev.vendor_name << "\n";
}
```

**Key benefits:**

- Parallel probing (fast)
- Capability detection (standard vs. modbus_pp)
- Device metadata (vendor, model, version)

**See also:** [Discovery Feature](../features/discovery.md)

---

## 9. Batch Inflexibility

### The Problem

Reading multiple ranges requires separate requests:

```cpp
client.read(addr1, count1);  // Request 1
client.read(addr2, count2);  // Request 2
client.read(addr3, count3);  // Request 3
```

Even if addr2 = addr1 + count1 (contiguous), you need 3 requests.

### The Solution: Batch Operations with Auto-Merging

modbus_pp aggregates multiple reads and **auto-merges contiguous ranges**:

```cpp
BatchRequest batch;
batch.add(0x0000, 10)
     .add(0x000A, 5)   // Contiguous with previous!
     .add(0x0014, 8);

auto optimized = batch.optimized();  // Returns merged request(s)
```

**Key benefits:**

- Fewer round trips
- Auto-merge detects contiguity
- Type and byte-order aware

**See also:** [Batch Operations Feature](../features/batch-operations.md)

---

## 10. Byte Order Chaos

### The Problem

Different vendors use different byte orderings for multi-register values:

| Vendor    | Order | Example                  |
| --------- | ----- | ------------------------ |
| Siemens   | ABCD  | Big-endian (Modbus spec) |
| Emerson   | DCBA  | Little-endian            |
| Schneider | BADC  | Byte-swapped             |
| Daniel    | CDAB  | Word-swapped             |

You must remember which order **each device** uses. Easy to get wrong!

### The Solution: Compile-Time Byte Order Selection

Define byte order in register descriptors:

```cpp
using Temp_Siemens   = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::ABCD>;
using Temp_Schneider = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;
```

The compiler generates **optimal byte-swap code** for each order via `constexpr if`:

```cpp
// Compiles to a single bswap instruction for DCBA
auto f = TypeCodec::decode_float<ByteOrder::DCBA>(regs.data());
```

**Key benefits:**

- Zero runtime cost (compiles to direct byte swap)
- Type safe (can't mix orders by accident)
- All 4 orders supported
- Compiler optimizes away unused orderings

**See also:** [Byte Order Safety Feature](../features/byte-order-safety.md)

---

## Summary

| Problem             | Cost Without          | Benefit With modbus_pp             |
| ------------------- | --------------------- | ---------------------------------- |
| No auth             | Security breach       | HMAC-SHA256 + challenge-response   |
| Weak types          | Bugs                  | Compile-time safe register maps    |
| Poll-only           | Latency + bandwidth   | Push notifications + dead-band     |
| Size limit          | Many round trips      | Extended payloads (KiB)            |
| No timestamps       | Debug blind           | Microsecond precision in headers   |
| Sequential          | 160ms for 16 reqs     | 20ms with pipelining (8x)          |
| Poor errors         | Hard debugging        | 25+ error codes + std::error_code  |
| Manual discovery    | Unreliable scanning   | Auto-discovery + capabilities      |
| Batch inflexibility | Extra requests        | Auto-merge + optimization          |
| Byte order chaos    | Wrong interpretations | Compile-time selection + zero cost |

**All 10 can be used together, independently, or not at all.** Wire compatibility first! 🎯
