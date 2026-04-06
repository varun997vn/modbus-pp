#include "modbus_pp/transport/loopback_transport.hpp"

#include <random>
#include <thread>

namespace modbus_pp {

LoopbackTransport::LoopbackTransport(std::shared_ptr<LoopbackQueue> tx,
                                     std::shared_ptr<LoopbackQueue> rx)
    : tx_queue_(std::move(tx)), rx_queue_(std::move(rx)) {}

std::pair<std::unique_ptr<LoopbackTransport>, std::unique_ptr<LoopbackTransport>>
LoopbackTransport::create_pair() {
    auto q1 = std::make_shared<LoopbackQueue>();
    auto q2 = std::make_shared<LoopbackQueue>();

    // Client: sends to q1, receives from q2
    // Server: sends to q2, receives from q1
    auto client = std::unique_ptr<LoopbackTransport>(new LoopbackTransport(q1, q2));
    auto server = std::unique_ptr<LoopbackTransport>(new LoopbackTransport(q2, q1));

    return {std::move(client), std::move(server)};
}

Result<void> LoopbackTransport::send(span_t<const byte_t> frame) {
    if (!connected_) {
        return ErrorCode::TransportDisconnected;
    }

    // Check injected errors
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        if (next_error_.has_value()) {
            auto ec = *next_error_;
            next_error_.reset();
            return ec;
        }
        if (error_rate_ > 0.0) {
            thread_local std::mt19937 rng{std::random_device{}()};
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < error_rate_) {
                return ErrorCode::TransportDisconnected;
            }
        }
    }

    // Simulated latency
    auto latency = simulated_latency_;
    if (latency.count() > 0) {
        std::this_thread::sleep_for(latency);
    }

    std::vector<byte_t> data(frame.begin(), frame.end());
    {
        std::lock_guard<std::mutex> lock(tx_queue_->mutex);
        tx_queue_->messages.push_back(std::move(data));
    }
    tx_queue_->cv.notify_one();

    return Result<void>{};
}

Result<std::vector<byte_t>> LoopbackTransport::receive(std::chrono::milliseconds timeout) {
    if (!connected_) {
        return ErrorCode::TransportDisconnected;
    }

    std::unique_lock<std::mutex> lock(rx_queue_->mutex);
    if (!rx_queue_->cv.wait_for(lock, timeout,
            [this] { return !rx_queue_->messages.empty(); })) {
        return ErrorCode::TimeoutExpired;
    }

    auto msg = std::move(rx_queue_->messages.front());
    rx_queue_->messages.pop_front();
    return msg;
}

Result<void> LoopbackTransport::connect() {
    connected_ = true;
    return Result<void>{};
}

void LoopbackTransport::disconnect() {
    connected_ = false;
    // Wake any blocked receivers
    tx_queue_->cv.notify_all();
    rx_queue_->cv.notify_all();
}

bool LoopbackTransport::is_connected() const noexcept {
    return connected_;
}

std::string_view LoopbackTransport::transport_name() const noexcept {
    return "loopback";
}

void LoopbackTransport::set_simulated_latency(std::chrono::microseconds latency) {
    simulated_latency_ = latency;
}

void LoopbackTransport::set_error_rate(double rate) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    error_rate_ = rate;
}

void LoopbackTransport::set_next_error(std::error_code ec) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    next_error_ = ec;
}

} // namespace modbus_pp
