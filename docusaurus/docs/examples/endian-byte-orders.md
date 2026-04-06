---
sidebar_position: 7
---

# Byte Order Safety & Endianness

Handling vendor-specific byte orderings (ABCD, DCBA, BADC, CDAB).

## Overview

Demonstrates:

- All 4 byte orders
- Compile-time order selection
- Zero runtime cost via constexpr if
- Multi-vendor device integration

## See the Code

Full source: [`examples/07_endian_byte_orders.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/07endian_byte_orders.cpp)

```cpp
// Each vendor uses a different byte order
using Siemens_Temp   = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::ABCD>;
using Schneider_Temp = RegisterDescriptor<Float32, 0x0000, 2, ByteOrder::BADC>;

// Decode each with correct order (compiler generates optimal code)
float t1 = TypeCodec::decode<Siemens_Temp>(regs);    // ABCD
float t2 = TypeCodec::decode<Schneider_Temp>(regs);  // BADC
```

---

**All 7 examples complete!** Choose your next path:

- [Features overview](../features/overview.md)
- [Architecture deep dive](../architecture/overview.md)
- [API Reference](../api/core-module.md)
