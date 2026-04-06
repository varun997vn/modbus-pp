#include "modbus_pp/pubsub/subscriber.hpp"
#include "modbus_pp/core/pdu.hpp"

namespace modbus_pp {

Subscriber::Subscriber(std::shared_ptr<Transport> transport)
    : transport_(std::move(transport)) {}

void Subscriber::on_event(SubscriptionID id,
                            std::function<void(const EventNotification&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[id] = std::move(handler);
}

void Subscriber::remove_handler(SubscriptionID id) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(id);
}

std::size_t Subscriber::poll() {
    if (!transport_ || !transport_->is_connected()) return 0;

    std::size_t processed = 0;
    while (true) {
        auto recv = transport_->receive(std::chrono::milliseconds{0});
        if (!recv) break;

        auto pdu_result = PDU::deserialize(span_t<const byte_t>{recv.value()});
        if (!pdu_result) continue;

        auto& pdu = pdu_result.value();
        if (!pdu.is_extended()) continue;
        if (pdu.extended_header()->ext_function_code !=
            ExtendedFunctionCode::EventNotification) continue;

        // Parse notification payload: [sub_id:4][addr:2][reg_count:2][regs...]
        auto payload = pdu.payload();
        if (payload.size() < 8) continue;

        EventNotification notif;
        notif.subscription_id = static_cast<SubscriptionID>(
            (static_cast<uint32_t>(payload[0]) << 24) |
            (static_cast<uint32_t>(payload[1]) << 16) |
            (static_cast<uint32_t>(payload[2]) << 8) |
            static_cast<uint32_t>(payload[3]));
        notif.start_address = static_cast<address_t>(
            (static_cast<uint16_t>(payload[4]) << 8) | payload[5]);

        auto reg_count = static_cast<uint16_t>(
            (static_cast<uint16_t>(payload[6]) << 8) | payload[7]);

        std::size_t offset = 8;
        for (uint16_t i = 0; i < reg_count && offset + 1 < payload.size(); ++i) {
            auto reg = static_cast<register_t>(
                (static_cast<uint16_t>(payload[offset]) << 8) | payload[offset + 1]);
            notif.current_values.push_back(reg);
            offset += 2;
        }

        if (pdu.extended_header()->timestamp.has_value()) {
            notif.timestamp = *pdu.extended_header()->timestamp;
        } else {
            notif.timestamp = Timestamp::now();
        }

        // Dispatch to handler
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(notif.subscription_id);
        if (it != handlers_.end()) {
            it->second(notif);
        }
        ++processed;
    }

    return processed;
}

std::size_t Subscriber::handler_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handlers_.size();
}

} // namespace modbus_pp
