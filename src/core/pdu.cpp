#include "modbus_pp/core/pdu.hpp"

#include <algorithm>

namespace modbus_pp {

PDU PDU::make_standard(FunctionCode fc, std::vector<byte_t> data) {
    PDU pdu;
    pdu.function_code_ = fc;
    pdu.payload_ = std::move(data);
    return pdu;
}

PDU PDU::make_extended(FrameHeader header, std::vector<byte_t> payload,
                       std::optional<HMACDigest> hmac) {
    PDU pdu;
    pdu.function_code_ = FunctionCode::Extended;
    header.payload_length = static_cast<std::uint16_t>(payload.size());
    if (header.timestamp.has_value()) {
        header.flags |= FrameFlag::HasTimestamp;
    }
    if (hmac.has_value()) {
        header.flags |= FrameFlag::HasHMAC;
    }
    pdu.ext_header_ = header;
    pdu.payload_ = std::move(payload);
    pdu.hmac_ = hmac;
    return pdu;
}

bool PDU::is_extended_frame(span_t<const byte_t> raw) noexcept {
    if (raw.empty()) return false;
    return raw[0] == static_cast<byte_t>(FunctionCode::Extended) ||
           raw[0] == static_cast<byte_t>(FunctionCode::ExtendedStream);
}

std::vector<byte_t> PDU::serialize() const {
    std::vector<byte_t> out;

    if (is_standard()) {
        // Standard Modbus PDU: [FC:1][Data:N]
        out.reserve(1 + payload_.size());
        out.push_back(static_cast<byte_t>(function_code_));
        out.insert(out.end(), payload_.begin(), payload_.end());
        return out;
    }

    // Extended frame: [0x6E][Version:1][Flags:2][CorrelationID:2]
    //                 [Timestamp?:8][ExtFC:1][PayloadLen:2][Payload:N][HMAC?:32]
    const auto& hdr = *ext_header_;

    // Estimate size
    std::size_t est = 1 + 1 + 2 + 2 + 1 + 2 + payload_.size();
    if (has_flag(hdr.flags, FrameFlag::HasTimestamp)) est += 8;
    if (has_flag(hdr.flags, FrameFlag::HasHMAC)) est += 32;
    out.reserve(est);

    out.push_back(static_cast<byte_t>(FunctionCode::Extended));
    out.push_back(hdr.version);

    // Flags — big-endian
    auto flags_val = static_cast<std::uint16_t>(hdr.flags);
    out.push_back(static_cast<byte_t>(flags_val >> 8));
    out.push_back(static_cast<byte_t>(flags_val & 0xFF));

    // Correlation ID — big-endian
    out.push_back(static_cast<byte_t>(hdr.correlation_id >> 8));
    out.push_back(static_cast<byte_t>(hdr.correlation_id & 0xFF));

    // Optional timestamp
    if (has_flag(hdr.flags, FrameFlag::HasTimestamp) && hdr.timestamp.has_value()) {
        auto ts_bytes = hdr.timestamp->serialize();
        out.insert(out.end(), ts_bytes.begin(), ts_bytes.end());
    }

    // Extended function code
    out.push_back(static_cast<byte_t>(hdr.ext_function_code));

    // Payload length — big-endian
    out.push_back(static_cast<byte_t>(hdr.payload_length >> 8));
    out.push_back(static_cast<byte_t>(hdr.payload_length & 0xFF));

    // Payload
    out.insert(out.end(), payload_.begin(), payload_.end());

    // Optional HMAC
    if (has_flag(hdr.flags, FrameFlag::HasHMAC) && hmac_.has_value()) {
        out.insert(out.end(), hmac_->begin(), hmac_->end());
    }

    return out;
}

Result<PDU> PDU::deserialize(span_t<const byte_t> raw) {
    if (raw.size() < 1) {
        return ErrorCode::InvalidFrame;
    }

    auto fc = static_cast<FunctionCode>(raw[0]);

    // Standard frame
    if (fc != FunctionCode::Extended && fc != FunctionCode::ExtendedStream) {
        std::vector<byte_t> data(raw.data() + 1, raw.data() + raw.size());
        return PDU::make_standard(fc, std::move(data));
    }

    // Extended frame — minimum header: FC(1) + Ver(1) + Flags(2) + CorrID(2) + ExtFC(1) + PayloadLen(2) = 9
    constexpr std::size_t min_header = 9;
    if (raw.size() < min_header) {
        return ErrorCode::InvalidFrame;
    }

    FrameHeader hdr;
    std::size_t offset = 1;

    hdr.version = raw[offset++];
    if (hdr.version != 1) {
        return ErrorCode::UnsupportedVersion;
    }

    hdr.flags = static_cast<FrameFlag>(
        (static_cast<std::uint16_t>(raw[offset]) << 8) | raw[offset + 1]);
    offset += 2;

    hdr.correlation_id = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(raw[offset]) << 8) | raw[offset + 1]);
    offset += 2;

    // Optional timestamp
    if (has_flag(hdr.flags, FrameFlag::HasTimestamp)) {
        if (raw.size() < offset + 8) {
            return ErrorCode::InvalidFrame;
        }
        hdr.timestamp = Timestamp::deserialize(raw.data() + offset);
        offset += 8;
    }

    if (raw.size() < offset + 3) { // ExtFC(1) + PayloadLen(2)
        return ErrorCode::InvalidFrame;
    }

    hdr.ext_function_code = static_cast<ExtendedFunctionCode>(raw[offset++]);

    hdr.payload_length = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(raw[offset]) << 8) | raw[offset + 1]);
    offset += 2;

    // Payload
    if (raw.size() < offset + hdr.payload_length) {
        return ErrorCode::InvalidFrame;
    }

    std::vector<byte_t> payload(raw.data() + offset,
                                 raw.data() + offset + hdr.payload_length);
    offset += hdr.payload_length;

    // Optional HMAC
    std::optional<HMACDigest> hmac;
    if (has_flag(hdr.flags, FrameFlag::HasHMAC)) {
        if (raw.size() < offset + 32) {
            return ErrorCode::InvalidFrame;
        }
        HMACDigest digest;
        std::copy_n(raw.data() + offset, 32, digest.begin());
        hmac = digest;
    }

    PDU pdu;
    pdu.function_code_ = fc;
    pdu.ext_header_ = hdr;
    pdu.payload_ = std::move(payload);
    pdu.hmac_ = hmac;
    return pdu;
}

} // namespace modbus_pp
