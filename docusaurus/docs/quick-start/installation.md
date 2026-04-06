---
sidebar_position: 2
---

# Installation

## Prerequisites

- **C++17** compiler (GCC 7+, Clang 6+, MSVC 2017+)
- **CMake** 3.14+
- **OpenSSL** (for HMAC-SHA256)
- **Python 3.8+** (optional, for benchmarks)

### On Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  libssl-dev \
  git
```

### On macOS

```bash
brew install cmake openssl
```

### On Windows

Download and install:

- [Visual Studio 2017+](https://visualstudio.microsoft.com/)
- [CMake](https://cmake.org/download/)
- [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html) or [vcpkg](https://github.com/Microsoft/vcpkg)

## Build from Source

### 1. Clone the Repository

```bash
git clone https://github.com/varuvenk/modbus_pp.git
cd modbus_pp
```

### 2. Create Build Directory

```bash
mkdir build
cd build
```

### 3. Configure with CMake

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DMODBUS_PP_BUILD_TESTS=ON \
  -DMODBUS_PP_BUILD_EXAMPLES=ON \
  -DMODBUS_PP_BUILD_BENCHMARKS=ON
```

**CMake Options:**

| Option                       | Default | Description                      |
| ---------------------------- | ------- | -------------------------------- |
| `MODBUS_PP_BUILD_TESTS`      | `ON`    | Build unit and integration tests |
| `MODBUS_PP_BUILD_EXAMPLES`   | `ON`    | Build 7 example programs         |
| `MODBUS_PP_BUILD_BENCHMARKS` | `ON`    | Build 5 benchmark programs       |
| `MODBUS_PP_ENABLE_ASAN`      | `OFF`   | Enable AddressSanitizer          |
| `MODBUS_PP_ENABLE_TSAN`      | `OFF`   | Enable ThreadSanitizer           |

### 4. Build

```bash
cmake --build . --config Release -j$(nproc)
```

### 5. Run Tests (Optional)

```bash
ctest --output-on-failure
```

### 6. Install (Optional)

```bash
cmake --install . --prefix /usr/local
```

## Verify Installation

Run the first example:

```bash
./bin/01_simple_pipeline
```

Expected output:

```
=== modbus_pp Example 01: Basic Client/Server ===

[setup] Loopback transport pair created and connected.
...
[client] Read holding registers (0x0000, count=2)
[response] Values: [1, 2]
```

## Using in Your Project

### Option 1: CMake `find_package()`

After installing modbus_pp:

```cmake
find_package(modbus_pp REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app modbus_pp::modbus_pp)
```

### Option 2: Header-Only (Recommended for small projects)

Copy `include/modbus_pp/` to your project and include:

```cpp
#include "modbus_pp/modbus_pp.hpp"

using namespace modbus_pp;
```

### Option 3: Git Submodule + CMake `add_subdirectory()`

```bash
git submodule add https://github.com/varuvenk/modbus_pp.git external/modbus_pp
```

In your `CMakeLists.txt`:

```cmake
add_subdirectory(external/modbus_pp)
add_executable(my_app main.cpp)
target_link_libraries(my_app modbus_pp)
```

## Troubleshooting

### "OpenSSL not found"

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl
# Then specify the path:
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

### "C++17 not available"

Ensure your compiler is C++17-capable:

```bash
g++ --version  # Should be 7.0+
clang++ --version  # Should be 6.0+
```

Update if needed, then specify:

```cmake
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```

### "CMake version too old"

```bash
cmake --version  # Should be 3.14+
sudo apt-get install -y cmake  # Install latest
```

---

**Done!** → Next: [Hello World Example](./hello-world.md)
