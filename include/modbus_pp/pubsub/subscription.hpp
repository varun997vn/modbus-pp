#pragma once

/// @file subscription.hpp
/// @brief Pub/Sub subscription types and event notifications.
///
/// Replaces Modbus polling with event-driven push notifications.
/// Supports three trigger modes: on-change (with dead-band filtering),
/// threshold crossing, and periodic.

#include "../core/timestamp.hpp"
#include "../core/types.hpp"
#include "../core/endian.hpp"
#include "../register_map/register_descriptor.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace modbus_pp {

using SubscriptionID = std::uint32_t;

/// Defines when the server should push data to the subscriber.
enum class Trigger : std::uint8_t {
    OnChange,     ///< Push when any value changes (respecting dead-band).
    OnThreshold,  ///< Push when value crosses a high or low threshold.
    Periodic,     ///< Push at fixed intervals regardless of change.
};

/// Describes a subscription request from client to server.
struct SubscriptionRequest {
    address_t   start_address{0};
    quantity_t  count{1};
    RegisterType type{RegisterType::UInt16};
    ByteOrder   byte_order{ByteOrder::ABCD};

    Trigger trigger{Trigger::OnChange};

    /// For OnThreshold: push when value rises above this.
    std::optional<double> threshold_high;

    /// For OnThreshold: push when value falls below this.
    std::optional<double> threshold_low;

    /// For Periodic: notification interval.
    std::optional<std::chrono::milliseconds> interval;

    /// Suppress notifications for changes smaller than this absolute value.
    /// Prevents noisy sensor data from flooding the bus.
    double dead_band{0.0};
};

/// An event notification pushed from server to subscriber.
struct EventNotification {
    SubscriptionID           subscription_id{0};
    address_t                start_address{0};
    Timestamp                timestamp;
    std::vector<register_t>  current_values;
    std::vector<register_t>  previous_values;
};

} // namespace modbus_pp
