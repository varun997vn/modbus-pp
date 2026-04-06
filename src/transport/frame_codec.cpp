#include "modbus_pp/transport/frame_codec.hpp"

namespace modbus_pp {

// ---------------------------------------------------------------------------
// TCP (MBAP) framing
// ---------------------------------------------------------------------------

std::vector<byte_t> FrameCodec::wrap_tcp(unit_id_t unit_id, const PDU& pdu,
                                          std::uint16_t transaction_id) {
    auto pdu_bytes = pdu.serialize();
    auto pdu_len = pdu_bytes.size();

    // MBAP header: TransactionID(2) + ProtocolID(2) + Length(2) + UnitID(1)
    // Length field = UnitID(1) + PDU size
    std::uint16_t length = static_cast<std::uint16_t>(1 + pdu_len);

    std::vector<byte_t> frame;
    frame.reserve(7 + pdu_len);

    // Transaction ID (big-endian)
    frame.push_back(static_cast<byte_t>(transaction_id >> 8));
    frame.push_back(static_cast<byte_t>(transaction_id & 0xFF));

    // Protocol ID (0x0000 for Modbus)
    frame.push_back(0x00);
    frame.push_back(0x00);

    // Length (big-endian)
    frame.push_back(static_cast<byte_t>(length >> 8));
    frame.push_back(static_cast<byte_t>(length & 0xFF));

    // Unit ID
    frame.push_back(unit_id);

    // PDU bytes
    frame.insert(frame.end(), pdu_bytes.begin(), pdu_bytes.end());

    return frame;
}

Result<std::tuple<unit_id_t, PDU, std::uint16_t>>
FrameCodec::unwrap_tcp(span_t<const byte_t> frame) {
    // Minimum: MBAP header (7) + function code (1) = 8
    if (frame.size() < 8) {
        return ErrorCode::InvalidFrame;
    }

    std::uint16_t transaction_id = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(frame[0]) << 8) | frame[1]);

    // Protocol ID check (must be 0x0000)
    if (frame[2] != 0x00 || frame[3] != 0x00) {
        return ErrorCode::InvalidFrame;
    }

    std::uint16_t length = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(frame[4]) << 8) | frame[5]);

    // Length includes unit ID (1 byte) + PDU
    if (frame.size() < static_cast<std::size_t>(6 + length)) {
        return ErrorCode::InvalidFrame;
    }

    unit_id_t unit_id = frame[6];

    // PDU starts at offset 7, length is (length - 1) bytes
    auto pdu_result = PDU::deserialize(
        span_t<const byte_t>{frame.data() + 7, static_cast<std::size_t>(length - 1)});
    if (!pdu_result) {
        return pdu_result.error();
    }

    return std::make_tuple(unit_id, std::move(pdu_result).value(), transaction_id);
}

// ---------------------------------------------------------------------------
// RTU framing
// ---------------------------------------------------------------------------

std::vector<byte_t> FrameCodec::wrap_rtu(unit_id_t unit_id, const PDU& pdu) {
    auto pdu_bytes = pdu.serialize();

    std::vector<byte_t> frame;
    frame.reserve(1 + pdu_bytes.size() + 2);

    frame.push_back(unit_id);
    frame.insert(frame.end(), pdu_bytes.begin(), pdu_bytes.end());

    // CRC16 over unit_id + pdu
    auto crc = crc16(span_t<const byte_t>{frame.data(), frame.size()});
    // CRC is appended low byte first, then high byte (Modbus RTU convention)
    frame.push_back(static_cast<byte_t>(crc & 0xFF));
    frame.push_back(static_cast<byte_t>(crc >> 8));

    return frame;
}

Result<std::pair<unit_id_t, PDU>>
FrameCodec::unwrap_rtu(span_t<const byte_t> frame) {
    // Minimum: UnitID(1) + FC(1) + CRC(2) = 4
    if (frame.size() < 4) {
        return ErrorCode::InvalidFrame;
    }

    // Verify CRC16 (covers everything except the last 2 CRC bytes)
    auto data_len = frame.size() - 2;
    auto expected_crc = crc16(span_t<const byte_t>{frame.data(), data_len});

    // CRC is stored low byte first
    auto received_crc = static_cast<std::uint16_t>(
        frame[data_len] | (static_cast<std::uint16_t>(frame[data_len + 1]) << 8));

    if (expected_crc != received_crc) {
        return ErrorCode::CrcMismatch;
    }

    unit_id_t unit_id = frame[0];

    auto pdu_result = PDU::deserialize(
        span_t<const byte_t>{frame.data() + 1, data_len - 1});
    if (!pdu_result) {
        return pdu_result.error();
    }

    return std::make_pair(unit_id, std::move(pdu_result).value());
}

// ---------------------------------------------------------------------------
// CRC16 (Modbus polynomial 0xA001)
// ---------------------------------------------------------------------------

std::uint16_t FrameCodec::crc16(span_t<const byte_t> data) noexcept {
    std::uint16_t crc = 0xFFFF;

    for (std::size_t i = 0; i < data.size(); ++i) {
        crc ^= static_cast<std::uint16_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

} // namespace modbus_pp
