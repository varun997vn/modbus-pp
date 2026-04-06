#pragma once

/// @file endian.hpp
/// @brief Compile-time endian-aware encoding/decoding for multi-register values.
///
/// Modbus uses 16-bit registers, but real devices store 32-bit floats, doubles,
/// and integers across multiple registers with vendor-specific byte ordering.
/// This module provides constexpr encode/decode functions parameterized by
/// ByteOrder, compiled to direct byte-swap instructions with zero runtime cost.

#include "types.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace modbus_pp {

/// Byte ordering conventions used by different Modbus device vendors.
///
/// For a 32-bit value with bytes [A, B, C, D] (A = most significant):
///   ABCD — big-endian (most common, Modbus default)
///   DCBA — little-endian
///   BADC — mid-big-endian / byte-swapped (common in Schneider, ABB)
///   CDAB — mid-little-endian / word-swapped
enum class ByteOrder : std::uint8_t {
    ABCD = 0x00,
    DCBA = 0x01,
    BADC = 0x02,
    CDAB = 0x03,
};

namespace detail {

// Helper: reorder bytes according to ByteOrder
template <ByteOrder Order, std::size_t N>
inline std::array<byte_t, N> reorder(std::array<byte_t, N> bytes) noexcept {
    static_assert(N == 4 || N == 8, "reorder supports 4 or 8 byte values");

    if constexpr (Order == ByteOrder::ABCD) {
        return bytes; // no-op
    } else if constexpr (Order == ByteOrder::DCBA) {
        if constexpr (N == 4) {
            return {bytes[3], bytes[2], bytes[1], bytes[0]};
        } else {
            return {bytes[7], bytes[6], bytes[5], bytes[4],
                    bytes[3], bytes[2], bytes[1], bytes[0]};
        }
    } else if constexpr (Order == ByteOrder::BADC) {
        if constexpr (N == 4) {
            return {bytes[1], bytes[0], bytes[3], bytes[2]};
        } else {
            return {bytes[1], bytes[0], bytes[3], bytes[2],
                    bytes[5], bytes[4], bytes[7], bytes[6]};
        }
    } else if constexpr (Order == ByteOrder::CDAB) {
        if constexpr (N == 4) {
            return {bytes[2], bytes[3], bytes[0], bytes[1]};
        } else {
            return {bytes[6], bytes[7], bytes[4], bytes[5],
                    bytes[2], bytes[3], bytes[0], bytes[1]};
        }
    }
}

// Convert native type to big-endian bytes
template <typename T>
inline std::array<byte_t, sizeof(T)> to_big_endian_bytes(T value) noexcept {
    std::array<byte_t, sizeof(T)> bytes{};
    // Write in big-endian (MSB first)
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        bytes[i] = static_cast<byte_t>(
            (value >> ((sizeof(T) - 1 - i) * 8)) & 0xFF);
    }
    return bytes;
}

// Specialization for floating-point: memcpy to uint, then to big-endian bytes
inline std::array<byte_t, 4> to_big_endian_bytes_float(float value) noexcept {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return to_big_endian_bytes(bits);
}

inline std::array<byte_t, 8> to_big_endian_bytes_double(double value) noexcept {
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return to_big_endian_bytes(bits);
}

// Convert big-endian bytes back to native integer type
template <typename T>
inline T from_big_endian_bytes(const byte_t* data) noexcept {
    T value = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        value = static_cast<T>(value | (static_cast<T>(data[i]) << ((sizeof(T) - 1 - i) * 8)));
    }
    return value;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Encode functions — native type → byte array in specified byte order
// ---------------------------------------------------------------------------

/// Encode a float into 4 bytes using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<byte_t, 4> encode_float(float value) noexcept {
    auto bytes = detail::to_big_endian_bytes_float(value);
    return detail::reorder<Order>(bytes);
}

/// Encode a double into 8 bytes using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<byte_t, 8> encode_double(double value) noexcept {
    auto bytes = detail::to_big_endian_bytes_double(value);
    return detail::reorder<Order>(bytes);
}

/// Encode a uint32_t into 4 bytes using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<byte_t, 4> encode_uint32(std::uint32_t value) noexcept {
    auto bytes = detail::to_big_endian_bytes(value);
    return detail::reorder<Order>(bytes);
}

/// Encode an int32_t into 4 bytes using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<byte_t, 4> encode_int32(std::int32_t value) noexcept {
    std::uint32_t uval;
    std::memcpy(&uval, &value, sizeof(uval));
    return encode_uint32<Order>(uval);
}

/// Encode a uint16_t into 2 bytes (big-endian, byte order irrelevant for 2 bytes).
inline std::array<byte_t, 2> encode_uint16(std::uint16_t value) noexcept {
    return {static_cast<byte_t>(value >> 8), static_cast<byte_t>(value & 0xFF)};
}

/// Encode an int16_t into 2 bytes (big-endian).
inline std::array<byte_t, 2> encode_int16(std::int16_t value) noexcept {
    std::uint16_t uval;
    std::memcpy(&uval, &value, sizeof(uval));
    return encode_uint16(uval);
}

// ---------------------------------------------------------------------------
// Decode functions — byte array in specified byte order → native type
// ---------------------------------------------------------------------------

/// Decode 4 bytes into a float using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline float decode_float(const byte_t* data) noexcept {
    std::array<byte_t, 4> bytes{data[0], data[1], data[2], data[3]};
    auto ordered = detail::reorder<Order>(bytes);
    // ordered is now in ABCD (big-endian) order — but we wrote ABCD as
    // big-endian, so reorder back: the inverse of the encoding reorder
    // is the same operation (self-inverse for swaps). However, for decode
    // we need to undo the encoding transform.
    // Actually: encoding applies reorder(ABCD→Order).
    // Decoding must apply the inverse: reorder(Order→ABCD).
    // For ABCD: identity. For DCBA: reverse (self-inverse).
    // For BADC and CDAB: also self-inverse.
    // So applying the same reorder is correct.
    std::uint32_t bits = detail::from_big_endian_bytes<std::uint32_t>(ordered.data());
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

/// Decode 8 bytes into a double using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline double decode_double(const byte_t* data) noexcept {
    std::array<byte_t, 8> bytes{data[0], data[1], data[2], data[3],
                                 data[4], data[5], data[6], data[7]};
    auto ordered = detail::reorder<Order>(bytes);
    std::uint64_t bits = detail::from_big_endian_bytes<std::uint64_t>(ordered.data());
    double result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

/// Decode 4 bytes into a uint32_t using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::uint32_t decode_uint32(const byte_t* data) noexcept {
    std::array<byte_t, 4> bytes{data[0], data[1], data[2], data[3]};
    auto ordered = detail::reorder<Order>(bytes);
    return detail::from_big_endian_bytes<std::uint32_t>(ordered.data());
}

/// Decode 4 bytes into an int32_t using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::int32_t decode_int32(const byte_t* data) noexcept {
    auto uval = decode_uint32<Order>(data);
    std::int32_t result;
    std::memcpy(&result, &uval, sizeof(result));
    return result;
}

/// Decode 2 bytes (big-endian) into a uint16_t.
inline std::uint16_t decode_uint16(const byte_t* data) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[0]) << 8) | data[1]);
}

/// Decode 2 bytes (big-endian) into an int16_t.
inline std::int16_t decode_int16(const byte_t* data) noexcept {
    auto uval = decode_uint16(data);
    std::int16_t result;
    std::memcpy(&result, &uval, sizeof(result));
    return result;
}

// ---------------------------------------------------------------------------
// Register-level encode/decode (register_t = uint16_t pairs)
// ---------------------------------------------------------------------------

/// Encode a float into 2 Modbus registers using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<register_t, 2> encode_float_regs(float value) noexcept {
    auto bytes = encode_float<Order>(value);
    return {decode_uint16(bytes.data()), decode_uint16(bytes.data() + 2)};
}

/// Decode 2 Modbus registers into a float using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline float decode_float_regs(const register_t* regs) noexcept {
    std::array<byte_t, 4> bytes;
    auto hi = encode_uint16(regs[0]);
    auto lo = encode_uint16(regs[1]);
    bytes[0] = hi[0]; bytes[1] = hi[1];
    bytes[2] = lo[0]; bytes[3] = lo[1];
    return decode_float<Order>(bytes.data());
}

/// Encode a uint32_t into 2 Modbus registers using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<register_t, 2> encode_uint32_regs(std::uint32_t value) noexcept {
    auto bytes = encode_uint32<Order>(value);
    return {decode_uint16(bytes.data()), decode_uint16(bytes.data() + 2)};
}

/// Decode 2 Modbus registers into a uint32_t using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::uint32_t decode_uint32_regs(const register_t* regs) noexcept {
    std::array<byte_t, 4> bytes;
    auto hi = encode_uint16(regs[0]);
    auto lo = encode_uint16(regs[1]);
    bytes[0] = hi[0]; bytes[1] = hi[1];
    bytes[2] = lo[0]; bytes[3] = lo[1];
    return decode_uint32<Order>(bytes.data());
}

/// Encode a double into 4 Modbus registers using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline std::array<register_t, 4> encode_double_regs(double value) noexcept {
    auto bytes = encode_double<Order>(value);
    return {decode_uint16(bytes.data()),     decode_uint16(bytes.data() + 2),
            decode_uint16(bytes.data() + 4), decode_uint16(bytes.data() + 6)};
}

/// Decode 4 Modbus registers into a double using the specified byte order.
template <ByteOrder Order = ByteOrder::ABCD>
inline double decode_double_regs(const register_t* regs) noexcept {
    std::array<byte_t, 8> bytes;
    for (int i = 0; i < 4; ++i) {
        auto b = encode_uint16(regs[i]);
        bytes[static_cast<std::size_t>(i * 2)]     = b[0];
        bytes[static_cast<std::size_t>(i * 2 + 1)] = b[1];
    }
    return decode_double<Order>(bytes.data());
}

} // namespace modbus_pp
