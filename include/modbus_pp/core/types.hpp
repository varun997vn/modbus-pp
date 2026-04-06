#pragma once

/// @file types.hpp
/// @brief Fundamental types and strong type wrappers for the modbus_pp library.
///
/// Provides type aliases for Modbus primitives and a zero-overhead StrongType
/// wrapper that prevents accidental mixing of semantically distinct values
/// (e.g., a register address vs. a register count).

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace modbus_pp {

// ---------------------------------------------------------------------------
// Primitive type aliases
// ---------------------------------------------------------------------------

using byte_t     = std::uint8_t;
using register_t = std::uint16_t;
using unit_id_t  = std::uint8_t;
using address_t  = std::uint16_t;
using quantity_t = std::uint16_t;

// ---------------------------------------------------------------------------
// StrongType — zero-overhead tagged wrapper
// ---------------------------------------------------------------------------

/// A tag-based wrapper that prevents implicit conversions between distinct
/// integer types (e.g., RegisterAddress vs. RegisterCount).
///
/// @tparam Tag   A unique tag type used to distinguish wrapper types.
/// @tparam T     The underlying arithmetic type (default: std::uint16_t).
template <typename Tag, typename T = std::uint16_t>
class StrongType {
public:
    static_assert(std::is_arithmetic_v<T>, "StrongType requires an arithmetic underlying type");

    constexpr StrongType() noexcept : value_{} {}
    constexpr explicit StrongType(T v) noexcept : value_(v) {}

    [[nodiscard]] constexpr T raw() const noexcept { return value_; }

    constexpr StrongType& operator++() noexcept    { ++value_; return *this; }
    constexpr StrongType  operator++(int) noexcept  { auto tmp = *this; ++value_; return tmp; }

    friend constexpr bool operator==(StrongType a, StrongType b) noexcept { return a.value_ == b.value_; }
    friend constexpr bool operator!=(StrongType a, StrongType b) noexcept { return a.value_ != b.value_; }
    friend constexpr bool operator< (StrongType a, StrongType b) noexcept { return a.value_ <  b.value_; }
    friend constexpr bool operator<=(StrongType a, StrongType b) noexcept { return a.value_ <= b.value_; }
    friend constexpr bool operator> (StrongType a, StrongType b) noexcept { return a.value_ >  b.value_; }
    friend constexpr bool operator>=(StrongType a, StrongType b) noexcept { return a.value_ >= b.value_; }

private:
    T value_;
};

// Tag types
struct AddressTag  {};
struct QuantityTag {};
struct UnitIdTag   {};

using RegisterAddress = StrongType<AddressTag,  address_t>;
using RegisterCount   = StrongType<QuantityTag, quantity_t>;
using UnitId          = StrongType<UnitIdTag,   unit_id_t>;

// ---------------------------------------------------------------------------
// Lightweight span — non-owning view over contiguous data
// ---------------------------------------------------------------------------

/// Minimal non-owning span implementation for C++17.
/// Replaces gsl::span / std::span (C++20) to avoid external dependencies.
///
/// @tparam T  Element type.
template <typename T>
class span_t {
public:
    constexpr span_t() noexcept : data_{nullptr}, size_{0} {}
    constexpr span_t(T* data, std::size_t size) noexcept : data_{data}, size_{size} {}

    template <std::size_t N>
    constexpr span_t(T (&arr)[N]) noexcept : data_{arr}, size_{N} {}            // NOLINT

    template <std::size_t N>
    constexpr span_t(std::array<T, N>& arr) noexcept : data_{arr.data()}, size_{N} {} // NOLINT

    template <std::size_t N>
    constexpr span_t(const std::array<std::remove_const_t<T>, N>& arr) noexcept // NOLINT
        : data_{arr.data()}, size_{N} {}

    /// Construct from any contiguous container (std::vector, std::string, etc.)
    template <typename Container,
              typename = std::enable_if_t<
                  !std::is_array_v<Container> &&
                  std::is_convertible_v<decltype(std::declval<Container&>().data()), T*>>>
    constexpr span_t(Container& c) noexcept : data_{c.data()}, size_{c.size()} {} // NOLINT

    [[nodiscard]] constexpr T*          data()  const noexcept { return data_; }
    [[nodiscard]] constexpr std::size_t size()  const noexcept { return size_; }
    [[nodiscard]] constexpr bool        empty() const noexcept { return size_ == 0; }

    [[nodiscard]] constexpr T& operator[](std::size_t i) const noexcept { return data_[i]; }

    [[nodiscard]] constexpr T* begin() const noexcept { return data_; }
    [[nodiscard]] constexpr T* end()   const noexcept { return data_ + size_; }

    [[nodiscard]] constexpr span_t subspan(std::size_t offset, std::size_t count) const noexcept {
        return {data_ + offset, count};
    }

    [[nodiscard]] constexpr span_t first(std::size_t count) const noexcept {
        return {data_, count};
    }

    [[nodiscard]] constexpr span_t last(std::size_t count) const noexcept {
        return {data_ + size_ - count, count};
    }

private:
    T*          data_;
    std::size_t size_;
};

} // namespace modbus_pp

// Hash support for StrongType
namespace std {
template <typename Tag, typename T>
struct hash<modbus_pp::StrongType<Tag, T>> {
    std::size_t operator()(modbus_pp::StrongType<Tag, T> v) const noexcept {
        return std::hash<T>{}(v.raw());
    }
};
} // namespace std
