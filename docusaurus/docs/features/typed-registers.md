---
sidebar_position: 2
---

# Typed Registers

Type-safe register access with compile-time validation and zero runtime overhead.

## The Problem (Without modbus_pp)

```cpp
// Without types, you must manually track everything:
std::vector<uint16_t> regs(256);  // Raw registers

// Reading a Float32 at address 0x0000 (2 registers, BADC byte order):
uint16_t r0 = regs[0x0000];
uint16_t r1 = regs[0x0001];
// Manually encode according to BADC...
// What if you use the wrong byte order? Wrong address? Wrong count?
```

## The Solution

Define typed register **descriptors** at compile time:

```cpp
#include "modbus_pp/modbus_pp.hpp"
using namespace modbus_pp;

// Define typed register descriptors
using Temperature = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
using Pressure    = RegisterDescriptor<RegisterType::Float32, 0x0002, 2, ByteOrder::ABCD>;
using Status      = RegisterDescriptor<RegisterType::UInt16,  0x0004>;
using Name        = RegisterDescriptor<RegisterType::String,  0x0010, 16>;

// Bundle into a register map (compile-time validation!)
using SensorMap = RegisterMap<Temperature, Pressure, Status, Name>;
```

### Compile-Time Protection

```cpp
// This code fails to compile with overlap detection:
using Bad = RegisterDescriptor<RegisterType::UInt16, 0x0001>;  // Overlaps Temperature!
using BrokenMap = RegisterMap<Temperature, Bad>;
// error: Register descriptors have overlapping address ranges
```

### Type-Safe Decoding

```cpp
auto temperature = TypeCodec::decode<Temperature>(registers.data());
// Type: Result<float>
// Automatically handles:
// - Address (0x0000)
// - Count (2 registers)
// - Byte order (BADC)
// - Encoding (Float32)

if (temperature) {
    std::cout << "Temperature: " << temperature.value() << "°C\n";
} else {
    std::cerr << "Error: " << temperature.error().message() << "\n";
}
```

### Type-Safe Encoding

```cpp
float new_temp = 25.5f;
auto encoded = TypeCodec::encode<Temperature>(new_temp);
// Type: Result<array<uint16_t, 2>>

if (encoded) {
    // Write to device
    client.write_multiple_registers(unit, Temp::address, encoded.value());
}
```

## Supported Types

| Type      | Size        | Example                  |
| --------- | ----------- | ------------------------ |
| `UInt16`  | 1 register  | Temperature (raw counts) |
| `Int16`   | 1 register  | Signed sensor reading    |
| `UInt32`  | 2 registers | Large counter            |
| `Int32`   | 2 registers | Signed value             |
| `Float32` | 2 registers | Temperature in °C        |
| `Float64` | 4 registers | High-precision value     |
| `String`  | N registers | Device name, serial #    |

## Byte Orders

All 4 vendor-specific byte orders:

| Order  | Name          | Vendors            | Example                                   |
| ------ | ------------- | ------------------ | ----------------------------------------- |
| `ABCD` | Big-endian    | Siemens, Honeywell | `0x12345678` → `[0x12][0x34][0x56][0x78]` |
| `DCBA` | Little-endian | Some Emerson       | `0x12345678` → `[0x78][0x56][0x34][0x12]` |
| `BADC` | Byte-swapped  | Schneider, ABB     | `0x12345678` → `[0x34][0x12][0x78][0x56]` |
| `CDAB` | Word-swapped  | Daniel             | `0x12345678` → `[0x56][0x78][0x12][0x34]` |

**Zero runtime cost:** Byte order selection compiles to a single `bswap` instruction.

## Complete Example

```cpp
#include "modbus_pp/modbus_pp.hpp"
#include <iostream>
#include <vector>

using namespace modbus_pp;

// Define sensor register map
using TempF32 = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
using PressF32 = RegisterDescriptor<RegisterType::Float32, 0x0002, 2, ByteOrder::ABCD>;
using Status = RegisterDescriptor<RegisterType::UInt16, 0x0004>;

using SensorMap = RegisterMap<TempF32, PressF32, Status>;

int main() {
    // Simulated register data from device
    std::vector<register_t> registers = {
        0x4148,  // Temp high byte (25.1°C in BADC)
        0xF147,  // Temp low byte
        0x41A6,  // Pressure high (20.6 bar)
        0x0000,  // Pressure low
        0x0001   // Status (online)
    };

    // Decode with type safety
    auto temp = TypeCodec::decode<TempF32>(registers.data());
    auto press = TypeCodec::decode<PressF32>(registers.data() + 2);
    auto status = TypeCodec::decode<Status>(registers.data() + 4);

    if (temp && press && status) {
        std::cout << "Temperature: " << temp.value() << "°C\n";
        std::cout << "Pressure: " << press.value() << " bar\n";
        std::cout << "Status: " << status.value() << "\n";
    }

    return 0;
}
```

## See Also

- **[Example: Typed Registers](../examples/typed-registers-example.md)** — Full working example
- **[API Reference: Register Map](../api/register-map-module.md)**
- **[Byte Order Safety](./byte-order-safety.md)** — More on byte ordering
