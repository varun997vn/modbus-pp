#pragma once

/// @file hmac_auth.hpp
/// @brief HMAC-SHA256 authentication primitives.
///
/// Provides HMAC computation and verification using OpenSSL, plus
/// cryptographic nonce generation. These are the building blocks for
/// the challenge-response handshake in session.hpp.

#include "../core/types.hpp"

#include <array>
#include <vector>

namespace modbus_pp {

/// HMAC-SHA256 digest — 32 bytes.
using HMACDigest = std::array<byte_t, 32>;

/// Cryptographic nonce — 16 bytes.
using Nonce = std::array<byte_t, 16>;

/// HMAC-SHA256 authentication utilities.
class HMACAuth {
public:
    /// Compute HMAC-SHA256 of data using the given key.
    static HMACDigest compute(span_t<const byte_t> data,
                               span_t<const byte_t> key);

    /// Verify that the expected HMAC matches the computed HMAC.
    /// Uses constant-time comparison to prevent timing attacks.
    static bool verify(span_t<const byte_t> data,
                       span_t<const byte_t> key,
                       const HMACDigest& expected);

    /// Generate a cryptographically random 16-byte nonce.
    static Nonce generate_nonce();
};

} // namespace modbus_pp
