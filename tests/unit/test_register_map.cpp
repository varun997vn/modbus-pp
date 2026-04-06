#include <gtest/gtest.h>

#include <modbus_pp/register_map/register_map.hpp>

using namespace modbus_pp;

// Alias to avoid conflict with POSIX register_t
using reg_t = modbus_pp::register_t;

// ---------------------------------------------------------------------------
// Define a test register map
// ---------------------------------------------------------------------------

// Simulates a sensor device:
//   Address 0x0000: Temperature (float, BADC byte order) — 2 registers
//   Address 0x0002: Status code (uint16)                  — 1 register
//   Address 0x0003: Pressure (float64)                    — 4 registers
//   Address 0x0010: Device name (string, 8 registers)     — 8 registers

using Temperature = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
using StatusCode  = RegisterDescriptor<RegisterType::UInt16,  0x0002>;
using Pressure    = RegisterDescriptor<RegisterType::Float64, 0x0003>;
using DeviceName  = RegisterDescriptor<RegisterType::String,  0x0010, 8>;

using SensorMap = RegisterMap<Temperature, StatusCode, Pressure, DeviceName>;

// ---------------------------------------------------------------------------
// Compile-time checks
// ---------------------------------------------------------------------------

TEST(RegisterMap, DescriptorCount) {
    EXPECT_EQ(SensorMap::descriptor_count, 4u);
}

TEST(RegisterMap, TotalRegisters) {
    // 2 + 1 + 4 + 8 = 15
    EXPECT_EQ(SensorMap::total_registers, 15);
}

TEST(RegisterMap, DescriptorInfo) {
    auto info = SensorMap::descriptor_info();
    EXPECT_EQ(info.size(), 4u);

    EXPECT_EQ(info[0].address, 0x0000);
    EXPECT_EQ(info[0].count, 2);
    EXPECT_EQ(info[0].type, RegisterType::Float32);
    EXPECT_EQ(info[0].order, ByteOrder::BADC);

    EXPECT_EQ(info[1].address, 0x0002);
    EXPECT_EQ(info[1].count, 1);
    EXPECT_EQ(info[1].type, RegisterType::UInt16);

    EXPECT_EQ(info[2].address, 0x0003);
    EXPECT_EQ(info[2].count, 4);
    EXPECT_EQ(info[2].type, RegisterType::Float64);

    EXPECT_EQ(info[3].address, 0x0010);
    EXPECT_EQ(info[3].count, 8);
    EXPECT_EQ(info[3].type, RegisterType::String);
}

// ---------------------------------------------------------------------------
// Decode from flat register buffer
// ---------------------------------------------------------------------------

TEST(RegisterMap, DecodeFloat) {
    // Build a flat register buffer matching the map layout
    reg_t regs[15] = {};

    // Encode temperature at offset 0 (BADC byte order)
    float temp = 23.5f;
    auto temp_regs = TypeCodec::encode_float<ByteOrder::BADC>(temp);
    regs[0] = temp_regs[0];
    regs[1] = temp_regs[1];

    auto decoded = SensorMap::decode<0>(regs);
    EXPECT_FLOAT_EQ(decoded, 23.5f);
}

TEST(RegisterMap, DecodeUint16) {
    reg_t regs[15] = {};
    regs[2] = 42; // Status at offset 2

    auto decoded = SensorMap::decode<1>(regs);
    EXPECT_EQ(decoded, 42);
}

TEST(RegisterMap, DecodeFloat64) {
    reg_t regs[15] = {};

    double pressure = 101.325;
    auto p_regs = TypeCodec::encode_double(pressure);
    regs[3] = p_regs[0];
    regs[4] = p_regs[1];
    regs[5] = p_regs[2];
    regs[6] = p_regs[3];

    auto decoded = SensorMap::decode<2>(regs);
    EXPECT_DOUBLE_EQ(decoded, 101.325);
}

TEST(RegisterMap, DecodeString) {
    reg_t regs[15] = {};

    // Encode "Sensor01" at offset 7
    auto str_regs = TypeCodec::encode_string("Sensor01", 8);
    for (std::size_t i = 0; i < str_regs.size(); ++i) {
        regs[7 + i] = str_regs[i];
    }

    auto decoded = SensorMap::decode<3>(regs);
    EXPECT_EQ(decoded, "Sensor01");
}

// ---------------------------------------------------------------------------
// Encode into flat register buffer
// ---------------------------------------------------------------------------

TEST(RegisterMap, EncodeAndDecode) {
    reg_t regs[15] = {};

    SensorMap::encode<0>(23.5f, regs);
    SensorMap::encode<1>(static_cast<std::uint16_t>(42), regs);
    SensorMap::encode<2>(101.325, regs);
    SensorMap::encode<3>(std::string("Test"), regs);

    EXPECT_FLOAT_EQ(SensorMap::decode<0>(regs), 23.5f);
    EXPECT_EQ(SensorMap::decode<1>(regs), 42);
    EXPECT_DOUBLE_EQ(SensorMap::decode<2>(regs), 101.325);
    EXPECT_EQ(SensorMap::decode<3>(regs), "Test");
}

// ---------------------------------------------------------------------------
// Overlap detection (compile-time — these should compile successfully)
// ---------------------------------------------------------------------------

// This map has no overlaps — should compile
using ValidMap = RegisterMap<
    RegisterDescriptor<RegisterType::UInt16,  0x0000>,
    RegisterDescriptor<RegisterType::UInt32,  0x0001>,
    RegisterDescriptor<RegisterType::Float32, 0x0003>
>;

TEST(RegisterMap, ValidMapCompiles) {
    EXPECT_EQ(ValidMap::descriptor_count, 3u);
    // 1 + 2 + 2 = 5
    EXPECT_EQ(ValidMap::total_registers, 5);
}

// NOTE: Overlapping maps would cause a static_assert failure at compile time:
// using OverlappingMap = RegisterMap<
//     RegisterDescriptor<RegisterType::UInt32, 0x0000>,  // 0x0000-0x0001
//     RegisterDescriptor<RegisterType::UInt16, 0x0001>,  // 0x0001 — overlaps!
// >;
