/// @file 06_security_hmac.cpp
/// @brief HMAC-SHA256 authentication and authenticated PDU frames.
///
/// Demonstrates the security primitives: nonce generation, HMAC computation,
/// verification (both correct and tampered), and constructing extended PDU
/// frames with HMAC digests attached.
///
/// Modbus disadvantage addressed: standard Modbus has zero authentication.
/// Any device on the bus can impersonate any unit ID, and there is no way
/// to verify message integrity. modbus_pp adds HMAC-SHA256 authentication
/// with per-device shared secrets and constant-time verification.

#include "modbus_pp/modbus_pp.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

/// Print a byte array as hex.
static void print_hex(const char* label, const byte_t* data, std::size_t len) {
    std::cout << "  " << label << ": ";
    for (std::size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(data[i]);
        if (i % 16 == 15 && i + 1 < len) {
            std::cout << "\n" << std::string(std::string(label).size() + 4, ' ');
        }
    }
    std::cout << std::dec << "\n";
}

int main() {
    std::cout << "=== modbus_pp Example 06: Security (HMAC-SHA256) ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Generate cryptographic nonce
    // -----------------------------------------------------------------------
    std::cout << "--- Nonce Generation ---\n";
    auto nonce1 = HMACAuth::generate_nonce();
    auto nonce2 = HMACAuth::generate_nonce();

    print_hex("Nonce 1 (16 bytes)", nonce1.data(), nonce1.size());
    print_hex("Nonce 2 (16 bytes)", nonce2.data(), nonce2.size());

    bool nonces_different = (nonce1 != nonce2);
    std::cout << "  Nonces are " << (nonces_different ? "unique" : "IDENTICAL (problem!)")
              << " (as expected from CSPRNG)\n\n";

    // -----------------------------------------------------------------------
    // 2. Compute HMAC-SHA256
    // -----------------------------------------------------------------------
    std::cout << "--- HMAC-SHA256 Computation ---\n";

    // Shared secret key (in production, this comes from a secure key store)
    std::vector<byte_t> key = {
        0x4d, 0x6f, 0x64, 0x62, 0x75, 0x73, 0x50, 0x50,  // "ModbusPP"
        0x53, 0x65, 0x63, 0x72, 0x65, 0x74, 0x4b, 0x65,  // "SecretKe"
        0x79, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  // "y1234567"
        0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,  // "89ABCDEF"
    };

    // Message to authenticate (a Modbus read holding registers request)
    std::vector<byte_t> message = {
        0x03,                    // FC: ReadHoldingRegisters
        0x00, 0x00,              // Start address: 0
        0x00, 0x0A,              // Quantity: 10
    };

    std::cout << "  Key:     32 bytes (\"ModbusPPSecretKey12345689ABCDEF\")\n";
    std::cout << "  Message: FC=0x03, addr=0x0000, count=10\n\n";

    HMACDigest digest = HMACAuth::compute(message, key);
    print_hex("HMAC digest (32 bytes)", digest.data(), digest.size());
    std::cout << "\n";

    // -----------------------------------------------------------------------
    // 3. Verify HMAC -- correct message
    // -----------------------------------------------------------------------
    std::cout << "--- HMAC Verification (correct message) ---\n";
    bool valid = HMACAuth::verify(message, key, digest);
    std::cout << "  Result: " << (valid ? "VALID" : "INVALID") << "\n\n";

    // -----------------------------------------------------------------------
    // 4. Verify HMAC -- tampered message
    // -----------------------------------------------------------------------
    std::cout << "--- HMAC Verification (tampered message) ---\n";
    std::vector<byte_t> tampered = message;
    tampered[4] = 0x0B;  // Changed quantity from 10 to 11

    std::cout << "  Original: count=10, Tampered: count=11\n";
    bool tampered_valid = HMACAuth::verify(tampered, key, digest);
    std::cout << "  Result: " << (tampered_valid ? "VALID (BAD!)" : "INVALID (correct rejection)")
              << "\n\n";

    // -----------------------------------------------------------------------
    // 5. Verify HMAC -- wrong key
    // -----------------------------------------------------------------------
    std::cout << "--- HMAC Verification (wrong key) ---\n";
    std::vector<byte_t> wrong_key = key;
    wrong_key[0] ^= 0xFF;  // Flip one byte

    bool wrong_key_valid = HMACAuth::verify(message, wrong_key, digest);
    std::cout << "  Result: " << (wrong_key_valid ? "VALID (BAD!)" : "INVALID (correct rejection)")
              << "\n\n";

    // -----------------------------------------------------------------------
    // 6. InMemoryCredentialStore
    // -----------------------------------------------------------------------
    std::cout << "--- Credential Store ---\n";
    auto store = std::make_shared<InMemoryCredentialStore>();
    store->add_credential(1, key);
    store->add_credential(2, {0xAA, 0xBB, 0xCC, 0xDD});

    auto key_result = store->get_key(1);
    if (key_result) {
        std::cout << "  Device 1 key: " << key_result.value().size() << " bytes (found)\n";
    }

    auto missing = store->get_key(99);
    if (!missing) {
        std::cout << "  Device 99 key: " << missing.error().message() << " (expected)\n\n";
    }

    // -----------------------------------------------------------------------
    // 7. Build an authenticated extended PDU frame
    // -----------------------------------------------------------------------
    std::cout << "--- Authenticated PDU Frame ---\n";

    // Build a payload for a batch read
    std::vector<byte_t> payload = {0x00, 0x01,        // 1 item
                                    0x00, 0x00,        // addr = 0
                                    0x00, 0x0A,        // count = 10
                                    0x00, 0x00};       // type + order

    // Compute HMAC over the payload
    HMACDigest frame_hmac = HMACAuth::compute(payload, key);

    // Build the extended frame header
    FrameHeader header;
    header.version = 1;
    header.flags = FrameFlag::HasHMAC | FrameFlag::HasTimestamp;
    header.correlation_id = 42;
    header.timestamp = Timestamp::now();
    header.ext_function_code = ExtendedFunctionCode::BatchRead;
    header.payload_length = static_cast<std::uint16_t>(payload.size());

    // Create the authenticated PDU
    auto pdu = PDU::make_extended(header, payload, frame_hmac);

    std::cout << "  Frame type:     extended\n";
    std::cout << "  Function code:  0x" << std::hex << std::setfill('0')
              << std::setw(2) << static_cast<int>(pdu.function_code()) << std::dec << "\n";
    std::cout << "  Has HMAC:       " << (pdu.hmac().has_value() ? "yes" : "no") << "\n";
    std::cout << "  Is extended:    " << (pdu.is_extended() ? "yes" : "no") << "\n";
    std::cout << "  Correlation ID: " << pdu.extended_header()->correlation_id << "\n";

    // Serialize and show size
    auto serialized = pdu.serialize();
    std::cout << "  Serialized size: " << serialized.size() << " bytes\n";
    std::cout << "    (includes 32-byte HMAC + 8-byte timestamp)\n\n";

    // Verify the HMAC on the received frame
    std::cout << "--- Verify Received Frame HMAC ---\n";
    auto deserialized = PDU::deserialize(serialized);
    if (deserialized) {
        auto& rx_pdu = deserialized.value();
        if (rx_pdu.hmac().has_value()) {
            // Extract payload and verify
            auto rx_payload = rx_pdu.payload_vec();
            bool frame_valid = HMACAuth::verify(rx_payload, key, *rx_pdu.hmac());
            std::cout << "  Frame HMAC verification: "
                      << (frame_valid ? "VALID" : "INVALID") << "\n\n";
        }
    } else {
        std::cerr << "  Deserialization failed: "
                  << deserialized.error().message() << "\n\n";
    }

    // -----------------------------------------------------------------------
    // Summary
    // -----------------------------------------------------------------------
    std::cout << "--- Security Summary ---\n"
              << "  Standard Modbus:  No authentication, no integrity checks.\n"
              << "  modbus_pp:        HMAC-SHA256 per-frame authentication with\n"
              << "                    per-device keys, cryptographic nonces, and\n"
              << "                    constant-time verification (timing-attack safe).\n\n";

    std::cout << "[done] Security HMAC example complete.\n";
    return 0;
}
