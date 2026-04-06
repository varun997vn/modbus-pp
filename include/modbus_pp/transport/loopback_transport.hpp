#pragma once

/// @file loopback_transport.hpp
/// @brief In-process paired loopback transport for testing.
///
/// Creates two linked transport endpoints: data sent on one appears as
/// received on the other. Supports simulated latency and error injection
/// for realistic integration testing and benchmarking.

#include "transport.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace modbus_pp {

/// Thread-safe message queue shared between paired LoopbackTransport endpoints.
struct LoopbackQueue {
    std::mutex              mutex;
    std::condition_variable cv;
    std::deque<std::vector<byte_t>> messages;
};

/// In-process transport that pairs two endpoints for testing.
///
/// Usage:
/// ```cpp
/// auto [client, server] = LoopbackTransport::create_pair();
/// client->connect();
/// server->connect();
/// client->send(data);         // appears at server->receive()
/// server->send(response);     // appears at client->receive()
/// ```
class LoopbackTransport final : public Transport {
public:
    /// Create a paired client/server loopback transport.
    static std::pair<std::unique_ptr<LoopbackTransport>,
                     std::unique_ptr<LoopbackTransport>>
    create_pair();

    ~LoopbackTransport() override = default;

    // Transport interface
    Result<void> send(span_t<const byte_t> frame) override;
    Result<std::vector<byte_t>> receive(
        std::chrono::milliseconds timeout = std::chrono::milliseconds{1000}) override;
    Result<void> connect() override;
    void disconnect() override;
    [[nodiscard]] bool is_connected() const noexcept override;
    [[nodiscard]] std::string_view transport_name() const noexcept override;

    /// Inject simulated latency (applied on send).
    void set_simulated_latency(std::chrono::microseconds latency);

    /// Set a probability (0.0–1.0) of a send failing with TransportDisconnected.
    void set_error_rate(double rate);

    /// Force the next send to fail with the given error code.
    void set_next_error(std::error_code ec);

private:
    LoopbackTransport(std::shared_ptr<LoopbackQueue> tx,
                      std::shared_ptr<LoopbackQueue> rx);

    std::shared_ptr<LoopbackQueue> tx_queue_;
    std::shared_ptr<LoopbackQueue> rx_queue_;
    std::atomic<bool> connected_{false};
    std::chrono::microseconds simulated_latency_{0};
    double error_rate_{0.0};
    std::optional<std::error_code> next_error_;
    std::mutex config_mutex_;
};

} // namespace modbus_pp
