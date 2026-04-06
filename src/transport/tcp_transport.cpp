#include "modbus_pp/transport/tcp_transport.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace modbus_pp {

TCPTransport::TCPTransport(TCPConfig config) : config_(std::move(config)) {}

TCPTransport::~TCPTransport() {
    disconnect();
}

Result<void> TCPTransport::connect() {
    if (connected_) return Result<void>{};

    socket_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) return ErrorCode::TransportDisconnected;

    // Set non-blocking for connect timeout
    int flags = ::fcntl(socket_fd_, F_GETFL, 0);
    ::fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.port);
    if (::inet_pton(AF_INET, config_.host.c_str(), &addr.sin_addr) <= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        return ErrorCode::TransportDisconnected;
    }

    int ret = ::connect(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        return ErrorCode::TransportDisconnected;
    }

    if (ret < 0) {
        // Wait for connection with timeout
        struct pollfd pfd{};
        pfd.fd = socket_fd_;
        pfd.events = POLLOUT;
        int poll_ret = ::poll(&pfd, 1, static_cast<int>(config_.connect_timeout.count()));
        if (poll_ret <= 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            return ErrorCode::TimeoutExpired;
        }

        int err = 0;
        socklen_t len = sizeof(err);
        ::getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err != 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            return ErrorCode::TransportDisconnected;
        }
    }

    // Restore blocking mode
    ::fcntl(socket_fd_, F_SETFL, flags);

    // Set TCP_NODELAY
    if (config_.tcp_nodelay) {
        int one = 1;
        ::setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }

    connected_ = true;
    return Result<void>{};
}

void TCPTransport::disconnect() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

bool TCPTransport::is_connected() const noexcept {
    return connected_;
}

std::string_view TCPTransport::transport_name() const noexcept {
    return "tcp";
}

Result<void> TCPTransport::send(span_t<const byte_t> frame) {
    if (!connected_) return ErrorCode::TransportDisconnected;

    std::size_t total_sent = 0;
    while (total_sent < frame.size()) {
        auto sent = ::write(socket_fd_, frame.data() + total_sent,
                             frame.size() - total_sent);
        if (sent <= 0) {
            connected_ = false;
            return ErrorCode::TransportDisconnected;
        }
        total_sent += static_cast<std::size_t>(sent);
    }
    return Result<void>{};
}

Result<std::vector<byte_t>> TCPTransport::receive(std::chrono::milliseconds timeout) {
    if (!connected_) return ErrorCode::TransportDisconnected;

    struct pollfd pfd{};
    pfd.fd = socket_fd_;
    pfd.events = POLLIN;

    int poll_ret = ::poll(&pfd, 1, static_cast<int>(timeout.count()));
    if (poll_ret < 0) {
        connected_ = false;
        return ErrorCode::TransportDisconnected;
    }
    if (poll_ret == 0) return ErrorCode::TimeoutExpired;

    // Read MBAP header first (7 bytes) to determine frame length
    byte_t header[7];
    std::size_t header_read = 0;
    while (header_read < 7) {
        auto n = ::read(socket_fd_, header + header_read, 7 - header_read);
        if (n <= 0) {
            connected_ = false;
            return ErrorCode::TransportDisconnected;
        }
        header_read += static_cast<std::size_t>(n);
    }

    // Extract length from MBAP header
    auto pdu_length = static_cast<std::size_t>(
        (static_cast<std::uint16_t>(header[4]) << 8) | header[5]);

    // pdu_length includes unit_id (1 byte), so remaining = pdu_length - 1
    if (pdu_length < 1) return ErrorCode::InvalidFrame;
    auto remaining = pdu_length - 1;

    std::vector<byte_t> frame(7 + remaining);
    std::memcpy(frame.data(), header, 7);

    std::size_t body_read = 0;
    while (body_read < remaining) {
        auto n = ::read(socket_fd_, frame.data() + 7 + body_read,
                         remaining - body_read);
        if (n <= 0) {
            connected_ = false;
            return ErrorCode::TransportDisconnected;
        }
        body_read += static_cast<std::size_t>(n);
    }

    return frame;
}

} // namespace modbus_pp
