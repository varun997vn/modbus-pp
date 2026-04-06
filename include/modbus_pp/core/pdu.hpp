#pragma once

/// @file pdu.hpp
/// @brief Protocol Data Unit — standard Modbus and extended modbus_pp frames.
///
/// Standard Modbus PDUs pass through unchanged for wire compatibility.
/// Extended frames use function code 0x6E with a structured header that
/// supports correlation IDs, timestamps, HMAC authentication, and
/// payloads exceeding the 253-byte standard limit.

#include "error.hpp"
#include "function_codes.hpp"
#include "result.hpp"
#include "timestamp.hpp"
#include "types.hpp"

#include <array>
#include <optional>
#include <vector>

namespace modbus_pp {

/// HMAC-SHA256 digest — 32 bytes.
using HMACDigest = std::array<byte_t, 32>;

/// Header for extended modbus_pp frames.
struct FrameHeader {
    std::uint8_t            version{1};
    FrameFlag               flags{FrameFlag::None};
    std::uint16_t           correlation_id{0};
    std::optional<Timestamp> timestamp;
    ExtendedFunctionCode    ext_function_code{};
    std::uint16_t           payload_length{0};
};

/// Protocol Data Unit — the core frame type for modbus_pp.
///
/// Supports both standard Modbus frames (wire-compatible with any device)
/// and extended frames that carry the modbus_pp enhancements.
///
/// ## Standard frame layout
/// ```
/// [FunctionCode: 1 byte] [Data: 0..252 bytes]
/// ```
///
/// ## Extended frame layout (FC = 0x6E)
/// ```
/// [0x6E] [Version:1] [Flags:2] [CorrelationID:2]
/// [Timestamp?:8] [ExtFC:1] [PayloadLen:2] [Payload:N] [HMAC?:32]
/// ```
class PDU {
public:
    /// Create a standard Modbus PDU (wire-compatible).
    static PDU make_standard(FunctionCode fc, std::vector<byte_t> data);

    /// Create an extended modbus_pp PDU.
    static PDU make_extended(FrameHeader header, std::vector<byte_t> payload,
                             std::optional<HMACDigest> hmac = std::nullopt);

    /// Check if raw frame bytes represent an extended frame.
    static bool is_extended_frame(span_t<const byte_t> raw) noexcept;

    /// Serialize the PDU to bytes.
    [[nodiscard]] std::vector<byte_t> serialize() const;

    /// Deserialize bytes into a PDU.
    static Result<PDU> deserialize(span_t<const byte_t> raw);

    // Accessors
    [[nodiscard]] FunctionCode function_code() const noexcept { return function_code_; }
    [[nodiscard]] const std::optional<FrameHeader>& extended_header() const noexcept { return ext_header_; }
    [[nodiscard]] span_t<const byte_t> payload() const noexcept { return {payload_.data(), payload_.size()}; }
    [[nodiscard]] const std::vector<byte_t>& payload_vec() const noexcept { return payload_; }
    [[nodiscard]] bool is_standard() const noexcept { return !ext_header_.has_value(); }
    [[nodiscard]] bool is_extended() const noexcept { return ext_header_.has_value(); }
    [[nodiscard]] const std::optional<HMACDigest>& hmac() const noexcept { return hmac_; }

    PDU() = default;

private:

    FunctionCode              function_code_{};
    std::optional<FrameHeader> ext_header_;
    std::vector<byte_t>        payload_;
    std::optional<HMACDigest>  hmac_;
};

} // namespace modbus_pp
