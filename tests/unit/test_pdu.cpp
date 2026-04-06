#include <gtest/gtest.h>

#include <modbus_pp/core/pdu.hpp>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// Standard PDU
// ---------------------------------------------------------------------------

TEST(PDU, StandardConstruction) {
    auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                   {0x00, 0x10, 0x00, 0x04});
    EXPECT_TRUE(pdu.is_standard());
    EXPECT_FALSE(pdu.is_extended());
    EXPECT_EQ(pdu.function_code(), FunctionCode::ReadHoldingRegisters);
    EXPECT_EQ(pdu.payload().size(), 4u);
    EXPECT_EQ(pdu.payload()[0], 0x00);
    EXPECT_EQ(pdu.payload()[1], 0x10);
}

TEST(PDU, StandardSerializeRoundtrip) {
    std::vector<byte_t> data = {0x00, 0x10, 0x00, 0x04};
    auto original = PDU::make_standard(FunctionCode::ReadHoldingRegisters, data);

    auto bytes = original.serialize();
    EXPECT_EQ(bytes[0], static_cast<byte_t>(FunctionCode::ReadHoldingRegisters));
    EXPECT_EQ(bytes.size(), 5u); // FC(1) + data(4)

    auto result = PDU::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());

    auto& deserialized = result.value();
    EXPECT_TRUE(deserialized.is_standard());
    EXPECT_EQ(deserialized.function_code(), FunctionCode::ReadHoldingRegisters);
    EXPECT_EQ(deserialized.payload().size(), 4u);
    for (std::size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(deserialized.payload()[i], data[i]);
    }
}

TEST(PDU, StandardIsNotExtended) {
    std::vector<byte_t> bytes = {0x03, 0x00, 0x10, 0x00, 0x04};
    EXPECT_FALSE(PDU::is_extended_frame(span_t<const byte_t>{bytes}));
}

// ---------------------------------------------------------------------------
// Extended PDU
// ---------------------------------------------------------------------------

TEST(PDU, ExtendedConstruction) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
    hdr.correlation_id = 42;

    std::vector<byte_t> payload = {0x01, 0x02, 0x03};
    auto pdu = PDU::make_extended(hdr, payload);

    EXPECT_TRUE(pdu.is_extended());
    EXPECT_FALSE(pdu.is_standard());
    EXPECT_EQ(pdu.function_code(), FunctionCode::Extended);
    EXPECT_EQ(pdu.payload().size(), 3u);
    EXPECT_TRUE(pdu.extended_header().has_value());
    EXPECT_EQ(pdu.extended_header()->correlation_id, 42);
    EXPECT_EQ(pdu.extended_header()->ext_function_code, ExtendedFunctionCode::BatchRead);
}

TEST(PDU, ExtendedSerializeRoundtrip) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::Subscribe;
    hdr.correlation_id = 1234;

    std::vector<byte_t> payload = {0xAA, 0xBB, 0xCC, 0xDD};
    auto original = PDU::make_extended(hdr, payload);

    auto bytes = original.serialize();
    EXPECT_EQ(bytes[0], static_cast<byte_t>(FunctionCode::Extended));

    auto result = PDU::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());

    auto& deserialized = result.value();
    EXPECT_TRUE(deserialized.is_extended());
    EXPECT_EQ(deserialized.extended_header()->correlation_id, 1234);
    EXPECT_EQ(deserialized.extended_header()->ext_function_code,
              ExtendedFunctionCode::Subscribe);
    EXPECT_EQ(deserialized.payload().size(), 4u);
    EXPECT_EQ(deserialized.payload()[0], 0xAA);
}

TEST(PDU, ExtendedWithTimestamp) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::EventNotification;
    hdr.correlation_id = 99;
    hdr.timestamp = Timestamp::from_epoch_us(1'700'000'000'000'000LL);

    std::vector<byte_t> payload = {0x01};
    auto original = PDU::make_extended(hdr, payload);

    EXPECT_TRUE(has_flag(original.extended_header()->flags, FrameFlag::HasTimestamp));

    auto bytes = original.serialize();
    auto result = PDU::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());

    auto& d = result.value();
    ASSERT_TRUE(d.extended_header()->timestamp.has_value());
    EXPECT_EQ(d.extended_header()->timestamp->epoch_microseconds(),
              1'700'000'000'000'000LL);
}

TEST(PDU, ExtendedWithHMAC) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::AuthResponse;
    hdr.correlation_id = 7;

    HMACDigest digest;
    for (std::size_t i = 0; i < 32; ++i) {
        digest[i] = static_cast<byte_t>(i);
    }

    std::vector<byte_t> payload = {0xFF};
    auto original = PDU::make_extended(hdr, payload, digest);

    EXPECT_TRUE(original.hmac().has_value());
    EXPECT_TRUE(has_flag(original.extended_header()->flags, FrameFlag::HasHMAC));

    auto bytes = original.serialize();
    auto result = PDU::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());

    auto& d = result.value();
    ASSERT_TRUE(d.hmac().has_value());
    EXPECT_EQ(*d.hmac(), digest);
}

TEST(PDU, ExtendedIsDetected) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::Discover;
    auto pdu = PDU::make_extended(hdr, {});
    auto bytes = pdu.serialize();

    EXPECT_TRUE(PDU::is_extended_frame(span_t<const byte_t>{bytes}));
}

// ---------------------------------------------------------------------------
// Error cases
// ---------------------------------------------------------------------------

TEST(PDU, EmptyFrameFails) {
    auto result = PDU::deserialize(span_t<const byte_t>{});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::InvalidFrame));
}

TEST(PDU, TruncatedExtendedFails) {
    // Extended frame header needs at least 9 bytes
    std::vector<byte_t> truncated = {0x6E, 0x01, 0x00};
    auto result = PDU::deserialize(span_t<const byte_t>{truncated});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::InvalidFrame));
}

TEST(PDU, WrongVersionFails) {
    // Valid length but version = 2
    std::vector<byte_t> bad_ver = {0x6E, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
    auto result = PDU::deserialize(span_t<const byte_t>{bad_ver});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::UnsupportedVersion));
}

TEST(PDU, StandardEmptyPayload) {
    auto pdu = PDU::make_standard(FunctionCode::WriteSingleCoil, {});
    auto bytes = pdu.serialize();
    EXPECT_EQ(bytes.size(), 1u); // Just FC

    auto result = PDU::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().payload().size(), 0u);
}
