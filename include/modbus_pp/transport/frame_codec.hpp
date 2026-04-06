#pragma once

/// @file frame_codec.hpp
/// @brief ADU framing/deframing for TCP (MBAP) and RTU (CRC16) transports.
///
/// Wraps PDUs into Application Data Units (ADUs) appropriate for each
/// transport type, handling MBAP headers, CRC16 calculation, and
/// transaction ID management.

#include "../core/pdu.hpp"
#include "../core/result.hpp"
#include "../core/types.hpp"

#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

namespace modbus_pp {

/// ADU framing utilities for TCP and RTU transports.
class FrameCodec {
public:
    // -----------------------------------------------------------------------
    // TCP (MBAP) framing
    // -----------------------------------------------------------------------

    /// Wrap a PDU into a TCP ADU with MBAP header.
    ///
    /// MBAP header: [TransactionID:2][ProtocolID:2][Length:2][UnitID:1]
    ///
    /// @param unit_id        Modbus unit/slave address.
    /// @param pdu            The PDU to wrap.
    /// @param transaction_id TCP transaction identifier.
    static std::vector<byte_t> wrap_tcp(unit_id_t unit_id, const PDU& pdu,
                                         std::uint16_t transaction_id);

    /// Unwrap a TCP ADU, extracting the unit ID, PDU, and transaction ID.
    static Result<std::tuple<unit_id_t, PDU, std::uint16_t>>
    unwrap_tcp(span_t<const byte_t> frame);

    // -----------------------------------------------------------------------
    // RTU framing
    // -----------------------------------------------------------------------

    /// Wrap a PDU into an RTU ADU with CRC16.
    ///
    /// RTU frame: [UnitID:1][PDU:N][CRC16:2]
    static std::vector<byte_t> wrap_rtu(unit_id_t unit_id, const PDU& pdu);

    /// Unwrap an RTU ADU, verifying CRC16 and extracting unit ID and PDU.
    static Result<std::pair<unit_id_t, PDU>> unwrap_rtu(span_t<const byte_t> frame);

    /// Compute the Modbus CRC16 (polynomial 0xA001).
    static std::uint16_t crc16(span_t<const byte_t> data) noexcept;
};

} // namespace modbus_pp
