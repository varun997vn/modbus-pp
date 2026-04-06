#include <gtest/gtest.h>

#include <modbus_pp/core/types.hpp>

#include <unordered_set>
#include <vector>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// StrongType basics
// ---------------------------------------------------------------------------

TEST(StrongType, Construction) {
    RegisterAddress addr{0x1000};
    EXPECT_EQ(addr.raw(), 0x1000);

    RegisterCount count{10};
    EXPECT_EQ(count.raw(), 10);

    UnitId uid{1};
    EXPECT_EQ(uid.raw(), 1);
}

TEST(StrongType, DefaultConstruction) {
    RegisterAddress addr;
    EXPECT_EQ(addr.raw(), 0);
}

TEST(StrongType, Comparison) {
    RegisterAddress a{100}, b{200}, c{100};

    EXPECT_EQ(a, c);
    EXPECT_NE(a, b);
    EXPECT_LT(a, b);
    EXPECT_LE(a, c);
    EXPECT_GT(b, a);
    EXPECT_GE(b, a);
}

TEST(StrongType, Increment) {
    RegisterAddress addr{5};
    ++addr;
    EXPECT_EQ(addr.raw(), 6);

    auto old = addr++;
    EXPECT_EQ(old.raw(), 6);
    EXPECT_EQ(addr.raw(), 7);
}

TEST(StrongType, Hash) {
    std::unordered_set<RegisterAddress> addrs;
    addrs.insert(RegisterAddress{100});
    addrs.insert(RegisterAddress{200});
    addrs.insert(RegisterAddress{100}); // duplicate

    EXPECT_EQ(addrs.size(), 2u);
    EXPECT_NE(addrs.find(RegisterAddress{100}), addrs.end());
    EXPECT_NE(addrs.find(RegisterAddress{200}), addrs.end());
    EXPECT_EQ(addrs.find(RegisterAddress{300}), addrs.end());
}

// ---------------------------------------------------------------------------
// span_t
// ---------------------------------------------------------------------------

TEST(SpanT, FromRawPointer) {
    byte_t data[] = {1, 2, 3, 4, 5};
    span_t<const byte_t> s{data, 5};

    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s[0], 1);
    EXPECT_EQ(s[4], 5);
    EXPECT_FALSE(s.empty());
}

TEST(SpanT, FromArray) {
    std::array<byte_t, 4> arr = {10, 20, 30, 40};
    span_t<const byte_t> s{arr};

    EXPECT_EQ(s.size(), 4u);
    EXPECT_EQ(s[2], 30);
}

TEST(SpanT, FromVector) {
    std::vector<byte_t> vec = {5, 6, 7};
    span_t<const byte_t> s{vec};

    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s[1], 6);
}

TEST(SpanT, EmptySpan) {
    span_t<byte_t> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.begin(), s.end());
}

TEST(SpanT, Subspan) {
    std::array<byte_t, 6> arr = {1, 2, 3, 4, 5, 6};
    span_t<const byte_t> s{arr};

    auto sub = s.subspan(2, 3);
    EXPECT_EQ(sub.size(), 3u);
    EXPECT_EQ(sub[0], 3);
    EXPECT_EQ(sub[2], 5);
}

TEST(SpanT, FirstAndLast) {
    std::array<byte_t, 5> arr = {10, 20, 30, 40, 50};
    span_t<const byte_t> s{arr};

    auto first3 = s.first(3);
    EXPECT_EQ(first3.size(), 3u);
    EXPECT_EQ(first3[2], 30);

    auto last2 = s.last(2);
    EXPECT_EQ(last2.size(), 2u);
    EXPECT_EQ(last2[0], 40);
    EXPECT_EQ(last2[1], 50);
}

TEST(SpanT, Iteration) {
    std::vector<int> data = {1, 2, 3, 4};
    span_t<const int> s{data};

    int sum = 0;
    for (auto val : s) {
        sum += val;
    }
    EXPECT_EQ(sum, 10);
}

TEST(SpanT, Mutation) {
    std::vector<byte_t> data = {1, 2, 3};
    span_t<byte_t> s{data};

    s[1] = 42;
    EXPECT_EQ(data[1], 42);
}
