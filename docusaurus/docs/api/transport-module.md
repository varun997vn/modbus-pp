---
sidebar_position: 2
---

# Transport Module API

## Overview

The Transport module provides abstract interfaces for different Modbus communication protocols (TCP, RTU, Loopback).

## Classes

### `Transport`

Base class for Modbus transport implementations.

```cpp
class Transport {
public:
  virtual ~Transport() = default;
  virtual Result<Frame> send(const Frame& frame) = 0;
  virtual Result<Frame> receive() = 0;
};
```

### `TcpTransport`

TCP/IP transport implementation.

### `RtuTransport`

RTU (serial) transport implementation.

### `LoopbackTransport`

In-memory loopback transport for testing.

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/transport)
