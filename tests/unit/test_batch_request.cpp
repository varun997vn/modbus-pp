#include <gtest/gtest.h>

#include <modbus_pp/register_map/batch_request.hpp>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// BatchRequest
// ---------------------------------------------------------------------------

TEST(BatchRequest, AddItems) {
    BatchRequest req;
    req.add(0x0000, 2, RegisterType::Float32)
       .add(0x0010, 1, RegisterType::UInt16)
       .add(0x0020, 4, RegisterType::Float64);

    EXPECT_EQ(req.size(), 3u);
    EXPECT_EQ(req.total_registers(), 7);
}

TEST(BatchRequest, OptimizeMergesContiguous) {
    BatchRequest req;
    req.add(0x0000, 2)
       .add(0x0002, 3)
       .add(0x0005, 1);

    auto optimized = req.optimized();
    // All three should merge into one [0x0000, 6]
    EXPECT_EQ(optimized.size(), 1u);
    EXPECT_EQ(optimized.items()[0].start_address, 0x0000);
    EXPECT_EQ(optimized.items()[0].count, 6);
}

TEST(BatchRequest, OptimizeKeepsGaps) {
    BatchRequest req;
    req.add(0x0000, 2)
       .add(0x0010, 3);

    auto optimized = req.optimized();
    // Not contiguous — should stay as 2 items
    EXPECT_EQ(optimized.size(), 2u);
}

TEST(BatchRequest, OptimizeMergesOverlapping) {
    BatchRequest req;
    req.add(0x0000, 4)
       .add(0x0002, 4); // overlaps 0x0002-0x0003, extends to 0x0005

    auto optimized = req.optimized();
    EXPECT_EQ(optimized.size(), 1u);
    EXPECT_EQ(optimized.items()[0].start_address, 0x0000);
    EXPECT_EQ(optimized.items()[0].count, 6);
}

TEST(BatchRequest, OptimizePreservesDifferentByteOrders) {
    BatchRequest req;
    req.add(0x0000, 2, RegisterType::Float32, ByteOrder::ABCD)
       .add(0x0002, 2, RegisterType::Float32, ByteOrder::BADC);

    auto optimized = req.optimized();
    // Different byte orders — should not merge
    EXPECT_EQ(optimized.size(), 2u);
}

TEST(BatchRequest, OptimizeSingleItem) {
    BatchRequest req;
    req.add(0x0000, 4);
    auto optimized = req.optimized();
    EXPECT_EQ(optimized.size(), 1u);
}

TEST(BatchRequest, OptimizeEmpty) {
    BatchRequest req;
    auto optimized = req.optimized();
    EXPECT_EQ(optimized.size(), 0u);
}

// ---------------------------------------------------------------------------
// Serialization round-trip
// ---------------------------------------------------------------------------

TEST(BatchRequest, SerializeRoundtrip) {
    BatchRequest original;
    original.add(0x0100, 2, RegisterType::Float32, ByteOrder::BADC)
            .add(0x0200, 4, RegisterType::Float64, ByteOrder::ABCD);

    auto bytes = original.serialize();
    auto result = BatchRequest::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());

    auto& decoded = result.value();
    EXPECT_EQ(decoded.size(), 2u);
    EXPECT_EQ(decoded.items()[0].start_address, 0x0100);
    EXPECT_EQ(decoded.items()[0].count, 2);
    EXPECT_EQ(decoded.items()[0].type, RegisterType::Float32);
    EXPECT_EQ(decoded.items()[0].byte_order, ByteOrder::BADC);

    EXPECT_EQ(decoded.items()[1].start_address, 0x0200);
    EXPECT_EQ(decoded.items()[1].count, 4);
}

TEST(BatchRequest, DeserializeTruncated) {
    std::vector<byte_t> bad = {0x00};
    auto result = BatchRequest::deserialize(span_t<const byte_t>{bad});
    EXPECT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// BatchResponse
// ---------------------------------------------------------------------------

TEST(BatchResponse, AllSucceeded) {
    BatchResponse resp;
    resp.add_result(0x0000, {0x1234, 0x5678});
    resp.add_result(0x0002, {0xABCD});

    EXPECT_TRUE(resp.all_succeeded());
    EXPECT_EQ(resp.size(), 2u);
}

TEST(BatchResponse, PartialFailure) {
    BatchResponse resp;
    resp.add_result(0x0000, {0x1234});
    resp.add_result(0x0002, {}, make_error_code(ErrorCode::IllegalDataAddress));

    EXPECT_FALSE(resp.all_succeeded());
}

TEST(BatchResponse, FindByAddress) {
    BatchResponse resp;
    resp.add_result(0x0010, {0xAAAA, 0xBBBB});
    resp.add_result(0x0020, {0xCCCC});

    auto* found = resp.find(0x0010);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->registers.size(), 2u);
    EXPECT_EQ(found->registers[0], 0xAAAA);

    EXPECT_EQ(resp.find(0x9999), nullptr);
}

TEST(BatchResponse, SerializeRoundtrip) {
    BatchResponse original;
    original.add_result(0x0100, {0x1111, 0x2222});
    original.add_result(0x0200, {0x3333}, make_error_code(ErrorCode::IllegalDataAddress));

    auto bytes = original.serialize();
    auto result = BatchResponse::deserialize(span_t<const byte_t>{bytes});
    ASSERT_TRUE(result.has_value());

    auto& decoded = result.value();
    EXPECT_EQ(decoded.size(), 2u);
    EXPECT_EQ(decoded.results()[0].address, 0x0100);
    EXPECT_EQ(decoded.results()[0].registers.size(), 2u);
    EXPECT_FALSE(decoded.results()[0].status);

    EXPECT_EQ(decoded.results()[1].address, 0x0200);
    EXPECT_TRUE(static_cast<bool>(decoded.results()[1].status));
}
