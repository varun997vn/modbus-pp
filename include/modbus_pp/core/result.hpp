#pragma once

/// @file result.hpp
/// @brief Exception-free Result<T> type for the modbus_pp library.
///
/// Provides a monadic Result type built on std::variant<T, std::error_code>.
/// This is the primary error-propagation mechanism — modbus_pp never throws
/// exceptions in the data path.

#include "error.hpp"

#include <cassert>
#include <type_traits>
#include <utility>
#include <variant>

namespace modbus_pp {

/// A result type that holds either a value of type T or a std::error_code.
///
/// Inspired by Rust's Result and C++23 std::expected, implemented for C++17.
/// Supports monadic operations (and_then, map, value_or) for clean chaining.
///
/// @tparam T  The success value type.
template <typename T>
class Result {
public:
    /// Construct a successful result.
    Result(T value) : storage_{std::move(value)} {}  // NOLINT(implicit)

    /// Construct a failed result from a std::error_code.
    Result(std::error_code ec) : storage_{ec} {      // NOLINT(implicit)
        assert(ec && "Cannot construct error Result with success error_code");
    }

    /// Construct a failed result from an ErrorCode enum.
    Result(ErrorCode ec) : storage_{make_error_code(ec)} {} // NOLINT(implicit)

    /// @return true if this result holds a value.
    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    /// Explicit bool conversion — true if has value.
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    /// Access the value. Undefined behaviour if !has_value().
    [[nodiscard]] T& value() & {
        assert(has_value());
        return std::get<T>(storage_);
    }

    [[nodiscard]] const T& value() const& {
        assert(has_value());
        return std::get<T>(storage_);
    }

    [[nodiscard]] T&& value() && {
        assert(has_value());
        return std::get<T>(std::move(storage_));
    }

    /// Access the error code. Returns a default (success) code if has_value().
    [[nodiscard]] std::error_code error() const noexcept {
        if (has_value()) return {};
        return std::get<std::error_code>(storage_);
    }

    /// Return the value if present, otherwise return the provided default.
    [[nodiscard]] T value_or(T default_val) const& {
        if (has_value()) return std::get<T>(storage_);
        return default_val;
    }

    /// Monadic and_then: if this result has a value, apply f to it and return
    /// the resulting Result. Otherwise propagate the error.
    ///
    /// @param f  A callable T → Result<U>.
    template <typename F>
    auto and_then(F&& f) const& -> std::invoke_result_t<F, const T&> {
        if (has_value()) return std::forward<F>(f)(std::get<T>(storage_));
        return error();
    }

    /// Monadic map: if this result has a value, apply f and wrap the return
    /// value in a new Result. Otherwise propagate the error.
    ///
    /// @param f  A callable T → U.
    template <typename F>
    auto map(F&& f) const& -> Result<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) return Result<U>{std::forward<F>(f)(std::get<T>(storage_))};
        return Result<U>{error()};
    }

private:
    std::variant<T, std::error_code> storage_;
};

/// Specialization for void results (success or error, no value).
template <>
class Result<void> {
public:
    /// Construct a successful void result.
    Result() : ec_{} {}

    /// Construct a failed result from a std::error_code.
    Result(std::error_code ec) : ec_{ec} {} // NOLINT(implicit)

    /// Construct a failed result from an ErrorCode enum.
    Result(ErrorCode ec) : ec_{make_error_code(ec)} {} // NOLINT(implicit)

    [[nodiscard]] bool has_value() const noexcept { return !ec_; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] std::error_code error() const noexcept { return ec_; }

private:
    std::error_code ec_;
};

} // namespace modbus_pp
