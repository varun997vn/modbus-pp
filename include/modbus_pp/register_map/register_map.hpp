#pragma once

/// @file register_map.hpp
/// @brief Compile-time typed register map with overlap detection.
///
/// RegisterMap bundles multiple RegisterDescriptors into a validated
/// collection. Overlapping address ranges are caught at compile time via
/// static_assert. Provides type-safe decode/encode by descriptor index.

#include "register_descriptor.hpp"
#include "type_codec.hpp"
#include "../core/types.hpp"

#include <array>
#include <tuple>
#include <type_traits>
#include <vector>

namespace modbus_pp {

namespace detail {

// ---------------------------------------------------------------------------
// Compile-time overlap detection
// ---------------------------------------------------------------------------

/// Check if two descriptors overlap.
template <typename A, typename B>
struct descriptors_overlap {
    static constexpr bool value =
        (A::address < B::end_address) && (B::address < A::end_address);
};

/// Check all pairs for overlaps (base case: single or empty).
template <typename...>
struct no_overlaps : std::true_type {};

template <typename First>
struct no_overlaps<First> : std::true_type {};

template <typename First, typename Second, typename... Rest>
struct no_overlaps<First, Second, Rest...> {
    static constexpr bool value =
        !descriptors_overlap<First, Second>::value &&
        no_overlaps<First, Rest...>::value &&
        no_overlaps<Second, Rest...>::value;
};

template <typename... Ds>
constexpr bool no_overlaps_v = no_overlaps<Ds...>::value;

// ---------------------------------------------------------------------------
// Decode/encode helpers using constexpr if
// ---------------------------------------------------------------------------

template <typename Descriptor>
typename Descriptor::value_type decode_register(const register_t* regs) {
    using VT = typename Descriptor::value_type;
    constexpr auto type = Descriptor::type;
    constexpr auto order = Descriptor::order;

    if constexpr (type == RegisterType::UInt16) {
        return regs[0];
    } else if constexpr (type == RegisterType::Int16) {
        return static_cast<std::int16_t>(regs[0]);
    } else if constexpr (type == RegisterType::UInt32) {
        return TypeCodec::decode_uint32<order>(regs);
    } else if constexpr (type == RegisterType::Int32) {
        return TypeCodec::decode_int32<order>(regs);
    } else if constexpr (type == RegisterType::Float32) {
        return TypeCodec::decode_float<order>(regs);
    } else if constexpr (type == RegisterType::Float64) {
        return TypeCodec::decode_double<order>(regs);
    } else if constexpr (type == RegisterType::String) {
        return TypeCodec::decode_string(regs, Descriptor::count);
    } else {
        static_assert(sizeof(VT) == 0, "Unsupported RegisterType");
    }
}

template <typename Descriptor>
void encode_register(const typename Descriptor::value_type& value, register_t* regs) {
    constexpr auto type = Descriptor::type;
    constexpr auto order = Descriptor::order;

    if constexpr (type == RegisterType::UInt16) {
        regs[0] = value;
    } else if constexpr (type == RegisterType::Int16) {
        regs[0] = static_cast<register_t>(value);
    } else if constexpr (type == RegisterType::UInt32) {
        auto arr = TypeCodec::encode_uint32<order>(value);
        regs[0] = arr[0]; regs[1] = arr[1];
    } else if constexpr (type == RegisterType::Int32) {
        auto arr = TypeCodec::encode_int32<order>(value);
        regs[0] = arr[0]; regs[1] = arr[1];
    } else if constexpr (type == RegisterType::Float32) {
        auto arr = TypeCodec::encode_float<order>(value);
        regs[0] = arr[0]; regs[1] = arr[1];
    } else if constexpr (type == RegisterType::Float64) {
        auto arr = TypeCodec::encode_double<order>(value);
        regs[0] = arr[0]; regs[1] = arr[1]; regs[2] = arr[2]; regs[3] = arr[3];
    } else if constexpr (type == RegisterType::String) {
        auto encoded = TypeCodec::encode_string(value, Descriptor::count);
        for (std::size_t i = 0; i < encoded.size(); ++i) {
            regs[i] = encoded[i];
        }
    }
}

// Compute total register count across all descriptors
template <typename... Ds>
constexpr quantity_t total_register_count() {
    return static_cast<quantity_t>((... + Ds::count));
}

// Find the offset of descriptor I within a flat register buffer
template <std::size_t I, typename Tuple>
constexpr quantity_t register_offset() {
    if constexpr (I == 0) {
        return 0;
    } else {
        using Prev = std::tuple_element_t<I - 1, Tuple>;
        return register_offset<I - 1, Tuple>() + Prev::count;
    }
}

} // namespace detail

/// A compile-time collection of register descriptors.
///
/// Validates at compile time that no descriptors have overlapping address
/// ranges. Provides type-safe decode/encode indexed by descriptor position.
///
/// Usage:
/// ```cpp
/// using SensorMap = RegisterMap<
///     RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>,  // temperature
///     RegisterDescriptor<RegisterType::UInt16,  0x0002>,                       // status
///     RegisterDescriptor<RegisterType::Float64, 0x0003>                        // pressure
/// >;
///
/// // Decode the temperature (index 0) from a raw register buffer:
/// auto temp = SensorMap::decode<0>(raw_registers);  // returns float
/// ```
template <typename... Descriptors>
class RegisterMap {
public:
    static constexpr std::size_t descriptor_count = sizeof...(Descriptors);
    static constexpr quantity_t total_registers = detail::total_register_count<Descriptors...>();

    static_assert(detail::no_overlaps_v<Descriptors...>,
                  "Register descriptors have overlapping address ranges");

    /// Get the descriptor type at index I.
    template <std::size_t I>
    using descriptor_at = std::tuple_element_t<I, std::tuple<Descriptors...>>;

    /// Decode a typed value from a flat register buffer at descriptor index I.
    ///
    /// The buffer must contain registers laid out in descriptor order
    /// (concatenated, not by Modbus address).
    template <std::size_t I>
    static auto decode(const register_t* regs) -> typename descriptor_at<I>::value_type {
        constexpr auto offset = detail::register_offset<I, std::tuple<Descriptors...>>();
        return detail::decode_register<descriptor_at<I>>(regs + offset);
    }

    /// Encode a typed value into a flat register buffer at descriptor index I.
    template <std::size_t I>
    static void encode(const typename descriptor_at<I>::value_type& value, register_t* regs) {
        constexpr auto offset = detail::register_offset<I, std::tuple<Descriptors...>>();
        detail::encode_register<descriptor_at<I>>(value, regs + offset);
    }

    /// Return the address and count info for each descriptor.
    struct DescriptorInfo {
        address_t  address;
        quantity_t count;
        RegisterType type;
        ByteOrder  order;
    };

    /// Get descriptor info for all descriptors (useful for batch request generation).
    static std::array<DescriptorInfo, descriptor_count> descriptor_info() {
        return {DescriptorInfo{Descriptors::address, Descriptors::count,
                               Descriptors::type, Descriptors::order}...};
    }
};

} // namespace modbus_pp
