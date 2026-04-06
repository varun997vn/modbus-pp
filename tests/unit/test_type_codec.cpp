#include <gtest/gtest.h>

#include <modbus_pp/register_map/type_codec.hpp>

#include <cmath>
#include <limits>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// Float encode/decode
// ---------------------------------------------------------------------------

TEST(TypeCodec, FloatRoundtrip) {
    float value = 3.14f;
    auto regs = TypeCodec::encode_float(value);
    EXPECT_EQ(regs.size(), 2u);
    EXPECT_FLOAT_EQ(TypeCodec::decode_float(regs.data()), value);
}

TEST(TypeCodec, FloatAllByteOrders) {
    float value = -273.15f;
    EXPECT_FLOAT_EQ(TypeCodec::decode_float<ByteOrder::ABCD>(
        TypeCodec::encode_float<ByteOrder::ABCD>(value).data()), value);
    EXPECT_FLOAT_EQ(TypeCodec::decode_float<ByteOrder::DCBA>(
        TypeCodec::encode_float<ByteOrder::DCBA>(value).data()), value);
    EXPECT_FLOAT_EQ(TypeCodec::decode_float<ByteOrder::BADC>(
        TypeCodec::encode_float<ByteOrder::BADC>(value).data()), value);
    EXPECT_FLOAT_EQ(TypeCodec::decode_float<ByteOrder::CDAB>(
        TypeCodec::encode_float<ByteOrder::CDAB>(value).data()), value);
}

TEST(TypeCodec, FloatZero) {
    auto regs = TypeCodec::encode_float(0.0f);
    EXPECT_FLOAT_EQ(TypeCodec::decode_float(regs.data()), 0.0f);
}

TEST(TypeCodec, FloatNegative) {
    auto regs = TypeCodec::encode_float(-42.5f);
    EXPECT_FLOAT_EQ(TypeCodec::decode_float(regs.data()), -42.5f);
}

// ---------------------------------------------------------------------------
// Double encode/decode
// ---------------------------------------------------------------------------

TEST(TypeCodec, DoubleRoundtrip) {
    double value = 2.718281828459045;
    auto regs = TypeCodec::encode_double(value);
    EXPECT_EQ(regs.size(), 4u);
    EXPECT_DOUBLE_EQ(TypeCodec::decode_double(regs.data()), value);
}

TEST(TypeCodec, DoubleAllByteOrders) {
    double value = -123456.789;
    EXPECT_DOUBLE_EQ(TypeCodec::decode_double<ByteOrder::ABCD>(
        TypeCodec::encode_double<ByteOrder::ABCD>(value).data()), value);
    EXPECT_DOUBLE_EQ(TypeCodec::decode_double<ByteOrder::DCBA>(
        TypeCodec::encode_double<ByteOrder::DCBA>(value).data()), value);
    EXPECT_DOUBLE_EQ(TypeCodec::decode_double<ByteOrder::BADC>(
        TypeCodec::encode_double<ByteOrder::BADC>(value).data()), value);
    EXPECT_DOUBLE_EQ(TypeCodec::decode_double<ByteOrder::CDAB>(
        TypeCodec::encode_double<ByteOrder::CDAB>(value).data()), value);
}

// ---------------------------------------------------------------------------
// UInt32 / Int32
// ---------------------------------------------------------------------------

TEST(TypeCodec, Uint32Roundtrip) {
    std::uint32_t value = 0xDEADBEEF;
    auto regs = TypeCodec::encode_uint32(value);
    EXPECT_EQ(TypeCodec::decode_uint32(regs.data()), value);
}

TEST(TypeCodec, Int32Roundtrip) {
    std::int32_t value = -123456;
    auto regs = TypeCodec::encode_int32(value);
    EXPECT_EQ(TypeCodec::decode_int32(regs.data()), value);
}

TEST(TypeCodec, Int32AllByteOrders) {
    std::int32_t value = -999;
    EXPECT_EQ(TypeCodec::decode_int32<ByteOrder::ABCD>(
        TypeCodec::encode_int32<ByteOrder::ABCD>(value).data()), value);
    EXPECT_EQ(TypeCodec::decode_int32<ByteOrder::DCBA>(
        TypeCodec::encode_int32<ByteOrder::DCBA>(value).data()), value);
    EXPECT_EQ(TypeCodec::decode_int32<ByteOrder::BADC>(
        TypeCodec::encode_int32<ByteOrder::BADC>(value).data()), value);
    EXPECT_EQ(TypeCodec::decode_int32<ByteOrder::CDAB>(
        TypeCodec::encode_int32<ByteOrder::CDAB>(value).data()), value);
}

// ---------------------------------------------------------------------------
// String encode/decode
// ---------------------------------------------------------------------------

TEST(TypeCodec, StringRoundtrip) {
    std::string str = "Hello";
    auto regs = TypeCodec::encode_string(str, 4);

    // "He" "ll" "o\0" → 3 registers used, 4th is zero
    auto decoded = TypeCodec::decode_string(regs.data(), 4);
    EXPECT_EQ(decoded, "Hello");
}

TEST(TypeCodec, StringEmpty) {
    auto regs = TypeCodec::encode_string("", 2);
    auto decoded = TypeCodec::decode_string(regs.data(), 2);
    EXPECT_EQ(decoded, "");
}

TEST(TypeCodec, StringTruncation) {
    // Max 2 registers = 4 chars max
    std::string str = "ABCDEF";
    auto regs = TypeCodec::encode_string(str, 2);
    auto decoded = TypeCodec::decode_string(regs.data(), 2);
    EXPECT_EQ(decoded, "ABCD");
}

TEST(TypeCodec, StringExactFit) {
    std::string str = "AB";
    auto regs = TypeCodec::encode_string(str, 1);
    auto decoded = TypeCodec::decode_string(regs.data(), 1);
    EXPECT_EQ(decoded, "AB");
}
