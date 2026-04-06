#pragma once

/// @file timestamp.hpp
/// @brief Microsecond-resolution timestamps and the Stamped<T> wrapper.
///
/// Standard Modbus has no temporal context for data. This module provides
/// a compact Timestamp type that serializes to exactly 8 bytes (int64
/// microseconds since Unix epoch) and a Stamped<T> wrapper that pairs
/// any value with its acquisition time.

#include "types.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <utility>

namespace modbus_pp {

/// High-resolution timestamp with microsecond precision.
///
/// Serialized as a 64-bit signed integer (microseconds since Unix epoch)
/// in big-endian byte order, occupying exactly 8 bytes in the extended
/// modbus_pp frame.
class Timestamp {
public:
    using clock     = std::chrono::system_clock;
    using duration  = std::chrono::microseconds;
    using time_point = std::chrono::time_point<clock, duration>;

    /// Create a timestamp representing the current time.
    static Timestamp now() noexcept {
        auto tp = std::chrono::time_point_cast<duration>(clock::now());
        return Timestamp{tp.time_since_epoch().count()};
    }

    /// Create a timestamp from raw microseconds since Unix epoch.
    static Timestamp from_epoch_us(std::int64_t microseconds) noexcept {
        return Timestamp{microseconds};
    }

    /// Default constructor — epoch (time 0).
    constexpr Timestamp() noexcept : us_since_epoch_{0} {}

    /// @return Microseconds since Unix epoch.
    [[nodiscard]] constexpr std::int64_t epoch_microseconds() const noexcept {
        return us_since_epoch_;
    }

    /// Convert to a std::chrono time_point.
    [[nodiscard]] time_point to_time_point() const noexcept {
        return time_point{duration{us_since_epoch_}};
    }

    /// Serialize to exactly 8 bytes (big-endian int64).
    [[nodiscard]] std::array<byte_t, 8> serialize() const noexcept {
        std::array<byte_t, 8> buf{};
        auto val = static_cast<std::uint64_t>(us_since_epoch_);
        for (int i = 7; i >= 0; --i) {
            buf[static_cast<std::size_t>(i)] = static_cast<byte_t>(val & 0xFF);
            val >>= 8;
        }
        return buf;
    }

    /// Deserialize from 8 bytes (big-endian int64).
    static Timestamp deserialize(const byte_t* data) noexcept {
        std::uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            val = (val << 8) | data[i];
        }
        std::int64_t signed_val;
        std::memcpy(&signed_val, &val, sizeof(signed_val));
        return Timestamp{signed_val};
    }

    // Comparison operators
    friend constexpr bool operator==(Timestamp a, Timestamp b) noexcept {
        return a.us_since_epoch_ == b.us_since_epoch_;
    }
    friend constexpr bool operator!=(Timestamp a, Timestamp b) noexcept {
        return a.us_since_epoch_ != b.us_since_epoch_;
    }
    friend constexpr bool operator<(Timestamp a, Timestamp b) noexcept {
        return a.us_since_epoch_ < b.us_since_epoch_;
    }
    friend constexpr bool operator<=(Timestamp a, Timestamp b) noexcept {
        return a.us_since_epoch_ <= b.us_since_epoch_;
    }
    friend constexpr bool operator>(Timestamp a, Timestamp b) noexcept {
        return a.us_since_epoch_ > b.us_since_epoch_;
    }
    friend constexpr bool operator>=(Timestamp a, Timestamp b) noexcept {
        return a.us_since_epoch_ >= b.us_since_epoch_;
    }

private:
    explicit constexpr Timestamp(std::int64_t us) noexcept : us_since_epoch_(us) {}

    std::int64_t us_since_epoch_;
};

/// Pairs a value with a timestamp, used to attach temporal context
/// to register reads and event notifications.
///
/// @tparam T  The value type.
template <typename T>
struct Stamped {
    T         value;
    Timestamp timestamp;

    Stamped(T v, Timestamp ts) : value(std::move(v)), timestamp(ts) {}

    /// Create a Stamped value using the current time.
    static Stamped now(T v) { return {std::move(v), Timestamp::now()}; }
};

} // namespace modbus_pp
