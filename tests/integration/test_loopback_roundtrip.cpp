#include <gtest/gtest.h>

#include <modbus_pp/core/pdu.hpp>
#include <modbus_pp/transport/frame_codec.hpp>
#include <modbus_pp/transport/loopback_transport.hpp>

#include <future>
#include <thread>

using namespace modbus_pp;

class LoopbackFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto [c, s] = LoopbackTransport::create_pair();
        client = std::move(c);
        server = std::move(s);
        client->connect();
        server->connect();
    }

    void TearDown() override {
        client->disconnect();
        server->disconnect();
    }

    std::unique_ptr<LoopbackTransport> client;
    std::unique_ptr<LoopbackTransport> server;
};

TEST_F(LoopbackFixture, BasicSendReceive) {
    std::vector<byte_t> data = {0x01, 0x02, 0x03, 0x04};
    auto send_result = client->send(span_t<const byte_t>{data});
    ASSERT_TRUE(send_result.has_value());

    auto recv_result = server->receive(std::chrono::milliseconds{100});
    ASSERT_TRUE(recv_result.has_value());
    EXPECT_EQ(recv_result.value(), data);
}

TEST_F(LoopbackFixture, BidirectionalCommunication) {
    // Client → Server
    std::vector<byte_t> request = {0x01, 0x03, 0x00, 0x10, 0x00, 0x04};
    client->send(span_t<const byte_t>{request});
    auto srv_recv = server->receive(std::chrono::milliseconds{100});
    ASSERT_TRUE(srv_recv.has_value());
    EXPECT_EQ(srv_recv.value(), request);

    // Server → Client
    std::vector<byte_t> response = {0x01, 0x03, 0x08, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04};
    server->send(span_t<const byte_t>{response});
    auto cli_recv = client->receive(std::chrono::milliseconds{100});
    ASSERT_TRUE(cli_recv.has_value());
    EXPECT_EQ(cli_recv.value(), response);
}

TEST_F(LoopbackFixture, TimeoutOnEmptyQueue) {
    auto result = client->receive(std::chrono::milliseconds{10});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::TimeoutExpired));
}

TEST_F(LoopbackFixture, DisconnectedSendFails) {
    client->disconnect();
    std::vector<byte_t> data = {0x01};
    auto result = client->send(span_t<const byte_t>{data});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::TransportDisconnected));
}

TEST_F(LoopbackFixture, DisconnectedReceiveFails) {
    server->disconnect();
    auto result = server->receive(std::chrono::milliseconds{10});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::TransportDisconnected));
}

TEST_F(LoopbackFixture, ErrorInjection) {
    client->set_next_error(make_error_code(ErrorCode::PayloadTooLarge));

    std::vector<byte_t> data = {0x01};
    auto result = client->send(span_t<const byte_t>{data});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), make_error_code(ErrorCode::PayloadTooLarge));

    // Next send should succeed
    auto ok = client->send(span_t<const byte_t>{data});
    EXPECT_TRUE(ok.has_value());
}

TEST_F(LoopbackFixture, PDURoundtripOverLoopback) {
    // Build a standard Modbus PDU, frame it for RTU, send over loopback
    auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                   {0x00, 0x10, 0x00, 0x04});
    auto frame = FrameCodec::wrap_rtu(1, pdu);

    client->send(span_t<const byte_t>{frame});

    auto recv = server->receive(std::chrono::milliseconds{100});
    ASSERT_TRUE(recv.has_value());

    auto decoded = FrameCodec::unwrap_rtu(span_t<const byte_t>{recv.value()});
    ASSERT_TRUE(decoded.has_value());

    auto& [unit_id, decoded_pdu] = decoded.value();
    EXPECT_EQ(unit_id, 1);
    EXPECT_EQ(decoded_pdu.function_code(), FunctionCode::ReadHoldingRegisters);
}

TEST_F(LoopbackFixture, ConcurrentSendReceive) {
    constexpr int num_messages = 100;

    auto sender = std::async(std::launch::async, [&] {
        for (int i = 0; i < num_messages; ++i) {
            std::vector<byte_t> msg = {static_cast<byte_t>(i)};
            auto r = client->send(span_t<const byte_t>{msg});
            EXPECT_TRUE(r.has_value());
        }
    });

    int received = 0;
    for (int i = 0; i < num_messages; ++i) {
        auto result = server->receive(std::chrono::milliseconds{1000});
        if (result.has_value()) {
            ++received;
        }
    }

    sender.get();
    EXPECT_EQ(received, num_messages);
}

TEST_F(LoopbackFixture, MultipleMessages) {
    for (int i = 0; i < 5; ++i) {
        std::vector<byte_t> data = {static_cast<byte_t>(i), static_cast<byte_t>(i + 1)};
        client->send(span_t<const byte_t>{data});
    }

    for (int i = 0; i < 5; ++i) {
        auto result = server->receive(std::chrono::milliseconds{100});
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value()[0], static_cast<byte_t>(i));
    }
}

TEST_F(LoopbackFixture, TransportName) {
    EXPECT_EQ(client->transport_name(), "loopback");
    EXPECT_EQ(server->transport_name(), "loopback");
}
