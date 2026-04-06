---
sidebar_position: 3
---

# Hello World Example

The simplest modbus_pp program: a client reads registers from a server over a loopback transport (in-process).

## Complete Code

```cpp
#include "modbus_pp/modbus_pp.hpp"

#include <iostream>
#include <thread>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

int main() {
    // 1. Create paired in-process loopback transports
    auto [client_transport, server_transport] = LoopbackTransport::create_pair();
    client_transport->connect();
    server_transport->connect();

    std::shared_ptr<Transport> srv_tp = std::move(server_transport);
    std::shared_ptr<Transport> cli_tp = std::move(client_transport);

    // 2. Set up server with simulated register storage
    std::vector<reg_t> registers(100, 0);
    registers[0] = 42;  // Pre-populate register 0 with value 42
    registers[1] = 100;

    ServerConfig srv_cfg{srv_tp, 1};  // unit_id = 1
    Server server(srv_cfg);

    // Register handler for reading holding registers
    server.on_read_holding_registers([&](address_t addr, quantity_t count) {
        return std::vector<reg_t>(registers.begin() + addr,
                                   registers.begin() + addr + count);
    });

    // 3. Run server in background thread
    std::atomic<bool> running{true};
    std::thread server_thread([&] {
        while (running.load()) {
            server.process_one();
        }
    });

    // 4. Create client and read registers
    ClientConfig cli_cfg{cli_tp};
    Client client(cli_cfg);

    std::cout << "Reading 2 registers from server...\n";
    auto result = client.read_holding_registers(1, 0, 2);

    if (result.has_value()) {
        std::cout << "Success! Values: [";
        auto& values = result.value();
        for (size_t i = 0; i < values.size(); ++i) {
            std::cout << values[i];
            if (i < values.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    } else {
        std::cerr << "Error: " << result.error().message() << "\n";
        return 1;
    }

    // 5. Cleanup
    running = false;
    server_thread.join();
    return 0;
}
```

## Compile & Run

### Using CMake

```bash
mkdir hello_world && cd hello_world
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.14)
project(hello_world)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(modbus_pp REQUIRED)

add_executable(hello_world main.cpp)
target_link_libraries(hello_world modbus_pp::modbus_pp)
EOF

cmake .. -DMODBUS_PP_ROOT=/path/to/modbus_pp/install
cmake --build .
./hello_world
```

### Using Compiler Directly

```bash
g++ -std=c++17 main.cpp \
  -I/path/to/modbus_pp/include \
  -L/path/to/modbus_pp/lib \
  -lmodbus_pp -lssl -lcrypto \
  -o hello_world
./hello_world
```

## Expected Output

```
Reading 2 registers from server...
Success! Values: [42, 100]
```

## Breaking Down the Code

| Step | What Happens                                                                            |
| ---- | --------------------------------------------------------------------------------------- |
| 1    | Create **loopback transport pair** — two in-process endpoints for client/server         |
| 2    | Initialize **server** with register storage and read handler                            |
| 3    | Launch **server event loop** in background thread (processes one request per iteration) |
| 4    | Create **client** and send read request (FC 0x03 = Read Holding Registers)              |
| 5    | **Cleanup** — stop server thread and exit                                               |

## Key Libraries Used

- **Transport**: `LoopbackTransport` — paired in-process endpoints
- **Facades**: `Client`, `Server` — high-level APIs
- **Types**: `register_t`, `address_t`, `quantity_t` — Modbus primitives
- **Error Handling**: `Result<T>` — monadic error type (no exceptions)

## What's Happening Under the Hood

1. Client calls `client.read_holding_registers(unit_id=1, addr=0, count=2)`
2. Client creates: PDU with FC=0x03, serializes to bytes, sends via transport
3. Server receives frame, parses PDU, looks up handler, reads from register storage
4. Server creates response PDU, serializes, sends back via transport
5. Client receives, deserializes, returns `Result<vector<register_t>>` to caller

**Zero exceptions, zero allocations in hot path** ✅

## Next Steps

Ready for more? Pick one:

- **Interactive examples**: See all [7 Examples](../examples/basic-client-server.md)
- **Core concepts**: Read [Modbus Fundamentals](../concepts/modbus-fundamentals.md)
- **Advanced features**: Learn about [Security](../features/security-hmac.md), [Pipelining](../features/pipelining.md)
