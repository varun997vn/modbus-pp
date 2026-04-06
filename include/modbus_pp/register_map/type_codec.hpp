#pragma once

/// @file type_codec.hpp
/// @brief Encode/decode typed values to/from Modbus register arrays.
///
/// Bridges the gap between Modbus's native 16-bit registers and real-world
/// data types (float, double, int32, strings, user-defined structs).
/// All conversions respect the configurable byte order.

#include "../core/endian.hpp"
#include "../core/types.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace modbus_pp {

/// Codec for converting between typed values and Modbus register arrays.
class TypeCodec {
public:
    // -----------------------------------------------------------------------
    // Float (2 registers)
    // -----------------------------------------------------------------------

    template <ByteOrder Order = ByteOrder::ABCD>
    static std::array<register_t, 2> encode_float(float value) noexcept {
        return encode_float_regs<Order>(value);
    }

    template <ByteOrder Order = ByteOrder::ABCD>
    static float decode_float(const register_t* regs) noexcept {
        return decode_float_regs<Order>(regs);
    }

    // -----------------------------------------------------------------------
    // Double (4 registers)
    // -----------------------------------------------------------------------

    template <ByteOrder Order = ByteOrder::ABCD>
    static std::array<register_t, 4> encode_double(double value) noexcept {
        return encode_double_regs<Order>(value);
    }

    template <ByteOrder Order = ByteOrder::ABCD>
    static double decode_double(const register_t* regs) noexcept {
        return decode_double_regs<Order>(regs);
    }

    // -----------------------------------------------------------------------
    // UInt32 (2 registers)
    // -----------------------------------------------------------------------

    template <ByteOrder Order = ByteOrder::ABCD>
    static std::array<register_t, 2> encode_uint32(std::uint32_t value) noexcept {
        return encode_uint32_regs<Order>(value);
    }

    template <ByteOrder Order = ByteOrder::ABCD>
    static std::uint32_t decode_uint32(const register_t* regs) noexcept {
        return decode_uint32_regs<Order>(regs);
    }

    // -----------------------------------------------------------------------
    // Int32 (2 registers)
    // -----------------------------------------------------------------------

    template <ByteOrder Order = ByteOrder::ABCD>
    static std::array<register_t, 2> encode_int32(std::int32_t value) noexcept {
        std::uint32_t uval;
        std::memcpy(&uval, &value, sizeof(uval));
        return encode_uint32_regs<Order>(uval);
    }

    template <ByteOrder Order = ByteOrder::ABCD>
    static std::int32_t decode_int32(const register_t* regs) noexcept {
        auto uval = decode_uint32_regs<Order>(regs);
        std::int32_t result;
        std::memcpy(&result, &uval, sizeof(result));
        return result;
    }

    // -----------------------------------------------------------------------
    // String (N registers, packed 2 chars per register, null-padded)
    // -----------------------------------------------------------------------

    /// Encode a string into registers (2 chars per register, null-padded).
    static std::vector<register_t> encode_string(std::string_view str,
                                                  quantity_t max_registers) {
        std::vector<register_t> regs(max_registers, 0);
        for (std::size_t i = 0; i < str.size() && i / 2 < max_registers; ++i) {
            auto reg_idx = i / 2;
            auto ch = static_cast<std::uint16_t>(str[i]);
            if (i % 2 == 0) {
                regs[reg_idx] = static_cast<register_t>(ch << 8);
            } else {
                regs[reg_idx] |= static_cast<register_t>(ch);
            }
        }
        return regs;
    }

    /// Decode registers into a string (2 chars per register).
    static std::string decode_string(const register_t* regs, quantity_t count) {
        std::string result;
        result.reserve(static_cast<std::size_t>(count) * 2);
        for (quantity_t i = 0; i < count; ++i) {
            auto hi = static_cast<char>((regs[i] >> 8) & 0xFF);
            auto lo = static_cast<char>(regs[i] & 0xFF);
            if (hi == '\0') break;
            result.push_back(hi);
            if (lo == '\0') break;
            result.push_back(lo);
        }
        return result;
    }
};

/// Trait for user-defined struct codecs.
///
/// Specialize this template for your custom types:
/// ```cpp
/// template<>
/// struct StructCodec<MyStruct> {
///     static constexpr quantity_t register_count = 4;
///     static std::vector<register_t> encode(const MyStruct& v);
///     static MyStruct decode(const register_t* regs);
/// };
/// ```
template <typename T>
struct StructCodec {
    static_assert(sizeof(T) != sizeof(T),
                  "Specialize StructCodec<T> for your custom type");
};

} // namespace modbus_pp
