/// @file 07_endian_byte_orders.cpp
/// @brief Byte order encoding across all 4 Modbus conventions.
///
/// Shows how the same float32 value (123.456f) is encoded differently
/// depending on the byte order convention used by the device vendor.
/// Demonstrates round-trip correctness for each order.
///
/// Modbus disadvantage addressed: the Modbus specification defines 16-bit
/// registers but says nothing about how multi-register values (floats,
/// int32, etc.) should be byte-ordered. Different vendors use different
/// conventions, leading to silent data corruption when the wrong order
/// is assumed. modbus_pp makes byte order explicit and type-safe.

#include "modbus_pp/modbus_pp.hpp"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

/// Print two registers as hex, then show their individual bytes.
static void print_register_pair(const char* label, const std::array<reg_t, 2>& regs) {
    // Extract individual bytes
    byte_t b0 = static_cast<byte_t>(regs[0] >> 8);
    byte_t b1 = static_cast<byte_t>(regs[0] & 0xFF);
    byte_t b2 = static_cast<byte_t>(regs[1] >> 8);
    byte_t b3 = static_cast<byte_t>(regs[1] & 0xFF);

    std::cout << "  " << std::setw(5) << label << ": "
              << "regs=[0x" << std::hex << std::setfill('0')
              << std::setw(4) << regs[0] << ", 0x"
              << std::setw(4) << regs[1] << "]"
              << "  bytes=["
              << std::setw(2) << static_cast<int>(b0) << " "
              << std::setw(2) << static_cast<int>(b1) << " "
              << std::setw(2) << static_cast<int>(b2) << " "
              << std::setw(2) << static_cast<int>(b3) << "]"
              << std::dec << std::setfill(' ') << "\n";
}

int main() {
    std::cout << "=== modbus_pp Example 07: Endian Byte Orders ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Show the IEEE 754 representation
    // -----------------------------------------------------------------------
    float value = 123.456f;
    std::uint32_t raw_bits;
    std::memcpy(&raw_bits, &value, sizeof(raw_bits));

    std::cout << "--- IEEE 754 Representation ---\n";
    std::cout << "  Value:      " << value << "f\n";
    std::cout << "  Raw bits:   0x" << std::hex << std::setfill('0')
              << std::setw(8) << raw_bits << std::dec << std::setfill(' ') << "\n";

    byte_t a = static_cast<byte_t>((raw_bits >> 24) & 0xFF);
    byte_t b = static_cast<byte_t>((raw_bits >> 16) & 0xFF);
    byte_t c = static_cast<byte_t>((raw_bits >>  8) & 0xFF);
    byte_t d = static_cast<byte_t>((raw_bits >>  0) & 0xFF);

    std::cout << "  Bytes:      A=0x" << std::hex << std::setfill('0')
              << std::setw(2) << static_cast<int>(a)
              << " B=0x" << std::setw(2) << static_cast<int>(b)
              << " C=0x" << std::setw(2) << static_cast<int>(c)
              << " D=0x" << std::setw(2) << static_cast<int>(d)
              << std::dec << std::setfill(' ')
              << "  (A=MSB, D=LSB)\n\n";

    // -----------------------------------------------------------------------
    // 2. Encode with all 4 byte orders
    // -----------------------------------------------------------------------
    std::cout << "--- Encoding 123.456f Across All Byte Orders ---\n";
    std::cout << "  Each order places the 4 bytes into 2 Modbus registers differently:\n\n";

    auto abcd = TypeCodec::encode_float<ByteOrder::ABCD>(value);
    auto dcba = TypeCodec::encode_float<ByteOrder::DCBA>(value);
    auto badc = TypeCodec::encode_float<ByteOrder::BADC>(value);
    auto cdab = TypeCodec::encode_float<ByteOrder::CDAB>(value);

    std::cout << "  Order   Byte arrangement     Registers & raw bytes\n";
    std::cout << "  -----   ------------------   --------------------------------\n";
    std::cout << "  ABCD    [A B] [C D]         ";
    print_register_pair("ABCD", abcd);
    std::cout << "  DCBA    [D C] [B A]         ";
    print_register_pair("DCBA", dcba);
    std::cout << "  BADC    [B A] [D C]         ";
    print_register_pair("BADC", badc);
    std::cout << "  CDAB    [C D] [A B]         ";
    print_register_pair("CDAB", cdab);

    // -----------------------------------------------------------------------
    // 3. Common vendor conventions
    // -----------------------------------------------------------------------
    std::cout << "\n--- Vendor Byte Order Conventions ---\n";
    std::cout << "  ABCD (big-endian):       Modbus default, Siemens, Honeywell, Emerson\n";
    std::cout << "  DCBA (little-endian):    Some Modicon/Schneider legacy devices\n";
    std::cout << "  BADC (byte-swapped):     Schneider Electric, ABB, Daniel flow meters\n";
    std::cout << "  CDAB (word-swapped):     Enron Modbus, some Yokogawa devices\n\n";

    // -----------------------------------------------------------------------
    // 4. Round-trip correctness for each order
    // -----------------------------------------------------------------------
    std::cout << "--- Round-Trip Verification ---\n";

    float dec_abcd = TypeCodec::decode_float<ByteOrder::ABCD>(abcd.data());
    float dec_dcba = TypeCodec::decode_float<ByteOrder::DCBA>(dcba.data());
    float dec_badc = TypeCodec::decode_float<ByteOrder::BADC>(badc.data());
    float dec_cdab = TypeCodec::decode_float<ByteOrder::CDAB>(cdab.data());

    auto check = [value](const char* name, float decoded) {
        bool ok = (decoded == value);
        std::cout << "  " << std::setw(5) << name << ": decode -> " << decoded
                  << (ok ? "  [OK]" : "  [MISMATCH]") << "\n";
    };

    check("ABCD", dec_abcd);
    check("DCBA", dec_dcba);
    check("BADC", dec_badc);
    check("CDAB", dec_cdab);

    // -----------------------------------------------------------------------
    // 5. Show what happens when you use the WRONG byte order
    // -----------------------------------------------------------------------
    std::cout << "\n--- What Happens With the Wrong Byte Order ---\n";
    std::cout << "  Encoding with ABCD, but decoding with each other order:\n\n";

    float wrong_dcba = TypeCodec::decode_float<ByteOrder::DCBA>(abcd.data());
    float wrong_badc = TypeCodec::decode_float<ByteOrder::BADC>(abcd.data());
    float wrong_cdab = TypeCodec::decode_float<ByteOrder::CDAB>(abcd.data());

    std::cout << "  Correct (ABCD -> ABCD): " << dec_abcd << "\n";
    std::cout << "  Wrong   (ABCD -> DCBA): " << wrong_dcba << "\n";
    std::cout << "  Wrong   (ABCD -> BADC): " << wrong_badc << "\n";
    std::cout << "  Wrong   (ABCD -> CDAB): " << wrong_cdab << "\n\n";

    std::cout << "  These silently wrong values are one of the most common\n"
              << "  sources of bugs in Modbus integration projects. The\n"
              << "  RegisterDescriptor type system encodes the correct byte\n"
              << "  order at compile time, making this class of bug impossible.\n\n";

    // -----------------------------------------------------------------------
    // 6. Also demonstrate uint32 and double byte ordering
    // -----------------------------------------------------------------------
    std::cout << "--- UInt32 Byte Order Demo (value = 305419896 = 0x12345678) ---\n";
    std::uint32_t u32_val = 0x12345678;

    auto u32_abcd = TypeCodec::encode_uint32<ByteOrder::ABCD>(u32_val);
    auto u32_dcba = TypeCodec::encode_uint32<ByteOrder::DCBA>(u32_val);
    auto u32_badc = TypeCodec::encode_uint32<ByteOrder::BADC>(u32_val);
    auto u32_cdab = TypeCodec::encode_uint32<ByteOrder::CDAB>(u32_val);

    print_register_pair("ABCD", u32_abcd);
    print_register_pair("DCBA", u32_dcba);
    print_register_pair("BADC", u32_badc);
    print_register_pair("CDAB", u32_cdab);

    // Verify round-trip
    std::cout << "\n  Round-trip: "
              << "ABCD=" << std::hex << TypeCodec::decode_uint32<ByteOrder::ABCD>(u32_abcd.data())
              << " DCBA=" << TypeCodec::decode_uint32<ByteOrder::DCBA>(u32_dcba.data())
              << " BADC=" << TypeCodec::decode_uint32<ByteOrder::BADC>(u32_badc.data())
              << " CDAB=" << TypeCodec::decode_uint32<ByteOrder::CDAB>(u32_cdab.data())
              << std::dec << " (all should be 0x12345678)\n\n";

    // -----------------------------------------------------------------------
    // 7. Double (8 bytes = 4 registers) with ABCD order
    // -----------------------------------------------------------------------
    std::cout << "--- Float64 (double) Demo (value = 3.14159265358979) ---\n";
    double dbl_val = 3.14159265358979;
    auto dbl_regs = TypeCodec::encode_double<ByteOrder::ABCD>(dbl_val);

    std::cout << "  ABCD regs: [";
    for (std::size_t i = 0; i < dbl_regs.size(); ++i) {
        std::cout << "0x" << std::hex << std::setfill('0')
                  << std::setw(4) << dbl_regs[i] << std::dec << std::setfill(' ');
        if (i + 1 < dbl_regs.size()) std::cout << ", ";
    }
    std::cout << "]\n";

    double dbl_decoded = TypeCodec::decode_double<ByteOrder::ABCD>(dbl_regs.data());
    std::cout << "  Decoded:   " << std::setprecision(14) << dbl_decoded << "\n\n";

    std::cout << "[done] Endian byte orders example complete.\n";
    return 0;
}
