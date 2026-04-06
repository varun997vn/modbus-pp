---
sidebar_position: 7
---

# Register Map Module API

## Overview

The Register Map module provides type-safe register access with compile-time validation.

## Classes

### `RegisterDescriptor`

Type-safe register definition.

```cpp
template <typename T>
class RegisterDescriptor {
public:
  uint16_t address;
  std::string name;
  ByteOrder byteOrder;
  AccessType accessType;  // Read, Write, ReadWrite
};
```

### `RegisterMap`

Collection of typed register definitions.

```cpp
class RegisterMap {
public:
  void add(const RegisterDescriptor& desc);
  Result<Value> read(Transport& transport, const std::string& name);
  Result<void> write(Transport& transport, const std::string& name, const Value& value);
};
```

### `StrongType`

Compile-time type safety wrapper.

```cpp
template <typename T, typename Tag>
class StrongType {
public:
  explicit StrongType(T value);
  T get() const;
};
```

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/core)
