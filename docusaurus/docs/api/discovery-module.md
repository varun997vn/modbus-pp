---
sidebar_position: 6
---

# Discovery Module API

## Overview

The Discovery module provides device discovery and capability detection for Modbus networks.

## Classes

### `BroadcastScanner`

Network-wide device discovery.

```cpp
class BroadcastScanner {
public:
  Result<std::vector<DeviceInfo>> scanNetwork(std::chrono::seconds timeout);
};
```

### `DeviceInfo`

Device metadata and capabilities.

```cpp
struct DeviceInfo {
  uint8_t unitId;
  std::string vendorName;
  std::string deviceModel;
  uint16_t numberOfRegisters;
  std::vector<FunctionCode> supportedFunctions;
};
```

### `CapabilityDetector`

Runtime capability discovery.

```cpp
class CapabilityDetector {
public:
  Result<Capabilities> detect(Transport& transport);
};
```

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/discovery)
