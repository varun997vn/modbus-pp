#include "modbus_pp/security/session.hpp"

#include <algorithm>

namespace modbus_pp {

Session Session::create(SessionToken token,
                        std::vector<byte_t> session_key,
                        Timestamp expiry) {
    Session s;
    s.token_ = token;
    s.session_key_ = std::move(session_key);
    s.expiry_ = expiry;
    s.authenticated_ = true;
    return s;
}

HMACDigest Session::sign(span_t<const byte_t> pdu_data) const {
    return HMACAuth::compute(pdu_data, span_t<const byte_t>{session_key_});
}

bool Session::verify(span_t<const byte_t> pdu_data,
                      const HMACDigest& digest) const {
    return HMACAuth::verify(pdu_data, span_t<const byte_t>{session_key_}, digest);
}

// Concatenate two nonces into a 32-byte buffer
static std::vector<byte_t> concat_nonces(const Nonce& a, const Nonce& b) {
    std::vector<byte_t> combined(32);
    std::copy(a.begin(), a.end(), combined.begin());
    std::copy(b.begin(), b.end(), combined.begin() + 16);
    return combined;
}

std::vector<byte_t> derive_session_key(
    const Nonce& client_nonce,
    const Nonce& server_nonce,
    span_t<const byte_t> shared_secret) {
    auto combined = concat_nonces(client_nonce, server_nonce);
    auto digest = HMACAuth::compute(
        span_t<const byte_t>{combined}, shared_secret);
    return {digest.begin(), digest.end()};
}

HMACDigest compute_auth_challenge_response(
    const Nonce& client_nonce,
    const Nonce& server_nonce,
    span_t<const byte_t> shared_secret) {
    auto combined = concat_nonces(client_nonce, server_nonce);
    return HMACAuth::compute(span_t<const byte_t>{combined}, shared_secret);
}

HMACDigest compute_auth_response(
    const Nonce& client_nonce,
    const Nonce& server_nonce,
    span_t<const byte_t> shared_secret) {
    // Note: reversed order — server_nonce first
    auto combined = concat_nonces(server_nonce, client_nonce);
    return HMACAuth::compute(span_t<const byte_t>{combined}, shared_secret);
}

} // namespace modbus_pp
