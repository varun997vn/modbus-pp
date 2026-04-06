#pragma once

/// @file request_queue.hpp
/// @brief Thread-safe in-flight request tracking for the pipeline.

#include "correlation_id.hpp"
#include "../core/pdu.hpp"
#include "../core/result.hpp"
#include "../core/timestamp.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace modbus_pp {

/// A pending request awaiting its response in the pipeline.
struct PendingRequest {
    CorrelationID                     id;
    PDU                               request;
    Timestamp                         sent_at;
    std::chrono::milliseconds         timeout;
    std::function<void(Result<PDU>)>  callback;
};

/// Thread-safe container for in-flight pipeline requests.
class RequestQueue {
public:
    /// Insert a pending request. Returns false if at capacity.
    bool insert(PendingRequest req) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (max_size_ > 0 && pending_.size() >= max_size_) {
            return false;
        }
        auto id = req.id;
        pending_.emplace(id, std::move(req));
        return true;
    }

    /// Remove and return a pending request by correlation ID.
    std::optional<PendingRequest> remove(CorrelationID id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pending_.find(id);
        if (it == pending_.end()) return std::nullopt;
        auto req = std::move(it->second);
        pending_.erase(it);
        return req;
    }

    /// Find expired requests and return them (removing from the queue).
    std::vector<PendingRequest> remove_expired() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = Timestamp::now();
        std::vector<PendingRequest> expired;

        for (auto it = pending_.begin(); it != pending_.end(); ) {
            auto elapsed_us = now.epoch_microseconds() - it->second.sent_at.epoch_microseconds();
            auto timeout_us = static_cast<std::int64_t>(it->second.timeout.count()) * 1000;
            if (elapsed_us > timeout_us) {
                expired.push_back(std::move(it->second));
                it = pending_.erase(it);
            } else {
                ++it;
            }
        }
        return expired;
    }

    /// Cancel a pending request by ID.
    bool cancel(CorrelationID id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.erase(id) > 0;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.size();
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.empty();
    }

    void set_max_size(std::size_t max) { max_size_ = max; }

private:
    mutable std::mutex mutex_;
    std::unordered_map<CorrelationID, PendingRequest> pending_;
    std::size_t max_size_{0}; // 0 = unlimited
};

} // namespace modbus_pp
