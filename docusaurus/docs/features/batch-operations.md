---
sidebar_position: 7
---

# Batch Operations

Aggregate multiple reads with automatic contiguous-range merging.

## The Problem

Reading multiple ranges requires separate requests:

```cpp
client.read(0x0000, 10);  // Request 1
client.read(0x000A, 5);   // Request 2
client.read(0x0014, 8);   // Request 3
// 3 round trips, even if ranges are contiguous!
```

## The Solution: Batch Operations

Aggregate reads and let modbus_pp **auto-merge** contiguous ranges:

```cpp
BatchRequest batch;
batch.add(0x0000, 10)     // Regs 0x0000–0x0009
     .add(0x000A, 5)      // Regs 0x000A–0x000E (contiguous!)
     .add(0x0014, 8);     // Regs 0x0014–0x001B

auto optimized = batch.optimized();  // Returns merged requests
// Now 2 requests instead of 3!
```

## Merging Rules

- **Contiguous ranges** (end of range A = start of range B) are merged
- **Same byte order** required for merging
- **Different types** can be mixed (UInt16, Float32, etc.)
- **Non-contiguous ranges** stay separate

## Benefits

- **Fewer round trips** — Reduced latency
- **Automatic** — Compiler/optimizer handles it
- **Type-aware** — Respects type and byte-order constraints

## Full Example

See [Batch Operations Feature](./batch-operations.md) or [Typed Registers Example](../examples/typed-registers-example.md).

---

**Next:** back to [Features Overview](./overview.md) or [Byte-Order Safety](./byte-order-safety.md)
