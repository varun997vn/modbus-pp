---
sidebar_position: 8
---

# Byte-Order Safety

Compile-time selection of 4 byte orders with zero runtime cost.

## The Problem

Different vendors use different multi-register byte orderings:

| Vendor             | Order                | 32-bit float 3.14 encoded as |
| ------------------ | -------------------- | ---------------------------- |
| Siemens, Honeywell | ABCD (big-endian)    | `0x4048F5C3`                 |
| Emerson            | DCBA (little-endian) | `0xC3F54840`                 |
| Schneider, ABB     | BADC (byte-swapped)  | `0x4840F5C3`                 |
| Daniel             | CDAB (word-swapped)  | `0xC35CF540`                 |

Get it wrong = **corrupted data** or crashes.

## The Solution: Compile-Time Selection

Define byte order in register descriptors:

```cpp
using Siemens_Temp   = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::ABCD>;
using Emerson_Temp   = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::DCBA>;
using Schneider_Temp = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;
```

The compiler **generates optimal byte-swap code** via `constexpr if`:

```cpp
// Compiles to a single BSWAP instruction (or similar)
float temp = TypeCodec::decode<Schneider_Temp>(regs);
```

## Supported Orders

All 4 vendors' orderings are supported:

| Order  | Meaning                          |
| ------ | -------------------------------- |
| `ABCD` | Big-endian (Modbus spec default) |
| `DCBA` | Little-endian                    |
| `BADC` | Byte-swapped                     |
| `CDAB` | Word-swapped                     |

## Zero Runtime Cost

```cpp
// Compile-time dispatch via constexpr if
// Optimizes to direct byte-swap, no branches!
if constexpr (Order == ByteOrder::DCBA) {
    return {bytes[3], bytes[2], bytes[1], bytes[0]};
}
```

## Example: Multi-Vendor Integration

```cpp
// Mix devices with different byte orders
using Device1_Temp = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::ABCD>;
using Device2_Temp = RegisterDescriptor<Float32, 0x0100, 2, ByteOrder::BADC>;

// Register map with both
using MultiVendorMap = RegisterMap<Device1_Temp, Device2_Temp>;
// static_assert ensures no overlaps!

// Decode each with correct byte order
auto t1 = TypeCodec::decode<Device1_Temp>(regs);  // ABCD
auto t2 = TypeCodec::decode<Device2_Temp>(regs);  // BADC
// Both correct, zero runtime cost!
```

## Full Example

See [Byte Order Safety Example](../examples/endian-byte-orders.md).

---

**Next:** [Back to Features](./overview.md) or [Architecture](../architecture/overview.md)
