#pragma once

/// @file tcp_transport.hpp
/// @brief Modbus TCP transport using POSIX sockets.

#include "transport.hpp"

#include <string>

namespace modbus_pp {

/// TCP transport configuration.
struct TCPConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{502};
    std::chrono::milliseconds connect_timeout{3000};
    std::chrono::milliseconds read_timeout{1000};
    bool tcp_nodelay{true};
};

/// Modbus TCP transport wrapping POSIX sockets.
///
/// Implements the Transport interface for TCP connections with
/// MBAP header framing.
class TCPTransport final : public Transport {
public:
    explicit TCPTransport(TCPConfig config);
    ~TCPTransport() override;

    TCPTransport(const TCPTransport&) = delete;
    TCPTransport& operator=(const TCPTransport&) = delete;

    Result<void> send(span_t<const byte_t> frame) override;
    Result<std::vector<byte_t>> receive(
        std::chrono::milliseconds timeout = std::chrono::milliseconds{1000}) override;
    Result<void> connect() override;
    void disconnect() override;
    [[nodiscard]] bool is_connected() const noexcept override;
    [[nodiscard]] std::string_view transport_name() const noexcept override;

private:
    TCPConfig config_;
    int socket_fd_{-1};
    bool connected_{false};
};

} // namespace modbus_pp
