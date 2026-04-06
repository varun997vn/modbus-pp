#pragma once

/// @file session.hpp
/// @brief Authenticated session with challenge-response handshake.
///
/// Implements a 4-step HMAC-based handshake:
///   1. Client → Server: AuthChallenge { client_nonce }
///   2. Server → Client: AuthChallenge { server_nonce, HMAC(client_nonce || server_nonce, key) }
///   3. Client → Server: AuthResponse  { HMAC(server_nonce || client_nonce, key) }
///   4. Server → Client: AuthResponse  { session_token, expiry }
///
/// Once established, the session signs all extended frames with HMAC.

#include "hmac_auth.hpp"
#include "../core/error.hpp"
#include "../core/result.hpp"
#include "../core/timestamp.hpp"
#include "../core/types.hpp"

#include <array>
#include <chrono>
#include <vector>

namespace modbus_pp {

/// A session token — 16 bytes.
using SessionToken = std::array<byte_t, 16>;

/// An authenticated session established via the challenge-response handshake.
class Session {
public:
    Session() = default;

    /// Create an authenticated session with a derived key.
    static Session create(SessionToken token,
                          std::vector<byte_t> session_key,
                          Timestamp expiry);

    [[nodiscard]] bool is_authenticated() const noexcept { return authenticated_; }

    [[nodiscard]] bool is_expired() const noexcept {
        return Timestamp::now() > expiry_;
    }

    [[nodiscard]] const SessionToken& token() const noexcept { return token_; }
    [[nodiscard]] Timestamp expiry() const noexcept { return expiry_; }

    /// Sign an outgoing PDU's data with the session key.
    [[nodiscard]] HMACDigest sign(span_t<const byte_t> pdu_data) const;

    /// Verify an incoming PDU's HMAC against the session key.
    [[nodiscard]] bool verify(span_t<const byte_t> pdu_data,
                               const HMACDigest& digest) const;

private:
    SessionToken token_{};
    std::vector<byte_t> session_key_;
    Timestamp expiry_;
    bool authenticated_{false};
};

/// Derive a session key from shared secret and nonces.
///
/// session_key = HMAC(client_nonce || server_nonce, shared_secret)
std::vector<byte_t> derive_session_key(
    const Nonce& client_nonce,
    const Nonce& server_nonce,
    span_t<const byte_t> shared_secret);

/// Generate the server's auth challenge response (step 2).
///
/// Returns HMAC(client_nonce || server_nonce, shared_secret).
HMACDigest compute_auth_challenge_response(
    const Nonce& client_nonce,
    const Nonce& server_nonce,
    span_t<const byte_t> shared_secret);

/// Generate the client's auth response (step 3).
///
/// Returns HMAC(server_nonce || client_nonce, shared_secret).
HMACDigest compute_auth_response(
    const Nonce& client_nonce,
    const Nonce& server_nonce,
    span_t<const byte_t> shared_secret);

} // namespace modbus_pp
