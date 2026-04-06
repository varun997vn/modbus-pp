#pragma once

/// @file rtu_transport.hpp
/// @brief Modbus RTU transport using serial ports (termios).

#include "transport.hpp"

#include <string>

namespace modbus_pp {

/// RTU transport configuration.
struct RTUConfig {
    std::string device{"/dev/ttyUSB0"};
    std::uint32_t baud_rate{9600};
    std::uint8_t data_bits{8};
    char parity{'N'}; // N, E, O
    std::uint8_t stop_bits{1};
    std::chrono::milliseconds read_timeout{1000};
};

/// Modbus RTU transport over serial ports.
///
/// Uses POSIX termios for serial configuration. Frames include
/// CRC16 appended/verified by FrameCodec.
class RTUTransport final : public Transport {
public:
    explicit RTUTransport(RTUConfig config);
    ~RTUTransport() override;

    RTUTransport(const RTUTransport&) = delete;
    RTUTransport& operator=(const RTUTransport&) = delete;

    Result<void> send(span_t<const byte_t> frame) override;
    Result<std::vector<byte_t>> receive(
        std::chrono::milliseconds timeout = std::chrono::milliseconds{1000}) override;
    Result<void> connect() override;
    void disconnect() override;
    [[nodiscard]] bool is_connected() const noexcept override;
    [[nodiscard]] std::string_view transport_name() const noexcept override;

private:
    RTUConfig config_;
    int fd_{-1};
    bool connected_{false};
};

} // namespace modbus_pp
