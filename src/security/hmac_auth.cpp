#include "modbus_pp/security/hmac_auth.hpp"

#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <algorithm>

namespace modbus_pp {

HMACDigest HMACAuth::compute(span_t<const byte_t> data,
                              span_t<const byte_t> key) {
    HMACDigest digest{};
    unsigned int len = 0;

    HMAC(EVP_sha256(),
         key.data(), static_cast<int>(key.size()),
         data.data(), data.size(),
         digest.data(), &len);

    return digest;
}

bool HMACAuth::verify(span_t<const byte_t> data,
                       span_t<const byte_t> key,
                       const HMACDigest& expected) {
    auto computed = compute(data, key);

    // Constant-time comparison to prevent timing attacks
    volatile byte_t diff = 0;
    for (std::size_t i = 0; i < 32; ++i) {
        diff |= computed[i] ^ expected[i];
    }
    return diff == 0;
}

Nonce HMACAuth::generate_nonce() {
    Nonce nonce{};
    RAND_bytes(nonce.data(), static_cast<int>(nonce.size()));
    return nonce;
}

} // namespace modbus_pp
