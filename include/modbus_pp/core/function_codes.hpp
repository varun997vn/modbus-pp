#pragma once

/// @file function_codes.hpp
/// @brief Standard and extended Modbus function code definitions.
///
/// Standard function codes (0x01–0x10) are wire-compatible with any
/// Modbus device. The extended function code 0x6E is used as an escape
/// hatch for modbus_pp features, with sub-function codes in
/// ExtendedFunctionCode.

#include <cstdint>

namespace modbus_pp {

/// Standard Modbus function codes.
enum class FunctionCode : std::uint8_t {
    ReadCoils              = 0x01,
    ReadDiscreteInputs     = 0x02,
    ReadHoldingRegisters   = 0x03,
    ReadInputRegisters     = 0x04,
    WriteSingleCoil        = 0x05,
    WriteSingleRegister    = 0x06,
    WriteMultipleCoils     = 0x0F,
    WriteMultipleRegisters = 0x10,
    ReadWriteMultipleRegs  = 0x17,

    /// Extended modbus_pp frame marker (user-defined range, unreserved).
    Extended               = 0x6E,

    /// Extended stream marker (for pub/sub push notifications).
    ExtendedStream         = 0x6F,
};

/// Sub-function codes carried inside Extended (0x6E) frames.
enum class ExtendedFunctionCode : std::uint8_t {
    BatchRead              = 0x01,
    BatchWrite             = 0x02,
    Subscribe              = 0x03,
    Unsubscribe            = 0x04,
    EventNotification      = 0x05,
    AuthChallenge          = 0x06,
    AuthResponse           = 0x07,
    Discover               = 0x08,
    DiscoverResponse       = 0x09,
    PipelineBegin          = 0x0A,
    PipelineEnd            = 0x0B,
};

/// Bitfield flags in the extended frame header.
enum class FrameFlag : std::uint16_t {
    None         = 0x0000,
    HasTimestamp  = 0x0001,
    HasHMAC      = 0x0002,
    IsCompressed = 0x0004,
    IsFragmented = 0x0008,
    RequiresAck  = 0x0010,
};

// Bitwise operators for FrameFlag
inline constexpr FrameFlag operator|(FrameFlag a, FrameFlag b) noexcept {
    return static_cast<FrameFlag>(
        static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}

inline constexpr FrameFlag operator&(FrameFlag a, FrameFlag b) noexcept {
    return static_cast<FrameFlag>(
        static_cast<std::uint16_t>(a) & static_cast<std::uint16_t>(b));
}

inline constexpr FrameFlag operator~(FrameFlag a) noexcept {
    return static_cast<FrameFlag>(~static_cast<std::uint16_t>(a));
}

inline constexpr FrameFlag& operator|=(FrameFlag& a, FrameFlag b) noexcept {
    a = a | b;
    return a;
}

inline constexpr FrameFlag& operator&=(FrameFlag& a, FrameFlag b) noexcept {
    a = a & b;
    return a;
}

/// Check if a flag is set.
inline constexpr bool has_flag(FrameFlag flags, FrameFlag flag) noexcept {
    return (flags & flag) != FrameFlag::None;
}

} // namespace modbus_pp
