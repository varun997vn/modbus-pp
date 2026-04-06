#include <gtest/gtest.h>

#include <modbus_pp/security/credential_store.hpp>
#include <modbus_pp/security/hmac_auth.hpp>
#include <modbus_pp/security/session.hpp>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// HMAC-SHA256 — RFC 4231 Test Vector 1
// Key  = 0x0b repeated 20 times
// Data = "Hi There"
// HMAC = b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7
// ---------------------------------------------------------------------------

TEST(HMACAuth, RFC4231TestVector1) {
    std::vector<byte_t> key(20, 0x0b);
    std::string data_str = "Hi There";
    std::vector<byte_t> data(data_str.begin(), data_str.end());

    auto digest = HMACAuth::compute(span_t<const byte_t>{data},
                                     span_t<const byte_t>{key});

    // Expected HMAC-SHA256
    HMACDigest expected = {
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
        0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
        0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
        0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
    };

    EXPECT_EQ(digest, expected);
}

// ---------------------------------------------------------------------------
// RFC 4231 Test Vector 2
// Key  = "Jefe"
// Data = "what do ya want for nothing?"
// HMAC = 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
// ---------------------------------------------------------------------------

TEST(HMACAuth, RFC4231TestVector2) {
    std::string key_str = "Jefe";
    std::string data_str = "what do ya want for nothing?";
    std::vector<byte_t> key(key_str.begin(), key_str.end());
    std::vector<byte_t> data(data_str.begin(), data_str.end());

    auto digest = HMACAuth::compute(span_t<const byte_t>{data},
                                     span_t<const byte_t>{key});

    HMACDigest expected = {
        0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
        0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
        0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
        0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
    };

    EXPECT_EQ(digest, expected);
}

// ---------------------------------------------------------------------------
// Verify
// ---------------------------------------------------------------------------

TEST(HMACAuth, VerifyCorrect) {
    std::vector<byte_t> key = {0x01, 0x02, 0x03, 0x04};
    std::vector<byte_t> data = {0xAA, 0xBB, 0xCC};

    auto digest = HMACAuth::compute(span_t<const byte_t>{data},
                                     span_t<const byte_t>{key});

    EXPECT_TRUE(HMACAuth::verify(span_t<const byte_t>{data},
                                  span_t<const byte_t>{key}, digest));
}

TEST(HMACAuth, VerifyWrongDigest) {
    std::vector<byte_t> key = {0x01, 0x02, 0x03, 0x04};
    std::vector<byte_t> data = {0xAA, 0xBB, 0xCC};

    HMACDigest bad_digest{};
    EXPECT_FALSE(HMACAuth::verify(span_t<const byte_t>{data},
                                   span_t<const byte_t>{key}, bad_digest));
}

TEST(HMACAuth, VerifyWrongKey) {
    std::vector<byte_t> key1 = {0x01, 0x02};
    std::vector<byte_t> key2 = {0x03, 0x04};
    std::vector<byte_t> data = {0xAA};

    auto digest = HMACAuth::compute(span_t<const byte_t>{data},
                                     span_t<const byte_t>{key1});

    EXPECT_FALSE(HMACAuth::verify(span_t<const byte_t>{data},
                                   span_t<const byte_t>{key2}, digest));
}

// ---------------------------------------------------------------------------
// Nonce generation
// ---------------------------------------------------------------------------

TEST(HMACAuth, NonceUniqueness) {
    auto n1 = HMACAuth::generate_nonce();
    auto n2 = HMACAuth::generate_nonce();
    // Overwhelming probability these are different
    EXPECT_NE(n1, n2);
}

TEST(HMACAuth, NonceNotAllZeros) {
    auto n = HMACAuth::generate_nonce();
    Nonce zeros{};
    EXPECT_NE(n, zeros);
}

// ---------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------

TEST(Session, DefaultNotAuthenticated) {
    Session s;
    EXPECT_FALSE(s.is_authenticated());
}

TEST(Session, Create) {
    SessionToken token{};
    std::vector<byte_t> key = {0x01, 0x02, 0x03, 0x04};
    auto expiry = Timestamp::from_epoch_us(
        Timestamp::now().epoch_microseconds() + 3600'000'000LL); // 1 hour

    auto s = Session::create(token, key, expiry);
    EXPECT_TRUE(s.is_authenticated());
    EXPECT_FALSE(s.is_expired());
}

TEST(Session, SignAndVerify) {
    std::vector<byte_t> key = {0xDE, 0xAD, 0xBE, 0xEF};
    auto expiry = Timestamp::from_epoch_us(
        Timestamp::now().epoch_microseconds() + 3600'000'000LL);

    auto s = Session::create(SessionToken{}, key, expiry);

    std::vector<byte_t> data = {0x01, 0x02, 0x03};
    auto digest = s.sign(span_t<const byte_t>{data});

    EXPECT_TRUE(s.verify(span_t<const byte_t>{data}, digest));
}

TEST(Session, ExpiredSession) {
    std::vector<byte_t> key = {0x01};
    // Already expired (epoch time 0)
    auto s = Session::create(SessionToken{}, key, Timestamp::from_epoch_us(0));
    EXPECT_TRUE(s.is_expired());
}

// ---------------------------------------------------------------------------
// Challenge-response handshake helpers
// ---------------------------------------------------------------------------

TEST(SessionHandshake, ChallengeResponseAsymmetry) {
    auto client_nonce = HMACAuth::generate_nonce();
    auto server_nonce = HMACAuth::generate_nonce();
    std::vector<byte_t> secret = {0x42, 0x43, 0x44, 0x45};

    auto challenge = compute_auth_challenge_response(
        client_nonce, server_nonce, span_t<const byte_t>{secret});
    auto response = compute_auth_response(
        client_nonce, server_nonce, span_t<const byte_t>{secret});

    // Challenge and response should be different (different nonce order)
    EXPECT_NE(challenge, response);
}

TEST(SessionHandshake, ChallengeResponseDeterministic) {
    Nonce cn = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    Nonce sn = {16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};
    std::vector<byte_t> secret = {0xAA, 0xBB};

    auto r1 = compute_auth_challenge_response(cn, sn, span_t<const byte_t>{secret});
    auto r2 = compute_auth_challenge_response(cn, sn, span_t<const byte_t>{secret});
    EXPECT_EQ(r1, r2);
}

TEST(SessionHandshake, DerivedKeyIsConsistent) {
    Nonce cn{}, sn{};
    cn.fill(0xAA);
    sn.fill(0xBB);
    std::vector<byte_t> secret = {0x01, 0x02, 0x03};

    auto k1 = derive_session_key(cn, sn, span_t<const byte_t>{secret});
    auto k2 = derive_session_key(cn, sn, span_t<const byte_t>{secret});
    EXPECT_EQ(k1, k2);
    EXPECT_EQ(k1.size(), 32u); // SHA-256 output
}

// ---------------------------------------------------------------------------
// CredentialStore
// ---------------------------------------------------------------------------

TEST(CredentialStore, InMemoryFound) {
    InMemoryCredentialStore store;
    store.add_credential(1, {0x01, 0x02, 0x03});

    auto result = store.get_key(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 3u);
}

TEST(CredentialStore, InMemoryNotFound) {
    InMemoryCredentialStore store;
    auto result = store.get_key(99);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::AuthenticationFailed));
}
