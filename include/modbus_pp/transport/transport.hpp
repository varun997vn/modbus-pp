#pragma once

/// @file transport.hpp
/// @brief Abstract transport interface for Modbus communication.
///
/// Separates protocol logic from physical transport, enabling the same
/// modbus_pp code to work over TCP, serial (RTU), or an in-process
/// loopback for testing.

#include "../core/result.hpp"
#include "../core/types.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace modbus_pp {

/// Abstract base class for Modbus transport layers.
///
/// Implementations must provide synchronous send/receive.
/// The transport owns the connection lifecycle (connect/disconnect).
class Transport {
public:
    virtual ~Transport() = default;

    /// Send a complete frame (ADU) over the transport.
    virtual Result<void> send(span_t<const byte_t> frame) = 0;

    /// Receive a complete frame (ADU) from the transport.
    /// @param timeout  Maximum time to wait for a response.
    virtual Result<std::vector<byte_t>> receive(
        std::chrono::milliseconds timeout = std::chrono::milliseconds{1000}) = 0;

    /// Establish the connection.
    virtual Result<void> connect() = 0;

    /// Close the connection.
    virtual void disconnect() = 0;

    /// @return true if the transport is currently connected.
    [[nodiscard]] virtual bool is_connected() const noexcept = 0;

    /// @return A human-readable transport name (e.g., "tcp", "rtu", "loopback").
    [[nodiscard]] virtual std::string_view transport_name() const noexcept = 0;
};

} // namespace modbus_pp
