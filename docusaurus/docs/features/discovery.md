---
sidebar_position: 6
---

# Device Discovery

Auto-scan the Modbus bus to discover devices and their capabilities.

## The Problem

Manual discovery is slow and unreliable:

```cpp
for (int addr = 1; addr <= 247; ++addr) {
    try {
        // Probe address...maybe it exists?
    } catch (...) {
        // Doesn't exist?
    }
}
```

## The Solution: Broadcast Scanner

Parallel discovery with capability detection:

```cpp
Scanner scanner(transport, {
    .scan_timeout = 500ms,
    .range_start = 1,
    .range_end = 247,
});

auto devices = scanner.scan();

for (const auto& device : devices) {
    std::cout << "Unit #" << device.unit_id << ": "
              << device.vendor_name << " "
              << device.model_name << "\n"
              << "Supports modbus_pp: "
              << (device.supports_extended ? "yes" : "no") << "\n";
}
```

## Device Information

Returned `DeviceInfo` includes:

- **unit_id** — Slave address
- **vendor_name** — Manufacturer (Siemens, Emerson, etc.)
- **model_name** — Device model
- **firmware_version** — FW version
- **supports_extended** — Supports modbus_pp extended frames?
- **capabilities** — Bitmap of supported extended features

## Full Example

See [examples/discovery.cpp](https://github.com/varuvenk/modbus_pp/blob/main/examples/08_device_discovery.cpp).

---

**Next:** [Batch Operations](./batch-operations.md) or back to [Features Overview](./overview.md)
