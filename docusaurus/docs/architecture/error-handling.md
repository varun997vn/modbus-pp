---
sidebar_position: 6
---

# Error Handling

How modbus_pp reports and handles errors.

## The Problem with Exceptions

Standard exceptions in C++:

- Add overhead to hot paths
- Complicate stack unwinding
- Not suitable for real-time systems
- Hard to use with -fno-exceptions

## The Solution: `Result<T>`

All fallible operations return `Result<T>`:

```cpp
template <typename T>
class Result {
public:
    // Construct with value or error
    Result(T value);
    Result(std::error_code error);
    Result(ErrorCode error);

    // Query state
    bool has_value() const;
    explicit operator bool() const;

    // Access value or error
    T& value();
    std::error_code error();

    // Monadic operations
    template <typename F>
    `Result<U>` map(F fn) const;

    template <typename F>
    `Result<U>` and_then(F fn) const;

    T value_or(T default_value) const;
};
```

## Error Codes

25+ error codes organized in two groups:

### Standard Modbus Exceptions (1-11)

From Modbus specification:

```cpp
enum class ErrorCode : int {
    Success = 0,
    IllegalFunction = 1,          // Invalid function code
    IllegalDataAddress = 2,       // Address out of range
    IllegalDataValue = 3,         // Invalid data
    SlaveDeviceFailure = 4,       // Slave malfunction
    Acknowledge = 5,              // Request accepted, processing
    SlaveDeviceBusy = 6,          // Slave busy
    NegativeAcknowledge = 7,      // Slave rejected request
    MemoryParityError = 8,        // Parity error in slave
    GatewayPathUnavailable = 0x0A,
    GatewayTargetFailed = 0x0B,
};
```

### Extended modbus_pp Errors (0x80+)

```cpp
    // Extended errors
    AuthenticationFailed = 0x80,
    SessionExpired = 0x81,
    HMACMismatch = 0x82,
    PipelineOverflow = 0x83,
    SubscriptionLimitReached = 0x84,
    PayloadTooLarge = 0x85,
    TransportDisconnected = 0x87,
    TimeoutExpired = 0x88,
    InvalidFrame = 0x8D,
    CrcMismatch = 0x8E,
    // ... more
```

## Usage Patterns

### Pattern 1: Check and Handle

```cpp
auto result = client.read_holding_registers(unit, addr, count);

if (result) {
    // Success path
    auto& values = result.value();
    std::cout << "Read " << values.size() << " registers\n";
} else {
    // Error path
    auto error = result.error();
    std::cerr << "Error: " << error.message() << "\n";
    std::cerr << "Category: " << error.category().name() << "\n";
}
```

### Pattern 2: Chain Operations

```cpp
auto final = read_pdu(data)
    .and_then([](PDU& pdu) { return validate(pdu); })
    .map([](PDU& pdu) { return pdu.payload(); })
    .value_or(default_payload);
```

### Pattern 3: Propagate Errors

```cpp
Result<RegisterValue> get_register(address_t addr) {
    auto pdu = client.read_register(1, addr, 1);
    if (!pdu) return pdu.error();  // Propagate error

    return parse_register(pdu.value());
}
```

## std::error_code Integration

All modbus_pp error codes are registered with the standard `std::error_code` system:

```cpp
std::error_code ec = make_error_code(ErrorCode::TimeoutExpired);

std::cout << ec.message() << "\n";  // "Timeout expired"
std::cout << ec.category().name() << "\n";  // "modbus_pp"

// Works with standard library
if (ec == ErrorCode::TimeoutExpired) { ... }
```

---

**Next:** Back to [Architecture Overview](./overview.md) or jump to [API Reference](../api/core-module.md)
