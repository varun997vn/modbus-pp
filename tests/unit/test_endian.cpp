#include <gtest/gtest.h>

#include <modbus_pp/core/endian.hpp>

#include <cmath>
#include <limits>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// Float round-trip tests across all byte orders
// ---------------------------------------------------------------------------

template <ByteOrder Order>
void test_float_roundtrip(float value) {
    auto encoded = encode_float<Order>(value);
    float decoded = decode_float<Order>(encoded.data());
    if (std::isnan(value)) {
        EXPECT_TRUE(std::isnan(decoded));
    } else {
        EXPECT_FLOAT_EQ(decoded, value);
    }
}

TEST(Endian, FloatRoundtrip_ABCD) {
    test_float_roundtrip<ByteOrder::ABCD>(3.14f);
    test_float_roundtrip<ByteOrder::ABCD>(0.0f);
    test_float_roundtrip<ByteOrder::ABCD>(-1.0f);
    test_float_roundtrip<ByteOrder::ABCD>(std::numeric_limits<float>::max());
    test_float_roundtrip<ByteOrder::ABCD>(std::numeric_limits<float>::min());
}

TEST(Endian, FloatRoundtrip_DCBA) {
    test_float_roundtrip<ByteOrder::DCBA>(3.14f);
    test_float_roundtrip<ByteOrder::DCBA>(0.0f);
    test_float_roundtrip<ByteOrder::DCBA>(-42.5f);
}

TEST(Endian, FloatRoundtrip_BADC) {
    test_float_roundtrip<ByteOrder::BADC>(3.14f);
    test_float_roundtrip<ByteOrder::BADC>(123.456f);
}

TEST(Endian, FloatRoundtrip_CDAB) {
    test_float_roundtrip<ByteOrder::CDAB>(3.14f);
    test_float_roundtrip<ByteOrder::CDAB>(-0.001f);
}

// ---------------------------------------------------------------------------
// Known float encoding values (IEEE 754: 3.14f = 0x4048F5C3)
// ---------------------------------------------------------------------------

TEST(Endian, FloatKnownValue_ABCD) {
    // 3.14f = 0x4048F5C3 → bytes: 40 48 F5 C3
    auto bytes = encode_float<ByteOrder::ABCD>(3.14f);
    EXPECT_EQ(bytes[0], 0x40);
    EXPECT_EQ(bytes[1], 0x48);
    EXPECT_EQ(bytes[2], 0xF5);
    EXPECT_EQ(bytes[3], 0xC3);
}

TEST(Endian, FloatKnownValue_DCBA) {
    auto bytes = encode_float<ByteOrder::DCBA>(3.14f);
    EXPECT_EQ(bytes[0], 0xC3);
    EXPECT_EQ(bytes[1], 0xF5);
    EXPECT_EQ(bytes[2], 0x48);
    EXPECT_EQ(bytes[3], 0x40);
}

TEST(Endian, FloatKnownValue_BADC) {
    auto bytes = encode_float<ByteOrder::BADC>(3.14f);
    EXPECT_EQ(bytes[0], 0x48);
    EXPECT_EQ(bytes[1], 0x40);
    EXPECT_EQ(bytes[2], 0xC3);
    EXPECT_EQ(bytes[3], 0xF5);
}

TEST(Endian, FloatKnownValue_CDAB) {
    auto bytes = encode_float<ByteOrder::CDAB>(3.14f);
    EXPECT_EQ(bytes[0], 0xF5);
    EXPECT_EQ(bytes[1], 0xC3);
    EXPECT_EQ(bytes[2], 0x40);
    EXPECT_EQ(bytes[3], 0x48);
}

// ---------------------------------------------------------------------------
// Double round-trip tests
// ---------------------------------------------------------------------------

template <ByteOrder Order>
void test_double_roundtrip(double value) {
    auto encoded = encode_double<Order>(value);
    double decoded = decode_double<Order>(encoded.data());
    EXPECT_DOUBLE_EQ(decoded, value);
}

TEST(Endian, DoubleRoundtrip) {
    test_double_roundtrip<ByteOrder::ABCD>(3.14159265358979);
    test_double_roundtrip<ByteOrder::DCBA>(3.14159265358979);
    test_double_roundtrip<ByteOrder::BADC>(3.14159265358979);
    test_double_roundtrip<ByteOrder::CDAB>(3.14159265358979);
}

// ---------------------------------------------------------------------------
// uint32 round-trip tests
// ---------------------------------------------------------------------------

TEST(Endian, Uint32Roundtrip) {
    std::uint32_t val = 0xDEADBEEF;

    EXPECT_EQ(decode_uint32<ByteOrder::ABCD>(encode_uint32<ByteOrder::ABCD>(val).data()), val);
    EXPECT_EQ(decode_uint32<ByteOrder::DCBA>(encode_uint32<ByteOrder::DCBA>(val).data()), val);
    EXPECT_EQ(decode_uint32<ByteOrder::BADC>(encode_uint32<ByteOrder::BADC>(val).data()), val);
    EXPECT_EQ(decode_uint32<ByteOrder::CDAB>(encode_uint32<ByteOrder::CDAB>(val).data()), val);
}

TEST(Endian, Uint32KnownValue_ABCD) {
    auto bytes = encode_uint32<ByteOrder::ABCD>(0x12345678u);
    EXPECT_EQ(bytes[0], 0x12);
    EXPECT_EQ(bytes[1], 0x34);
    EXPECT_EQ(bytes[2], 0x56);
    EXPECT_EQ(bytes[3], 0x78);
}

// ---------------------------------------------------------------------------
// int32 round-trip
// ---------------------------------------------------------------------------

TEST(Endian, Int32Roundtrip) {
    std::int32_t val = -12345;
    EXPECT_EQ(decode_int32<ByteOrder::ABCD>(encode_int32<ByteOrder::ABCD>(val).data()), val);
    EXPECT_EQ(decode_int32<ByteOrder::DCBA>(encode_int32<ByteOrder::DCBA>(val).data()), val);
}

// ---------------------------------------------------------------------------
// uint16 / int16 tests
// ---------------------------------------------------------------------------

TEST(Endian, Uint16) {
    auto bytes = encode_uint16(0x1234);
    EXPECT_EQ(bytes[0], 0x12);
    EXPECT_EQ(bytes[1], 0x34);
    EXPECT_EQ(decode_uint16(bytes.data()), 0x1234);
}

TEST(Endian, Int16) {
    std::int16_t val = -1;
    auto bytes = encode_int16(val);
    EXPECT_EQ(decode_int16(bytes.data()), val);
}

// ---------------------------------------------------------------------------
// Register-level encode/decode (float → 2 registers)
// ---------------------------------------------------------------------------

TEST(Endian, FloatRegsRoundtrip) {
    float value = 3.14f;

    auto regs = encode_float_regs<ByteOrder::ABCD>(value);
    EXPECT_EQ(regs.size(), 2u);
    float decoded = decode_float_regs<ByteOrder::ABCD>(regs.data());
    EXPECT_FLOAT_EQ(decoded, value);
}

TEST(Endian, FloatRegsAllOrders) {
    float value = -273.15f;

    EXPECT_FLOAT_EQ(decode_float_regs<ByteOrder::ABCD>(
        encode_float_regs<ByteOrder::ABCD>(value).data()), value);
    EXPECT_FLOAT_EQ(decode_float_regs<ByteOrder::DCBA>(
        encode_float_regs<ByteOrder::DCBA>(value).data()), value);
    EXPECT_FLOAT_EQ(decode_float_regs<ByteOrder::BADC>(
        encode_float_regs<ByteOrder::BADC>(value).data()), value);
    EXPECT_FLOAT_EQ(decode_float_regs<ByteOrder::CDAB>(
        encode_float_regs<ByteOrder::CDAB>(value).data()), value);
}

TEST(Endian, Uint32RegsRoundtrip) {
    std::uint32_t val = 0xCAFEBABE;
    auto regs = encode_uint32_regs<ByteOrder::ABCD>(val);
    EXPECT_EQ(decode_uint32_regs<ByteOrder::ABCD>(regs.data()), val);
}

TEST(Endian, DoubleRegsRoundtrip) {
    double value = 2.718281828459045;
    auto regs = encode_double_regs<ByteOrder::ABCD>(value);
    EXPECT_EQ(regs.size(), 4u);
    EXPECT_DOUBLE_EQ(decode_double_regs<ByteOrder::ABCD>(regs.data()), value);
}

// ---------------------------------------------------------------------------
// Different byte orders produce different encodings
// ---------------------------------------------------------------------------

TEST(Endian, DifferentOrdersDifferentBytes) {
    float value = 42.0f;
    auto abcd = encode_float<ByteOrder::ABCD>(value);
    auto dcba = encode_float<ByteOrder::DCBA>(value);
    auto badc = encode_float<ByteOrder::BADC>(value);
    auto cdab = encode_float<ByteOrder::CDAB>(value);

    // All should be different from each other (for non-palindromic values)
    EXPECT_NE(abcd, dcba);
    EXPECT_NE(abcd, badc);
    EXPECT_NE(abcd, cdab);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST(Endian, ZeroFloat) {
    auto bytes = encode_float<ByteOrder::ABCD>(0.0f);
    for (auto b : bytes) EXPECT_EQ(b, 0x00);
}

TEST(Endian, ZeroUint32) {
    auto bytes = encode_uint32<ByteOrder::ABCD>(0u);
    for (auto b : bytes) EXPECT_EQ(b, 0x00);
}
