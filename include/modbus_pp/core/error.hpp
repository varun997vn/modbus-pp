#pragma once

/// @file error.hpp
/// @brief Rich error handling via std::error_code integration.
///
/// Standard Modbus defines only 9 exception codes. This module extends
/// error reporting with fine-grained error codes for all modbus_pp features,
/// registered as a proper std::error_category so they integrate seamlessly
/// with the C++ standard library error handling infrastructure.

#include <string>
#include <system_error>

namespace modbus_pp {

/// Error codes covering both standard Modbus exceptions and extended
/// modbus_pp error conditions.
enum class ErrorCode : int {
    Success                  = 0,

    // Standard Modbus exception codes (1–11 per Modbus spec)
    IllegalFunction          = 1,
    IllegalDataAddress       = 2,
    IllegalDataValue         = 3,
    SlaveDeviceFailure       = 4,
    Acknowledge              = 5,
    SlaveDeviceBusy          = 6,
    NegativeAcknowledge      = 7,
    MemoryParityError        = 8,
    GatewayPathUnavailable   = 0x0A,
    GatewayTargetFailed      = 0x0B,

    // Extended modbus_pp error codes (0x80+)
    AuthenticationFailed     = 0x80,
    SessionExpired           = 0x81,
    HMACMismatch             = 0x82,
    PipelineOverflow         = 0x83,
    SubscriptionLimitReached = 0x84,
    PayloadTooLarge          = 0x85,
    FragmentReassemblyFailed = 0x86,
    TransportDisconnected    = 0x87,
    TimeoutExpired           = 0x88,
    TypeCodecMismatch        = 0x89,
    UnsupportedVersion       = 0x8A,
    CorrelationMismatch      = 0x8B,
    BatchPartialFailure      = 0x8C,
    InvalidFrame             = 0x8D,
    CrcMismatch              = 0x8E,
    DeserializationFailed    = 0x8F,
};

/// The std::error_category for modbus_pp error codes.
class ModbusCategory final : public std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] std::string message(int ev) const override;
};

/// Returns the singleton ModbusCategory instance.
const std::error_category& modbus_category() noexcept;

/// Construct a std::error_code from an ErrorCode.
std::error_code make_error_code(ErrorCode e) noexcept;

} // namespace modbus_pp

// Register ErrorCode as an error_code enum so implicit conversion works.
namespace std {
template <>
struct is_error_code_enum<modbus_pp::ErrorCode> : true_type {};
} // namespace std
