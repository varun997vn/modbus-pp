#pragma once

/// @file publisher.hpp
/// @brief Server-side change detection and push notification.
///
/// The Publisher monitors registered data sources, detects changes
/// (respecting dead-band filtering), and pushes EventNotifications
/// to subscribed clients.

#include "subscription.hpp"
#include "../core/result.hpp"
#include "../transport/transport.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace modbus_pp {

/// Server-side publisher that monitors data and pushes change notifications.
class Publisher {
public:
    explicit Publisher(std::shared_ptr<Transport> transport = nullptr);

    /// Register a data source that the publisher monitors.
    void register_data_source(address_t start, quantity_t count,
                               std::function<std::vector<register_t>()> reader);

    /// Accept a subscription request from a client.
    Result<SubscriptionID> accept_subscription(unit_id_t client,
                                                const SubscriptionRequest& request);

    /// Remove a subscription.
    Result<void> remove_subscription(SubscriptionID id);

    /// Scan registered data sources for changes and send notifications.
    /// Call this periodically from the server event loop.
    /// @return Number of notifications sent.
    std::size_t scan_and_notify();

    [[nodiscard]] std::size_t subscription_count() const;

private:
    struct DataSource {
        address_t start;
        quantity_t count;
        std::function<std::vector<register_t>()> reader;
        std::vector<register_t> last_values;
        bool has_baseline{false};
    };

    struct ActiveSubscription {
        SubscriptionID id;
        unit_id_t client;
        SubscriptionRequest request;
        Timestamp last_notified;
    };

    bool should_notify(const ActiveSubscription& sub,
                       const std::vector<register_t>& current,
                       const std::vector<register_t>& previous) const;

    std::shared_ptr<Transport> transport_;
    std::vector<DataSource> sources_;
    std::vector<ActiveSubscription> subscriptions_;
    SubscriptionID next_sub_id_{1};
    mutable std::mutex mutex_;
};

} // namespace modbus_pp
