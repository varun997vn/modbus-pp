---
sidebar_position: 3
---

# Failure Propagation & Error Handling

How to properly handle errors with `Result<T>`.

## Overview

Demonstrates:

- Returning `Result<T>` from functions
- Chaining operations with `and_then()` and `map()`
- Error code inspection
- Pattern matching on success/failure

## See the Code

Full source: [`examples/03_failure_propagation.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/03_failure_propagation.cpp)

```cpp
auto result = client.read_holding_registers(unit, addr, count)
    .and_then([](auto& regs) { return validate(regs); })
    .map([](auto& regs) { return regs[0]; })
    .value_or(default_value);
```

---

Next: [Pipeline Throughput](./pipeline-throughput.md)
