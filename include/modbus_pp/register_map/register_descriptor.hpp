#pragma once

/// @file register_descriptor.hpp
/// @brief Compile-time typed register descriptors.
///
/// Each descriptor binds a register address to its data type, count, and
/// byte order at compile time. This eliminates an entire class of runtime
/// bugs: wrong register count, wrong byte order, wrong data type
/// interpretation.

#include "../core/endian.hpp"
#include "../core/types.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

namespace modbus_pp {

/// Register data types supported by the type system.
enum class RegisterType : std::uint8_t {
    UInt16,
    Int16,
    UInt32,
    Int32,
    Float32,
    Float64,
    String,
};

namespace detail {

/// Map RegisterType → C++ value type.
template <RegisterType T>
struct RegisterValueType;

template <> struct RegisterValueType<RegisterType::UInt16>  { using type = std::uint16_t; };
template <> struct RegisterValueType<RegisterType::Int16>   { using type = std::int16_t; };
template <> struct RegisterValueType<RegisterType::UInt32>  { using type = std::uint32_t; };
template <> struct RegisterValueType<RegisterType::Int32>   { using type = std::int32_t; };
template <> struct RegisterValueType<RegisterType::Float32> { using type = float; };
template <> struct RegisterValueType<RegisterType::Float64> { using type = double; };
template <> struct RegisterValueType<RegisterType::String>  { using type = std::string; };

/// Default register count for each type.
template <RegisterType T>
constexpr quantity_t default_register_count() {
    if constexpr (T == RegisterType::UInt16 || T == RegisterType::Int16)
        return 1;
    else if constexpr (T == RegisterType::UInt32 || T == RegisterType::Int32 ||
                       T == RegisterType::Float32)
        return 2;
    else if constexpr (T == RegisterType::Float64)
        return 4;
    else
        return 0; // String: must be specified explicitly
}

} // namespace detail

/// Compile-time register descriptor binding address, type, count, and byte order.
///
/// @tparam Type   The register data type.
/// @tparam Addr   The starting register address.
/// @tparam Count  Number of 16-bit registers (0 = auto-detect from Type).
/// @tparam Order  Byte order for multi-register values.
///
/// Example:
/// ```cpp
/// using Temperature = RegisterDescriptor<RegisterType::Float32, 0x0000, 2, ByteOrder::BADC>;
/// using Status      = RegisterDescriptor<RegisterType::UInt16,  0x0002>;
/// using DeviceName  = RegisterDescriptor<RegisterType::String,  0x0010, 16>;
/// ```
template <RegisterType Type,
          address_t Addr,
          quantity_t Count = detail::default_register_count<Type>(),
          ByteOrder Order = ByteOrder::ABCD>
struct RegisterDescriptor {
    static constexpr RegisterType type    = Type;
    static constexpr address_t    address = Addr;
    static constexpr quantity_t   count   = Count;
    static constexpr ByteOrder    order   = Order;

    using value_type = typename detail::RegisterValueType<Type>::type;

    static_assert(Type != RegisterType::String || Count > 0,
                  "String RegisterDescriptor must specify a non-zero Count");
    static_assert(Count > 0, "Register count must be positive");

    /// End address (exclusive) — used for overlap detection.
    static constexpr address_t end_address = Addr + Count;
};

} // namespace modbus_pp
