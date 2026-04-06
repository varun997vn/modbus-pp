#pragma once

/// @file scanner.hpp
/// @brief Broadcast device discovery with parallel probing.
///
/// Scans a range of Modbus unit addresses to discover devices,
/// probing each with a Read Device Identification request (FC 0x2B/0x0E).
/// For standard devices that respond, basic info is recorded. For
/// devices supporting modbus_pp extensions, capability detection is performed.

#include "device_info.hpp"
#include "../core/result.hpp"
#include "../transport/transport.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace modbus_pp {

/// Configuration for the device scanner.
struct ScannerConfig {
    std::chrono::milliseconds scan_timeout{500};
    unit_id_t range_start{1};
    unit_id_t range_end{247};
};

/// Scans for Modbus devices on the bus.
class Scanner {
public:
    explicit Scanner(std::shared_ptr<Transport> transport, ScannerConfig config = {});

    /// Perform a blocking scan of the configured address range.
    /// @return List of discovered devices.
    std::vector<DeviceInfo> scan();

    /// Probe a single device address.
    Result<DeviceInfo> probe(unit_id_t unit_id);

    /// Async scan with per-device callback.
    void scan_async(std::function<void(DeviceInfo)> on_found,
                     std::function<void()> on_complete);

private:
    std::shared_ptr<Transport> transport_;
    ScannerConfig config_;
};

} // namespace modbus_pp
