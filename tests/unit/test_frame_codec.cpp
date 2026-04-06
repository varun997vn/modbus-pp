#include <gtest/gtest.h>

#include <modbus_pp/transport/frame_codec.hpp>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// CRC16 known test vectors
// ---------------------------------------------------------------------------

TEST(FrameCodec, CRC16EmptyData) {
    // CRC of empty data with initial value 0xFFFF
    std::vector<byte_t> empty;
    auto crc = FrameCodec::crc16(span_t<const byte_t>{empty});
    EXPECT_EQ(crc, 0xFFFF);
}

TEST(FrameCodec, CRC16KnownVector) {
    // Known Modbus CRC16 for {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A}
    // This is: Unit 1, Read Holding Registers, start addr 0, count 10
    std::vector<byte_t> data = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    auto crc = FrameCodec::crc16(span_t<const byte_t>{data});
    EXPECT_EQ(crc, 0xCDC5);
}

TEST(FrameCodec, CRC16SingleByte) {
    std::vector<byte_t> data = {0x00};
    auto crc = FrameCodec::crc16(span_t<const byte_t>{data});
    // CRC of single zero byte
    EXPECT_NE(crc, 0u);
}

// ---------------------------------------------------------------------------
// RTU framing
// ---------------------------------------------------------------------------

TEST(FrameCodec, RTURoundtrip) {
    auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                   {0x00, 0x10, 0x00, 0x04});
    unit_id_t unit = 1;

    auto frame = FrameCodec::wrap_rtu(unit, pdu);

    // Frame: [UnitID:1][FC:1][Data:4][CRC:2] = 8 bytes
    EXPECT_EQ(frame.size(), 8u);
    EXPECT_EQ(frame[0], 1); // Unit ID

    auto result = FrameCodec::unwrap_rtu(span_t<const byte_t>{frame});
    ASSERT_TRUE(result.has_value());

    auto& [decoded_unit, decoded_pdu] = result.value();
    EXPECT_EQ(decoded_unit, unit);
    EXPECT_EQ(decoded_pdu.function_code(), FunctionCode::ReadHoldingRegisters);
    EXPECT_EQ(decoded_pdu.payload().size(), 4u);
}

TEST(FrameCodec, RTUCorruptedCRC) {
    auto pdu = PDU::make_standard(FunctionCode::ReadCoils, {0x00, 0x00, 0x00, 0x08});
    auto frame = FrameCodec::wrap_rtu(1, pdu);

    // Corrupt a byte
    frame[3] ^= 0xFF;

    auto result = FrameCodec::unwrap_rtu(span_t<const byte_t>{frame});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::CrcMismatch));
}

TEST(FrameCodec, RTUTooShort) {
    std::vector<byte_t> frame = {0x01, 0x03, 0x00};
    auto result = FrameCodec::unwrap_rtu(span_t<const byte_t>{frame});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::InvalidFrame));
}

// ---------------------------------------------------------------------------
// TCP (MBAP) framing
// ---------------------------------------------------------------------------

TEST(FrameCodec, TCPRoundtrip) {
    auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                   {0x00, 0x10, 0x00, 0x04});
    unit_id_t unit = 5;
    std::uint16_t txn_id = 1234;

    auto frame = FrameCodec::wrap_tcp(unit, pdu, txn_id);

    // MBAP header (7) + FC(1) + data(4) = 12
    EXPECT_EQ(frame.size(), 12u);

    auto result = FrameCodec::unwrap_tcp(span_t<const byte_t>{frame});
    ASSERT_TRUE(result.has_value());

    auto& [decoded_unit, decoded_pdu, decoded_txn] = result.value();
    EXPECT_EQ(decoded_unit, unit);
    EXPECT_EQ(decoded_txn, txn_id);
    EXPECT_EQ(decoded_pdu.function_code(), FunctionCode::ReadHoldingRegisters);
    EXPECT_EQ(decoded_pdu.payload().size(), 4u);
}

TEST(FrameCodec, TCPMBAPStructure) {
    auto pdu = PDU::make_standard(FunctionCode::WriteSingleRegister, {0x00, 0x01, 0x00, 0xFF});
    auto frame = FrameCodec::wrap_tcp(1, pdu, 0x0042);

    // Transaction ID
    EXPECT_EQ(frame[0], 0x00);
    EXPECT_EQ(frame[1], 0x42);

    // Protocol ID (always 0x0000)
    EXPECT_EQ(frame[2], 0x00);
    EXPECT_EQ(frame[3], 0x00);

    // Length = 1 (unit ID) + 1 (FC) + 4 (data) = 6
    EXPECT_EQ(frame[4], 0x00);
    EXPECT_EQ(frame[5], 0x06);

    // Unit ID
    EXPECT_EQ(frame[6], 0x01);
}

TEST(FrameCodec, TCPTooShort) {
    std::vector<byte_t> frame = {0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    auto result = FrameCodec::unwrap_tcp(span_t<const byte_t>{frame});
    EXPECT_FALSE(result.has_value());
}

TEST(FrameCodec, TCPBadProtocolID) {
    auto pdu = PDU::make_standard(FunctionCode::ReadCoils, {0x00, 0x00, 0x00, 0x08});
    auto frame = FrameCodec::wrap_tcp(1, pdu, 1);

    // Corrupt protocol ID
    frame[2] = 0x01;

    auto result = FrameCodec::unwrap_tcp(span_t<const byte_t>{frame});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::InvalidFrame));
}

// ---------------------------------------------------------------------------
// Extended PDU through framing
// ---------------------------------------------------------------------------

TEST(FrameCodec, RTUExtendedFrame) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
    hdr.correlation_id = 42;

    auto pdu = PDU::make_extended(hdr, {0x01, 0x02, 0x03});
    auto frame = FrameCodec::wrap_rtu(1, pdu);

    auto result = FrameCodec::unwrap_rtu(span_t<const byte_t>{frame});
    ASSERT_TRUE(result.has_value());

    auto& [unit, decoded] = result.value();
    EXPECT_EQ(unit, 1);
    EXPECT_TRUE(decoded.is_extended());
    EXPECT_EQ(decoded.extended_header()->correlation_id, 42);
}

TEST(FrameCodec, TCPExtendedFrame) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::Subscribe;
    hdr.correlation_id = 7;
    hdr.timestamp = Timestamp::from_epoch_us(1000000);

    auto pdu = PDU::make_extended(hdr, {0xFF});
    auto frame = FrameCodec::wrap_tcp(3, pdu, 99);

    auto result = FrameCodec::unwrap_tcp(span_t<const byte_t>{frame});
    ASSERT_TRUE(result.has_value());

    auto& [unit, decoded, txn] = result.value();
    EXPECT_EQ(unit, 3);
    EXPECT_EQ(txn, 99);
    EXPECT_TRUE(decoded.is_extended());
    ASSERT_TRUE(decoded.extended_header()->timestamp.has_value());
    EXPECT_EQ(decoded.extended_header()->timestamp->epoch_microseconds(), 1000000);
}
