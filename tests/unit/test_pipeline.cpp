#include <gtest/gtest.h>

#include <modbus_pp/pipeline/pipeline.hpp>
#include <modbus_pp/transport/loopback_transport.hpp>

#include <future>
#include <thread>

using namespace modbus_pp;

class PipelineFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto [c, s] = LoopbackTransport::create_pair();
        client_transport = std::move(c);
        server_transport = std::move(s);
        client_transport->connect();
        server_transport->connect();
    }

    void TearDown() override {
        client_transport->disconnect();
        server_transport->disconnect();
    }

    std::unique_ptr<LoopbackTransport> client_transport;
    std::unique_ptr<LoopbackTransport> server_transport;
};

TEST_F(PipelineFixture, SubmitAndReceive) {
    auto pipeline = Pipeline(
        std::shared_ptr<Transport>(client_transport.get(), [](Transport*){}),
        PipelineConfig{16, std::chrono::milliseconds{1000}});

    // Submit a request
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
    auto request = PDU::make_extended(hdr, {0x01, 0x02});

    bool callback_called = false;
    auto res = pipeline.submit(request,
        [&](Result<PDU> r) {
            callback_called = true;
            EXPECT_TRUE(r.has_value());
        });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(pipeline.in_flight_count(), 1u);

    // Server receives and sends response
    auto recv = server_transport->receive(std::chrono::milliseconds{100});
    ASSERT_TRUE(recv.has_value());

    auto recv_pdu = PDU::deserialize(span_t<const byte_t>{recv.value()});
    ASSERT_TRUE(recv_pdu.has_value());

    // Echo back with same correlation ID
    FrameHeader resp_hdr;
    resp_hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
    resp_hdr.correlation_id = recv_pdu.value().extended_header()->correlation_id;
    auto response = PDU::make_extended(resp_hdr, {0xAA, 0xBB});
    auto resp_bytes = response.serialize();
    server_transport->send(span_t<const byte_t>{resp_bytes});

    // Poll to process the response
    pipeline.poll();
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(pipeline.in_flight_count(), 0u);
    EXPECT_EQ(pipeline.completed_count(), 1u);
}

TEST_F(PipelineFixture, CorrelationIDUniqueness) {
    auto pipeline = Pipeline(
        std::shared_ptr<Transport>(client_transport.get(), [](Transport*){}),
        PipelineConfig{16, std::chrono::milliseconds{5000}});

    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;

    std::vector<CorrelationID> ids;
    for (int i = 0; i < 10; ++i) {
        auto res = pipeline.submit(PDU::make_extended(hdr, {static_cast<byte_t>(i)}),
                                    [](Result<PDU>) {});
        ASSERT_TRUE(res.has_value());
        ids.push_back(res.value());
    }

    // All IDs should be unique
    std::sort(ids.begin(), ids.end());
    auto it = std::unique(ids.begin(), ids.end());
    EXPECT_EQ(it, ids.end());
    EXPECT_EQ(pipeline.in_flight_count(), 10u);
}

TEST_F(PipelineFixture, PipelineOverflow) {
    auto pipeline = Pipeline(
        std::shared_ptr<Transport>(client_transport.get(), [](Transport*){}),
        PipelineConfig{2, std::chrono::milliseconds{5000}});

    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;

    auto r1 = pipeline.submit(PDU::make_extended(hdr, {}), [](Result<PDU>) {});
    auto r2 = pipeline.submit(PDU::make_extended(hdr, {}), [](Result<PDU>) {});
    EXPECT_TRUE(r1.has_value());
    EXPECT_TRUE(r2.has_value());

    // Third should fail — capacity is 2
    auto r3 = pipeline.submit(PDU::make_extended(hdr, {}), [](Result<PDU>) {});
    EXPECT_FALSE(r3.has_value());
    EXPECT_EQ(r3.error(), make_error_code(ErrorCode::PipelineOverflow));
}

TEST_F(PipelineFixture, CancelRequest) {
    auto pipeline = Pipeline(
        std::shared_ptr<Transport>(client_transport.get(), [](Transport*){}),
        PipelineConfig{16, std::chrono::milliseconds{5000}});

    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;

    auto res = pipeline.submit(PDU::make_extended(hdr, {}), [](Result<PDU>) {});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(pipeline.in_flight_count(), 1u);

    EXPECT_TRUE(pipeline.cancel(res.value()));
    EXPECT_EQ(pipeline.in_flight_count(), 0u);

    // Cancel again should return false
    EXPECT_FALSE(pipeline.cancel(res.value()));
}

TEST_F(PipelineFixture, AverageLatency) {
    auto pipeline = Pipeline(
        std::shared_ptr<Transport>(client_transport.get(), [](Transport*){}),
        PipelineConfig{16, std::chrono::milliseconds{1000}});

    // Initial latency should be 0
    EXPECT_EQ(pipeline.average_latency().count(), 0);
}
