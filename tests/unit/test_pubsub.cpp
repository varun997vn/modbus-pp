#include <gtest/gtest.h>

#include <modbus_pp/pubsub/publisher.hpp>
#include <modbus_pp/pubsub/subscriber.hpp>
#include <modbus_pp/transport/loopback_transport.hpp>

using namespace modbus_pp;
using reg_t = modbus_pp::register_t;

// ---------------------------------------------------------------------------
// Publisher — local mode (no transport)
// ---------------------------------------------------------------------------

TEST(Publisher, AcceptSubscription) {
    Publisher pub;
    pub.register_data_source(0x0000, 4, []() -> std::vector<reg_t> {
        return {100, 200, 300, 400};
    });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 4;
    req.trigger = Trigger::OnChange;

    auto result = pub.accept_subscription(1, req);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0u);
    EXPECT_EQ(pub.subscription_count(), 1u);
}

TEST(Publisher, RemoveSubscription) {
    Publisher pub;
    pub.register_data_source(0x0000, 2, []() { return std::vector<reg_t>{1, 2}; });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 2;
    auto id = pub.accept_subscription(1, req);
    ASSERT_TRUE(id.has_value());

    auto r = pub.remove_subscription(id.value());
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(pub.subscription_count(), 0u);
}

TEST(Publisher, OnChangeFirstScanAlwaysNotifies) {
    Publisher pub;
    pub.register_data_source(0x0000, 2, []() { return std::vector<reg_t>{100, 200}; });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 2;
    req.trigger = Trigger::OnChange;
    pub.accept_subscription(1, req);

    // First scan should notify (no baseline yet)
    auto sent = pub.scan_and_notify();
    EXPECT_EQ(sent, 1u);
}

TEST(Publisher, OnChangeNoNotifyWhenStable) {
    Publisher pub;
    std::vector<reg_t> values = {100, 200};
    pub.register_data_source(0x0000, 2, [&]() { return values; });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 2;
    req.trigger = Trigger::OnChange;
    pub.accept_subscription(1, req);

    pub.scan_and_notify(); // baseline scan

    // No change — should not notify
    EXPECT_EQ(pub.scan_and_notify(), 0u);
}

TEST(Publisher, OnChangeNotifiesOnValueChange) {
    int call_count = 0;
    std::vector<reg_t> values = {100, 200};

    Publisher pub;
    pub.register_data_source(0x0000, 2, [&]() {
        ++call_count;
        return values;
    });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 2;
    req.trigger = Trigger::OnChange;
    pub.accept_subscription(1, req);

    pub.scan_and_notify(); // baseline

    values[0] = 150; // change
    EXPECT_EQ(pub.scan_and_notify(), 1u);
}

TEST(Publisher, DeadBandFiltering) {
    std::vector<reg_t> values = {100};

    Publisher pub;
    pub.register_data_source(0x0000, 1, [&]() { return values; });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 1;
    req.trigger = Trigger::OnChange;
    req.dead_band = 10.0; // ignore changes < 10
    pub.accept_subscription(1, req);

    pub.scan_and_notify(); // baseline

    values[0] = 105; // change of 5 — within dead-band
    EXPECT_EQ(pub.scan_and_notify(), 0u);

    values[0] = 120; // change of 20 from baseline (100→120) — exceeds dead-band
    EXPECT_EQ(pub.scan_and_notify(), 1u);
}

TEST(Publisher, ThresholdTrigger) {
    std::vector<reg_t> values = {500};

    Publisher pub;
    pub.register_data_source(0x0000, 1, [&]() { return values; });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 1;
    req.trigger = Trigger::OnThreshold;
    req.threshold_high = 1000.0;
    req.threshold_low = 100.0;
    pub.accept_subscription(1, req);

    pub.scan_and_notify(); // baseline

    // Value below high, above low — no notification
    values[0] = 600;
    EXPECT_EQ(pub.scan_and_notify(), 0u);

    // Value above high — notification
    values[0] = 1001;
    EXPECT_EQ(pub.scan_and_notify(), 1u);
}

// ---------------------------------------------------------------------------
// Pub/Sub over loopback
// ---------------------------------------------------------------------------

TEST(PubSub, LoopbackRoundtrip) {
    auto [client_t, server_t] = LoopbackTransport::create_pair();
    client_t->connect();
    server_t->connect();

    auto server_ptr = std::shared_ptr<Transport>(server_t.get(), [](Transport*){});
    auto client_ptr = std::shared_ptr<Transport>(client_t.get(), [](Transport*){});

    Publisher pub(server_ptr);
    Subscriber sub(client_ptr);

    std::vector<reg_t> values = {100};
    pub.register_data_source(0x0000, 1, [&]() { return values; });

    SubscriptionRequest req;
    req.start_address = 0x0000;
    req.count = 1;
    req.trigger = Trigger::OnChange;
    auto sub_id = pub.accept_subscription(1, req);
    ASSERT_TRUE(sub_id.has_value());

    bool event_received = false;
    reg_t received_value = 0;

    sub.on_event(sub_id.value(), [&](const EventNotification& e) {
        event_received = true;
        if (!e.current_values.empty()) {
            received_value = e.current_values[0];
        }
    });

    // First scan sends initial notification
    pub.scan_and_notify();

    // Subscriber polls to receive it
    sub.poll();
    EXPECT_TRUE(event_received);
    EXPECT_EQ(received_value, 100);

    // Change value and scan again
    event_received = false;
    values[0] = 200;
    pub.scan_and_notify();
    sub.poll();
    EXPECT_TRUE(event_received);
    EXPECT_EQ(received_value, 200);

    client_t->disconnect();
    server_t->disconnect();
}

// ---------------------------------------------------------------------------
// Subscriber
// ---------------------------------------------------------------------------

TEST(Subscriber, HandlerCount) {
    Subscriber sub;
    EXPECT_EQ(sub.handler_count(), 0u);

    sub.on_event(1, [](const EventNotification&) {});
    sub.on_event(2, [](const EventNotification&) {});
    EXPECT_EQ(sub.handler_count(), 2u);

    sub.remove_handler(1);
    EXPECT_EQ(sub.handler_count(), 1u);
}
