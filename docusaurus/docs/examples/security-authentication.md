---
sidebar_position: 6
---

# Security & HMAC Authentication

Challenge-response handshake with HMAC-SHA256.

## Overview

Demonstrates:

- Generating cryptographic nonces
- Computing HMAC-SHA256 digests
- Challenge-response handshake
- Authenticating PDU frames

## See the Code

Full source: [`examples/06_security_hmac.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/06_security_hmac.cpp)

```cpp
// Client and server share a secret
auto nonce_client = HMACAuth::generate_nonce();
auto nonce_server = HMACAuth::generate_nonce();

// Compute HMAC-SHA256
auto hmac = HMACAuth::compute(data, shared_key);

// Verify with constant-time comparison
bool valid = HMACAuth::verify(data, shared_key, expected_hmac);
```

---

Next: [Byte Order Safety](./endian-byte-orders.md)
