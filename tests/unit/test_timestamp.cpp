#include <gtest/gtest.h>

#include <modbus_pp/core/timestamp.hpp>

#include <thread>

using namespace modbus_pp;

TEST(Timestamp, DefaultIsEpoch) {
    Timestamp ts;
    EXPECT_EQ(ts.epoch_microseconds(), 0);
}

TEST(Timestamp, Now) {
    auto ts = Timestamp::now();
    // Should be a reasonable value (after 2020-01-01 in microseconds)
    EXPECT_GT(ts.epoch_microseconds(), 1'577'836'800'000'000LL);
}

TEST(Timestamp, FromEpochUs) {
    auto ts = Timestamp::from_epoch_us(1'000'000);
    EXPECT_EQ(ts.epoch_microseconds(), 1'000'000);
}

TEST(Timestamp, SerializeRoundtrip) {
    auto original = Timestamp::from_epoch_us(1'700'000'000'000'000LL);
    auto bytes = original.serialize();
    auto deserialized = Timestamp::deserialize(bytes.data());

    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(deserialized.epoch_microseconds(), 1'700'000'000'000'000LL);
}

TEST(Timestamp, SerializeBigEndian) {
    // Value 0x0001020304050607 should serialize as {00, 01, 02, 03, 04, 05, 06, 07}
    auto ts = Timestamp::from_epoch_us(0x0001020304050607LL);
    auto bytes = ts.serialize();

    EXPECT_EQ(bytes[0], 0x00);
    EXPECT_EQ(bytes[1], 0x01);
    EXPECT_EQ(bytes[2], 0x02);
    EXPECT_EQ(bytes[3], 0x03);
    EXPECT_EQ(bytes[4], 0x04);
    EXPECT_EQ(bytes[5], 0x05);
    EXPECT_EQ(bytes[6], 0x06);
    EXPECT_EQ(bytes[7], 0x07);
}

TEST(Timestamp, Comparison) {
    auto a = Timestamp::from_epoch_us(100);
    auto b = Timestamp::from_epoch_us(200);
    auto c = Timestamp::from_epoch_us(100);

    EXPECT_EQ(a, c);
    EXPECT_NE(a, b);
    EXPECT_LT(a, b);
    EXPECT_LE(a, c);
    EXPECT_GT(b, a);
    EXPECT_GE(b, a);
}

TEST(Timestamp, NowIsMonotonic) {
    auto t1 = Timestamp::now();
    auto t2 = Timestamp::now();
    EXPECT_LE(t1, t2);
}

TEST(Timestamp, ToTimePoint) {
    auto ts = Timestamp::from_epoch_us(1'000'000);
    auto tp = ts.to_time_point();
    auto duration = tp.time_since_epoch();
    EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(duration).count(), 1'000'000);
}

// ---------------------------------------------------------------------------
// Stamped<T>
// ---------------------------------------------------------------------------

TEST(Stamped, Construction) {
    auto s = Stamped<float>{3.14f, Timestamp::from_epoch_us(1000)};
    EXPECT_FLOAT_EQ(s.value, 3.14f);
    EXPECT_EQ(s.timestamp.epoch_microseconds(), 1000);
}

TEST(Stamped, Now) {
    auto s = Stamped<int>::now(42);
    EXPECT_EQ(s.value, 42);
    EXPECT_GT(s.timestamp.epoch_microseconds(), 0);
}
