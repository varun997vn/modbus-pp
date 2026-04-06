#pragma once

/// @file subscriber.hpp
/// @brief Client-side event subscription and notification dispatch.

#include "subscription.hpp"
#include "../core/result.hpp"
#include "../transport/transport.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace modbus_pp {

/// Client-side subscriber that receives event notifications from a Publisher.
class Subscriber {
public:
    explicit Subscriber(std::shared_ptr<Transport> transport = nullptr);

    /// Register a handler for a subscription ID.
    void on_event(SubscriptionID id,
                   std::function<void(const EventNotification&)> handler);

    /// Remove a handler.
    void remove_handler(SubscriptionID id);

    /// Process incoming event notifications from the transport.
    /// @return Number of events processed.
    std::size_t poll();

    [[nodiscard]] std::size_t handler_count() const;

private:
    std::shared_ptr<Transport> transport_;
    std::unordered_map<SubscriptionID,
                       std::function<void(const EventNotification&)>> handlers_;
    mutable std::mutex mutex_;
};

} // namespace modbus_pp
