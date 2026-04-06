/// @file 02_typed_registers.cpp
/// @brief Compile-time typed register system with RegisterMap.
///
/// Demonstrates how RegisterDescriptor and RegisterMap eliminate the most
/// common class of Modbus bugs: reading the wrong number of registers,
/// using the wrong byte order, or interpreting raw uint16 values as the
/// wrong data type.
///
/// This is a pure data-plane demo -- no transport is needed.
///
/// Modbus disadvantage addressed: standard Modbus has no type system.
/// Every value is just raw uint16 registers. Engineers must manually
/// track address, count, byte order, and type for every register.
/// modbus_pp encodes all of this at compile time.

#include "modbus_pp/modbus_pp.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

// ---------------------------------------------------------------------------
// Define a register map for a hypothetical multi-sensor device
// ---------------------------------------------------------------------------
// Address layout:
//   0x0000-0x0001  Temperature (float32, BADC byte order -- Schneider style)
//   0x0002-0x0003  Pressure    (float32, ABCD byte order -- standard)
//   0x0004         Status      (uint16)
//   0x0010-0x001F  Device name (string, 16 registers = 32 chars max)

using Temperature = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
using Pressure    = RegisterDescriptor<RegisterType::Float32, 0x0002, 2, ByteOrder::ABCD>;
using Status      = RegisterDescriptor<RegisterType::UInt16,  0x0004>;
using DeviceName  = RegisterDescriptor<RegisterType::String,  0x0010, 16>;

using SensorMap = RegisterMap<Temperature, Pressure, Status, DeviceName>;

// Compile-time constants exposed by the map
static_assert(SensorMap::descriptor_count == 4, "Expected 4 descriptors");
static_assert(SensorMap::total_registers == 2 + 2 + 1 + 16, "Expected 21 total registers");

// The overlap detector would fire at compile time if we accidentally
// overlapped address ranges. Try uncommenting this to see the error:
// using Bad = RegisterDescriptor<RegisterType::UInt16, 0x0001>;
// using BrokenMap = RegisterMap<Temperature, Bad>;  // static_assert fires!

int main() {
    std::cout << "=== modbus_pp Example 02: Typed Register System ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Show compile-time descriptor info
    // -----------------------------------------------------------------------
    std::cout << "--- Register Map Layout ---\n";
    auto info = SensorMap::descriptor_info();
    const char* names[] = {"Temperature", "Pressure", "Status", "DeviceName"};
    const char* type_names[] = {"UInt16", "Int16", "UInt32", "Int32",
                                "Float32", "Float64", "String"};
    const char* order_names[] = {"ABCD", "DCBA", "BADC", "CDAB"};

    for (std::size_t i = 0; i < info.size(); ++i) {
        std::cout << "  [" << i << "] " << std::setw(12) << std::left << names[i]
                  << "  addr=0x" << std::hex << std::setfill('0') << std::setw(4)
                  << info[i].address << std::dec << std::setfill(' ')
                  << "  count=" << info[i].count
                  << "  type=" << type_names[static_cast<int>(info[i].type)]
                  << "  order=" << order_names[static_cast<int>(info[i].order)]
                  << "\n";
    }
    std::cout << "  Total registers: " << SensorMap::total_registers << "\n\n";

    // -----------------------------------------------------------------------
    // 2. Encode values into a flat register buffer
    // -----------------------------------------------------------------------
    std::cout << "--- Encoding Values ---\n";
    std::vector<reg_t> buffer(SensorMap::total_registers, 0);

    // Encode each field by index -- the map knows the type, count, and order
    SensorMap::encode<0>(98.6f, buffer.data());           // Temperature
    SensorMap::encode<1>(1013.25f, buffer.data());        // Pressure
    SensorMap::encode<2>(static_cast<uint16_t>(0x0001), buffer.data()); // Status
    SensorMap::encode<3>(std::string("SLB-Sensor-42"), buffer.data());  // DeviceName

    std::cout << "  Encoded temperature  = 98.6 (BADC byte order)\n";
    std::cout << "  Encoded pressure     = 1013.25 (ABCD byte order)\n";
    std::cout << "  Encoded status       = 0x0001\n";
    std::cout << "  Encoded device name  = \"SLB-Sensor-42\"\n\n";

    // Print the raw register buffer
    std::cout << "--- Raw Register Buffer ---\n";
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        if (i == 0) std::cout << "  Temperature: ";
        else if (i == 2) std::cout << "  Pressure:    ";
        else if (i == 4) std::cout << "  Status:      ";
        else if (i == 5) std::cout << "  DeviceName:  ";

        if (i < 5 || i == 5) {
            std::cout << "0x" << std::hex << std::setfill('0')
                      << std::setw(4) << buffer[i];
        } else if (i >= 5 && i < 5 + 16) {
            std::cout << "0x" << std::hex << std::setfill('0')
                      << std::setw(4) << buffer[i];
        }

        if (i == 1 || i == 3 || i == 4) std::cout << "\n";
        else if (i >= 5 && (i - 5) % 8 == 7) std::cout << "\n              ";
        else std::cout << " ";
    }
    std::cout << std::dec << "\n\n";

    // -----------------------------------------------------------------------
    // 3. Decode values back (simulating receiving registers from a device)
    // -----------------------------------------------------------------------
    std::cout << "--- Decoding Values ---\n";

    // The decode<I> call returns the correct C++ type automatically:
    //   decode<0> -> float    (Temperature)
    //   decode<1> -> float    (Pressure)
    //   decode<2> -> uint16_t (Status)
    //   decode<3> -> string   (DeviceName)
    float temp = SensorMap::decode<0>(buffer.data());
    float pres = SensorMap::decode<1>(buffer.data());
    std::uint16_t status = SensorMap::decode<2>(buffer.data());
    std::string name = SensorMap::decode<3>(buffer.data());

    std::cout << "  Temperature: " << temp << " F\n";
    std::cout << "  Pressure:    " << pres << " hPa\n";
    std::cout << "  Status:      0x" << std::hex << std::setfill('0')
              << std::setw(4) << status << std::dec << "\n";
    std::cout << "  Device name: \"" << name << "\"\n\n";

    // -----------------------------------------------------------------------
    // 4. Demonstrate byte-order matters for floats
    // -----------------------------------------------------------------------
    std::cout << "--- Byte Order Comparison for Temperature (98.6) ---\n";
    auto abcd = TypeCodec::encode_float<ByteOrder::ABCD>(98.6f);
    auto badc = TypeCodec::encode_float<ByteOrder::BADC>(98.6f);
    auto cdab = TypeCodec::encode_float<ByteOrder::CDAB>(98.6f);
    auto dcba = TypeCodec::encode_float<ByteOrder::DCBA>(98.6f);

    auto print_regs = [](const char* label, const std::array<reg_t, 2>& r) {
        std::cout << "  " << std::setw(5) << label << ": [0x"
                  << std::hex << std::setfill('0')
                  << std::setw(4) << r[0] << ", 0x"
                  << std::setw(4) << r[1] << "]"
                  << std::dec << std::setfill(' ') << "\n";
    };

    print_regs("ABCD", abcd);
    print_regs("BADC", badc);
    print_regs("CDAB", cdab);
    print_regs("DCBA", dcba);

    std::cout << "\n  Without the type system, using the wrong byte order\n"
              << "  produces silently wrong values -- a classic Modbus bug.\n"
              << "  RegisterDescriptor encodes the correct order at compile time.\n\n";

    // -----------------------------------------------------------------------
    // 5. Show that the type system prevents misuse at compile time
    // -----------------------------------------------------------------------
    std::cout << "--- Compile-Time Safety ---\n"
              << "  SensorMap::decode<0> returns float   (Temperature)\n"
              << "  SensorMap::decode<1> returns float   (Pressure)\n"
              << "  SensorMap::decode<2> returns uint16_t (Status)\n"
              << "  SensorMap::decode<3> returns string   (DeviceName)\n\n"
              << "  Attempting to decode<0> as uint16_t is a compile error.\n"
              << "  Attempting to use an out-of-range index is a compile error.\n"
              << "  Overlapping address ranges trigger static_assert.\n\n";

    std::cout << "[done] Typed register system example complete.\n";
    return 0;
}
