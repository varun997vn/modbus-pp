---
sidebar_position: 4
---

# Design Philosophy

Three core principles guide modbus_pp:

## 1. Wire Compatibility First

**Standard Modbus frames pass through unchanged.**

A modbus_pp client can communicate with any standard Modbus device using standard function codes (0x01–0x10). Extended features only activate when **both endpoints support them**.

This means:

- You can add modbus_pp to existing systems incrementally
- Legacy devices continue to work
- No flag days or coordinated upgrades required
- Backward compatibility is **guaranteed at the protocol level**

## 2. Compile-Time Safety

**Zero runtime cost for features you don't use.**

- Register address overlaps are caught by `static_assert` (compile time, not runtime)
- Byte order selection compiles down to direct byte-swap instructions via `constexpr if`
- Strong type wrappers (`RegisterAddress` vs. `RegisterCount`) prevent accidental parameter mixing
- Unused abstractions are optimized away by the compiler

**Example:**

```cpp
using Temp = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;
using Bad  = RegisterDescriptor<Float32, 0x0001>;  // Overlaps!

using Map = RegisterMap<Temp, Bad>; // compile error: "overlapping address ranges"
```

This catches bugs **before runtime**, at **zero runtime cost**.

## 3. Zero-Overhead Abstractions

**You only pay for what you use.**

- `Result<T>` avoids exceptions in the data path (hot code path is exception-free)
- Template-based codecs generate optimal code per byte order at compile time
- Pipeline request queue is thread-safe but optional
- Pub/Sub is opt-in; poll-based code path is unchanged

**Example:**

```cpp
// This compiles to direct register access, no function call overhead:
auto f = TypeCodec::decode_float<ByteOrder::ABCD>(data);

// `Result<T>` is inlined; no exception unwinding needed:
if (auto result = operation()) {
    use(result.value());  // Fast path
} else {
    handle(result.error());  // Slow path (errors are rare)
}
```

---

## Corollaries

From these three principles emerge several corollaries:

### A. No Exceptions in the Data Path

All error handling uses `Result<T>` (an ADT, not exceptions). This means:

- No exception handling overhead in tight loops
- Predictable performance (no surprise allocations)
- Works in embedded/real-time systems with -fno-exceptions

### B. Minimal Dependencies

- **No Boost** — Uses only C++17 standard library + OpenSSL (for HMAC)
- **Linker-friendly** — Small code size (~4,300 lines)
- **Embedded-suitable** — ~4KiB headers, ~2KiB library code

### C. Prefer Composition Over Inheritance

Client and Server are high-level **facades** that compose all subsystems:

- No deep vtable hierarchies
- Easy to understand at a glance
- Each module is independently useful

### D. Monadic Error Handling

`Result<T>` supports chainable operations:

```cpp
parse_frame(data)
    .and_then([](PDU& p) { return validate(p); })
    .map([](PDU& p) { return p.payload(); })
    .value_or(default_value);
```

Clean, readable, exception-free error propagation.

---

## Impact on Design

These principles shaped every decision:

| Design Decision                    | Why?                                        |
| ---------------------------------- | ------------------------------------------- |
| `Result<T>` type                   | Zero-overhead + composable errors           |
| `constexpr if` for byte orders     | Compile-time dispatch → zero runtime cost   |
| `static_assert` for overlaps       | Fail fast at compile time, not runtime      |
| Loopback transport                 | Enable testing without mocking complexity   |
| Separate `Client`/`Server` facades | Easier to understand than monolithic class  |
| Optional fields in extended PDU    | Only pay for features you use               |
| Thread-safe request queue          | Enables pipelining without locking hot path |

---

**Next:** [Module Tour](./module-tour.md)
