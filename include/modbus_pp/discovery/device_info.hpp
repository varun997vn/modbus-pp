#pragma once

/// @file device_info.hpp
/// @brief Discovered device descriptor with capability bitfield.

#include "../core/timestamp.hpp"
#include "../core/types.hpp"

#include <cstdint>
#include <string>

namespace modbus_pp {

/// Capabilities advertised by a device that supports modbus_pp extensions.
enum class Capability : std::uint32_t {
    None        = 0,
    Pipeline    = 1 << 0,
    PubSub      = 1 << 1,
    BatchOps    = 1 << 2,
    HMACAuth    = 1 << 3,
    Timestamps  = 1 << 4,
    Compression = 1 << 5,
};

inline constexpr Capability operator|(Capability a, Capability b) noexcept {
    return static_cast<Capability>(
        static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
inline constexpr Capability operator&(Capability a, Capability b) noexcept {
    return static_cast<Capability>(
        static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}
inline constexpr bool has_capability(Capability caps, Capability cap) noexcept {
    return (caps & cap) != Capability::None;
}

/// Information about a discovered Modbus device.
struct DeviceInfo {
    unit_id_t    unit_id{0};
    std::string  vendor;
    std::string  product_code;
    std::string  firmware_version;
    bool         supports_extended{false};
    std::uint8_t extension_version{0};
    Timestamp    discovered_at;
    Capability   capabilities{Capability::None};
};

} // namespace modbus_pp
