/// @file 05_pubsub_events.cpp
/// @brief Pub/Sub event notifications with dead-band filtering.
///
/// Demonstrates how a Server's Publisher monitors a temperature data source
/// and pushes change notifications to a subscribed Client. The dead-band
/// filter suppresses notifications for small changes, reducing bus traffic
/// from noisy sensors.
///
/// Modbus disadvantage addressed: standard Modbus is purely poll-based.
/// The client must repeatedly ask "has anything changed?" even when data
/// is static. This wastes bandwidth and adds latency. Pub/Sub pushes
/// data only when meaningful changes occur.

#include "modbus_pp/modbus_pp.hpp"

#include <atomic>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

int main() {
    std::cout << "=== modbus_pp Example 05: Pub/Sub Events ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Set up loopback transport
    // -----------------------------------------------------------------------
    auto [client_transport, server_transport] = LoopbackTransport::create_pair();
    client_transport->connect();
    server_transport->connect();

    std::shared_ptr<Transport> srv_tp = std::move(server_transport);
    std::shared_ptr<Transport> cli_tp = std::move(client_transport);

    // -----------------------------------------------------------------------
    // 2. Server side: register a temperature data source
    // -----------------------------------------------------------------------
    // Simulated temperature that we control from the main thread
    std::atomic<int> temp_raw{2550};  // stored as int, represents 25.50 C * 100
    std::mutex data_mutex;

    ServerConfig srv_cfg{srv_tp, 1, true};
    Server server(std::move(srv_cfg));

    // Register handler for standard reads (so client can also poll)
    server.on_read_holding_registers(
        [&temp_raw](address_t addr, quantity_t count) -> Result<std::vector<reg_t>> {
            if (addr == 0 && count == 2) {
                float temp = static_cast<float>(temp_raw.load()) / 100.0f;
                auto regs = TypeCodec::encode_float<ByteOrder::ABCD>(temp);
                return std::vector<reg_t>(regs.begin(), regs.end());
            }
            return ErrorCode::IllegalDataAddress;
        });

    // Register a data source that the Publisher monitors
    auto& publisher = server.publisher();
    publisher.register_data_source(0, 2,
        [&temp_raw]() -> std::vector<reg_t> {
            float temp = static_cast<float>(temp_raw.load()) / 100.0f;
            auto regs = TypeCodec::encode_float<ByteOrder::ABCD>(temp);
            return std::vector<reg_t>(regs.begin(), regs.end());
        });

    // Accept a subscription with dead-band filtering
    SubscriptionRequest sub_request;
    sub_request.start_address = 0;
    sub_request.count = 2;
    sub_request.type = RegisterType::Float32;
    sub_request.byte_order = ByteOrder::ABCD;
    sub_request.trigger = Trigger::OnChange;
    sub_request.dead_band = 5.0;  // Only notify when change >= 5.0 degrees

    auto sub_result = publisher.accept_subscription(1, sub_request);
    SubscriptionID sub_id = 0;
    if (sub_result) {
        sub_id = sub_result.value();
        std::cout << "[server] Subscription accepted, ID = " << sub_id << "\n";
        std::cout << "[server] Trigger: OnChange, dead-band = 5.0 degrees\n\n";
    } else {
        std::cerr << "[server] Subscription failed: "
                  << sub_result.error().message() << "\n";
        return 1;
    }

    // -----------------------------------------------------------------------
    // 3. Client side: register event handler
    // -----------------------------------------------------------------------
    ClientConfig cli_cfg{cli_tp, nullptr, PipelineConfig{}, true,
                         std::chrono::milliseconds{1000}};
    Client client(std::move(cli_cfg));

    int notification_count = 0;
    auto& subscriber = client.subscriber();
    subscriber.on_event(sub_id,
        [&notification_count](const EventNotification& event) {
            ++notification_count;

            // Decode current temperature from the notification
            float current = TypeCodec::decode_float<ByteOrder::ABCD>(
                event.current_values.data());

            float previous = 0.0f;
            if (!event.previous_values.empty()) {
                previous = TypeCodec::decode_float<ByteOrder::ABCD>(
                    event.previous_values.data());
            }

            float delta = current - previous;
            std::cout << "  [event #" << notification_count << "] "
                      << "Temperature changed: " << std::fixed << std::setprecision(1)
                      << previous << " -> " << current
                      << " (delta = " << std::showpos << delta << std::noshowpos << ")\n";
        });

    // -----------------------------------------------------------------------
    // 4. Simulate temperature changes and observe notifications
    // -----------------------------------------------------------------------
    std::cout << "--- Simulating Temperature Changes ---\n";
    std::cout << "  Dead-band = 5.0: only changes >= 5.0 degrees trigger notifications.\n\n";

    // Temperature steps: start at 25.5, apply various deltas
    struct TempStep {
        float new_temp;
        const char* description;
        bool expect_notification;
    };

    TempStep steps[] = {
        {25.5f,  "Initial value (baseline)",               true},   // baseline scan
        {27.0f,  "Small change: +1.5 (within dead-band)",  false},
        {28.0f,  "Small change: +1.0 (within dead-band)",  false},
        {33.0f,  "Large change: +5.0 (meets dead-band!)",  true},
        {34.0f,  "Small change: +1.0 (within dead-band)",  false},
        {25.0f,  "Large drop:  -9.0 (exceeds dead-band!)", true},
        {24.0f,  "Small change: -1.0 (within dead-band)",  false},
        {30.5f,  "Large rise:  +6.5 (exceeds dead-band!)", true},
        {31.0f,  "Small change: +0.5 (within dead-band)",  false},
        {38.0f,  "Large rise:  +7.0 (exceeds dead-band!)", true},
    };

    for (const auto& step : steps) {
        // Update the temperature
        temp_raw.store(static_cast<int>(step.new_temp * 100.0f));

        int before = notification_count;

        std::cout << "  Set temp = " << std::fixed << std::setprecision(1)
                  << step.new_temp << " -- " << step.description << "\n";

        // Server scans data sources and sends notifications if thresholds met
        publisher.scan_and_notify();

        // Client processes incoming events
        subscriber.poll();

        bool fired = (notification_count > before);
        std::cout << "    -> notification " << (fired ? "FIRED" : "suppressed")
                  << (fired == step.expect_notification ? " (expected)" : " (UNEXPECTED)")
                  << "\n";
    }

    // -----------------------------------------------------------------------
    // 5. Summary
    // -----------------------------------------------------------------------
    std::cout << "\n--- Summary ---\n";
    std::cout << "  Total temperature changes: " << (sizeof(steps) / sizeof(steps[0])) << "\n";
    std::cout << "  Notifications sent:        " << notification_count << "\n";
    std::cout << "  Notifications suppressed:  "
              << ((sizeof(steps) / sizeof(steps[0])) - notification_count) << "\n";
    std::cout << "\n  Dead-band filtering reduced bus traffic by "
              << (100 - 100 * notification_count / static_cast<int>(sizeof(steps) / sizeof(steps[0])))
              << "%.\n";
    std::cout << "  In standard Modbus, every poll sees the current value regardless\n"
              << "  of whether it changed meaningfully -- wasting bandwidth.\n";

    std::cout << "\n[done] Pub/Sub events example complete.\n";
    return 0;
}
