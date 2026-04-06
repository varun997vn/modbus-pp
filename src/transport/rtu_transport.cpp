#include "modbus_pp/transport/rtu_transport.hpp"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace modbus_pp {

RTUTransport::RTUTransport(RTUConfig config) : config_(std::move(config)) {}

RTUTransport::~RTUTransport() {
    disconnect();
}

Result<void> RTUTransport::connect() {
    if (connected_) return Result<void>{};

    fd_ = ::open(config_.device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) return ErrorCode::TransportDisconnected;

    struct termios tty{};
    if (::tcgetattr(fd_, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        return ErrorCode::TransportDisconnected;
    }

    // Baud rate
    speed_t speed;
    switch (config_.baud_rate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        default:     speed = B9600;   break;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // Data bits
    tty.c_cflag &= ~CSIZE;
    switch (config_.data_bits) {
        case 7: tty.c_cflag |= CS7; break;
        case 8: tty.c_cflag |= CS8; break;
        default: tty.c_cflag |= CS8; break;
    }

    // Parity
    switch (config_.parity) {
        case 'E':
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
        case 'O':
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            break;
        default:
            tty.c_cflag &= ~PARENB;
            break;
    }

    // Stop bits
    if (config_.stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }

    // No flow control
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    // Raw mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT |
                      PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    // Timeout
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // 0.1 second intervals

    if (::tcsetattr(fd_, TCSANOW, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        return ErrorCode::TransportDisconnected;
    }

    // Flush
    ::tcflush(fd_, TCIOFLUSH);

    // Switch back to blocking
    int flags = ::fcntl(fd_, F_GETFL, 0);
    ::fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);

    connected_ = true;
    return Result<void>{};
}

void RTUTransport::disconnect() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    connected_ = false;
}

bool RTUTransport::is_connected() const noexcept {
    return connected_;
}

std::string_view RTUTransport::transport_name() const noexcept {
    return "rtu";
}

Result<void> RTUTransport::send(span_t<const byte_t> frame) {
    if (!connected_) return ErrorCode::TransportDisconnected;

    auto written = ::write(fd_, frame.data(), frame.size());
    if (written < 0 || static_cast<std::size_t>(written) != frame.size()) {
        return ErrorCode::TransportDisconnected;
    }

    // Wait for transmission to complete
    ::tcdrain(fd_);
    return Result<void>{};
}

Result<std::vector<byte_t>> RTUTransport::receive(std::chrono::milliseconds timeout) {
    if (!connected_) return ErrorCode::TransportDisconnected;

    struct pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

    int poll_ret = ::poll(&pfd, 1, static_cast<int>(timeout.count()));
    if (poll_ret < 0) return ErrorCode::TransportDisconnected;
    if (poll_ret == 0) return ErrorCode::TimeoutExpired;

    // Read available data with inter-character timeout detection
    std::vector<byte_t> buffer;
    buffer.reserve(256);
    byte_t tmp[256];

    while (true) {
        auto n = ::read(fd_, tmp, sizeof(tmp));
        if (n > 0) {
            buffer.insert(buffer.end(), tmp, tmp + n);
        }

        // Check for more data with a short timeout (3.5 character times)
        // At 9600 baud, 1 char ≈ 1.04ms, so 3.5 chars ≈ 3.6ms
        int more = ::poll(&pfd, 1, 5);
        if (more <= 0) break;
    }

    if (buffer.empty()) return ErrorCode::TimeoutExpired;
    return buffer;
}

} // namespace modbus_pp
