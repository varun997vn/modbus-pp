---
sidebar_position: 1
---

# Basic Client/Server Communication

The simplest end-to-end example: a client reads and writes registers from a server.

## Overview

This example demonstrates:

- Creating a single loopback transport pair
- Setting up a server with a simulated register file
- Launching a server event loop in a background thread
- Creating a client and performing standard Modbus read/write operations

## See the Code

Full source: [`examples/01_basic_client_server.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/01_basic_client_server.cpp)

```cpp
#include "modbus_pp/modbus_pp.hpp"
// ... setup loopback, server, client ...
auto result = client.read_holding_registers(unit_id, 0, 2);
if (result) {
    std::cout << "Values: [" << result.value()[0] << ", "
              << result.value()[1] << "]\n";
}
```

## Key Learnings

- `LoopbackTransport::create_pair()` creates paired in-process endpoints
- Server's `process_one()` handles one request per call
- Client's `read_holding_registers()` returns `Result<vector<register_t>>`
- Error handling via `Result<T>` (no exceptions)

---

Next example: [Typed Registers](./typed-registers-example.md)
