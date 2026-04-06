---
sidebar_position: 4
---

# Design Patterns

Key C++ patterns used throughout modbus_pp.

## 1. Monadic Error Handling (`Result<T>`)

Instead of exceptions, functions return `Result<T>`:

```cpp
template <typename T>
class Result {
    std::variant<T, std::error_code> storage;
public:
    bool has_value() const;
    T& value();
    std::error_code error();

    template <typename F>
    `Result<U>` map(F fn);  // Transform value

    template <typename F>
    `Result<U>` and_then(F fn);  // Chain operations
};
```

**Benefits:**

- No exceptions in hot path
- Composable error handling
- Predictable control flow

## 2. Compile-Time Type Safety (StrongType)

Prevent mixing semantically different values:

```cpp
template <typename Tag, typename T>
class StrongType {
    T value_;
public:
    explicit StrongType(T v) : value_(v) {}
    T raw() const { return value_; }
};

// Usage:
using RegisterAddress = StrongType<struct RegisterAddressTag, uint16_t>;
using RegisterCount = StrongType<struct RegisterCountTag, uint16_t>;

RegisterAddress addr(100);
RegisterCount count(10);
// addr and count are incompatible types (won't compile if swapped!)
```

## 3. CRTP for Static Polymorphism (Transport Layer)

Abstract interface via inheritance + `constexpr if`:

```cpp
// Not fully virtual (avoids vtable overhead)
class Transport {
    virtual Result<void> send(...) = 0;  // Only for interface spec
};

class TCPTransport : public Transport { ... };
class RTUTransport : public Transport { ... };
```

## 4. Compile-Time Dispatch (constexpr if for byte orders)

```cpp
template <ByteOrder Order, size_t N>
inline std::array<byte_t, N> reorder(std::array<byte_t, N> bytes) noexcept {
    if constexpr (Order == ByteOrder::ABCD) {
        return bytes;  // no-op, compiles away
    } else if constexpr (Order == ByteOrder::DCBA) {
        return {bytes[3], bytes[2], bytes[1], bytes[0]};  // BSWAP
    }
    // ... other orders
}
```

Compiler generates optimal code for each order. Unused orderings vanish.

## 5. Zero-Cost Abstractions (Template Specialization)

```cpp
// TypeCodec generates optimal encode/decode for each type & byte order
template <RegisterType Type, ByteOrder Order>
struct TypeCodec { ... };

// Generates optimal machine code for each combination
// Unused combinations don't increase binary size (linker removes them)
```

## 6. RAII for Resource Management

```cpp
class LoopbackTransport {
    std::unique_ptr<...> client_endpoint_;
    std::unique_ptr<...> server_endpoint_;

public:
    ~LoopbackTransport() { /* auto cleanup via unique_ptr */ }
};
```

---

**Next:** [Transport Abstraction](./transport-abstraction.md)
