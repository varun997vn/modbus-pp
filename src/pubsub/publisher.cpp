#include "modbus_pp/pubsub/publisher.hpp"
#include "modbus_pp/core/function_codes.hpp"
#include "modbus_pp/core/pdu.hpp"

#include <algorithm>
#include <cmath>

namespace modbus_pp {

Publisher::Publisher(std::shared_ptr<Transport> transport)
    : transport_(std::move(transport)) {}

void Publisher::register_data_source(address_t start, quantity_t count,
                                      std::function<std::vector<register_t>()> reader) {
    std::lock_guard<std::mutex> lock(mutex_);
    sources_.push_back({start, count, std::move(reader), {}, false});
}

Result<SubscriptionID> Publisher::accept_subscription(
    unit_id_t client, const SubscriptionRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto id = next_sub_id_++;
    subscriptions_.push_back({id, client, request, Timestamp::now()});
    return id;
}

Result<void> Publisher::remove_subscription(SubscriptionID id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                              [id](const ActiveSubscription& s) { return s.id == id; });
    if (it == subscriptions_.end()) {
        return ErrorCode::IllegalDataAddress;
    }
    subscriptions_.erase(it, subscriptions_.end());
    return Result<void>{};
}

bool Publisher::should_notify(const ActiveSubscription& sub,
                               const std::vector<register_t>& current,
                               const std::vector<register_t>& previous) const {
    const auto& req = sub.request;

    switch (req.trigger) {
        case Trigger::Periodic: {
            if (!req.interval.has_value()) return false;
            auto now = Timestamp::now();
            auto elapsed_us = now.epoch_microseconds() - sub.last_notified.epoch_microseconds();
            auto interval_us = static_cast<std::int64_t>(req.interval->count()) * 1000;
            return elapsed_us >= interval_us;
        }

        case Trigger::OnChange: {
            if (previous.empty()) return true; // first reading
            for (std::size_t i = 0; i < current.size() && i < previous.size(); ++i) {
                auto diff = std::abs(static_cast<double>(current[i]) -
                                     static_cast<double>(previous[i]));
                if (diff > req.dead_band) return true;
            }
            return false;
        }

        case Trigger::OnThreshold: {
            if (current.empty()) return false;
            double value = static_cast<double>(current[0]);
            if (req.threshold_high.has_value() && value > *req.threshold_high) return true;
            if (req.threshold_low.has_value() && value < *req.threshold_low) return true;
            return false;
        }
    }
    return false;
}

std::size_t Publisher::scan_and_notify() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t notifications_sent = 0;

    // Read current values from all data sources
    for (auto& source : sources_) {
        auto current = source.reader();

        for (auto& sub : subscriptions_) {
            // Check if this subscription overlaps this data source
            if (sub.request.start_address < source.start ||
                sub.request.start_address >= source.start + source.count) {
                continue;
            }

            // Extract the relevant slice
            auto offset = sub.request.start_address - source.start;
            auto end = std::min(static_cast<std::size_t>(offset + sub.request.count),
                                current.size());
            std::vector<register_t> slice(current.begin() + offset,
                                           current.begin() + static_cast<std::ptrdiff_t>(end));

            std::vector<register_t> prev_slice;
            if (source.has_baseline) {
                auto prev_end = std::min(static_cast<std::size_t>(offset + sub.request.count),
                                          source.last_values.size());
                prev_slice.assign(source.last_values.begin() + offset,
                                   source.last_values.begin() + static_cast<std::ptrdiff_t>(prev_end));
            }

            if (should_notify(sub, slice, prev_slice)) {
                EventNotification notif;
                notif.subscription_id = sub.id;
                notif.start_address = sub.request.start_address;
                notif.timestamp = Timestamp::now();
                notif.current_values = slice;
                notif.previous_values = prev_slice;

                // Send notification if transport is available
                if (transport_ && transport_->is_connected()) {
                    FrameHeader hdr;
                    hdr.ext_function_code = ExtendedFunctionCode::EventNotification;
                    hdr.timestamp = notif.timestamp;

                    // Serialize notification payload: [sub_id:4][addr:2][reg_count:2][regs...]
                    std::vector<byte_t> payload;
                    auto sid = notif.subscription_id;
                    payload.push_back(static_cast<byte_t>((sid >> 24) & 0xFF));
                    payload.push_back(static_cast<byte_t>((sid >> 16) & 0xFF));
                    payload.push_back(static_cast<byte_t>((sid >> 8) & 0xFF));
                    payload.push_back(static_cast<byte_t>(sid & 0xFF));
                    payload.push_back(static_cast<byte_t>(notif.start_address >> 8));
                    payload.push_back(static_cast<byte_t>(notif.start_address & 0xFF));
                    auto rc = static_cast<std::uint16_t>(notif.current_values.size());
                    payload.push_back(static_cast<byte_t>(rc >> 8));
                    payload.push_back(static_cast<byte_t>(rc & 0xFF));
                    for (auto reg : notif.current_values) {
                        payload.push_back(static_cast<byte_t>(reg >> 8));
                        payload.push_back(static_cast<byte_t>(reg & 0xFF));
                    }

                    auto pdu = PDU::make_extended(hdr, std::move(payload));
                    auto bytes = pdu.serialize();
                    transport_->send(span_t<const byte_t>{bytes});
                }

                sub.last_notified = Timestamp::now();
                ++notifications_sent;
            }
        }

        source.last_values = std::move(current);
        source.has_baseline = true;
    }

    return notifications_sent;
}

std::size_t Publisher::subscription_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.size();
}

} // namespace modbus_pp
