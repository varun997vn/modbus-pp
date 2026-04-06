---
sidebar_position: 8
---

# Client/Server Facades API

## Overview

High-level facade classes that combine transport, security, and pipelining into easy-to-use interfaces.

## Classes

### `ModbusClient`

Unified client facade.

```cpp
class ModbusClient {
public:
  Result<uint16_t> readRegister(uint16_t address);
  Result<std::vector<uint16_t>> readRegisters(uint16_t address, uint16_t count);
  Result<void> writeRegister(uint16_t address, uint16_t value);
  Result<void> writeRegisters(uint16_t address, const std::vector<uint16_t>& values);
};
```

### `ModbusServer`

Unified server facade.

```cpp
class ModbusServer {
public:
  void start(uint16_t port);
  void stop();
  void setRegisterValue(uint16_t address, uint16_t value);
  uint16_t getRegisterValue(uint16_t address);
};
```

### `SecureClient`

Client with HMAC authentication.

```cpp
class SecureClient : public ModbusClient {
public:
  explicit SecureClient(const std::string& secret);
  Result<uint16_t> secureRead(uint16_t address);
};
```

### `SecureServer`

Server with HMAC authentication.

```cpp
class SecureServer : public ModbusServer {
public:
  explicit SecureServer(const std::string& secret);
};
```

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/client)
