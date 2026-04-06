---
sidebar_position: 3
---

# Security Module API

## Overview

The Security module provides HMAC-based authentication and nonce generation for secure Modbus communication.

## Classes

### `HmacSha256`

HMAC-SHA256 authentication class.

```cpp
class HmacSha256 {
public:
  static Result<Digest> compute(const std::string& secret, const Data& message);
  static bool verify(const Digest& expected, const Digest& computed);
};
```

### `NonceGenerator`

Cryptographically secure nonce generation.

```cpp
class NonceGenerator {
public:
  static uint64_t generate();
};
```

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/security)
