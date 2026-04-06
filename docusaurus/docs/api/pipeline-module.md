---
sidebar_position: 4
---

# Pipeline Module API

## Overview

The Pipeline module provides pipelined request/response handling for concurrent Modbus operations.

## Classes

### `PipelineRequest`

Request wrapper with correlation ID.

```cpp
struct PipelineRequest {
  uint16_t correlationId;
  Pdu pdu;
  std::chrono::milliseconds timeout;
};
```

### `PipelineClient`

High-level pipelined client interface.

```cpp
class PipelineClient {
public:
  Result<std::vector<Response>> sendPipelined(const std::vector<Request>& requests);
};
```

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/pipeline)
