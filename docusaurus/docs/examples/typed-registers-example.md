---
sidebar_position: 2
---

# Typed Registers

Compile-time type-safe register access with zero runtime overhead.

## Overview

This example demonstrates:

- Defining typed register descriptors
- Bundling descriptors into a register map
- Compile-time overlap detection
- Type-safe encoding/decoding

## See the Code

Full source: [`examples/02_typed_registers.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/02_typed_registers.cpp)

```cpp
using Temperature = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;
using Pressure = RegisterDescriptor<Float32, 0x0002, 2, ByteOrder::ABCD>;
using SensorMap = RegisterMap<Temperature, Pressure>;

auto temp = TypeCodec::decode<Temperature>(regs.data());
```

## Key Learnings

- `RegisterDescriptor<Type, Address, Count, ByteOrder>` defines a typed register
- `RegisterMap<...>` validates no overlaps at compile time
- `TypeCodec::decode<Descriptor>()` safely converts raw registers to typed values
- **Zero runtime cost** — all type information is compile-time

---

Next example: [Pipeline Throughput](./pipeline-throughput.md)
