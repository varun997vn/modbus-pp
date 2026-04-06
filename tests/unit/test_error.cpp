#include <gtest/gtest.h>

#include <modbus_pp/core/error.hpp>
#include <modbus_pp/core/result.hpp>

#include <string>
#include <system_error>

using namespace modbus_pp;

// ---------------------------------------------------------------------------
// ErrorCode and error_category
// ---------------------------------------------------------------------------

TEST(Error, CategoryName) {
    EXPECT_STREQ(modbus_category().name(), "modbus_pp");
}

TEST(Error, SuccessMessage) {
    auto ec = make_error_code(ErrorCode::Success);
    EXPECT_EQ(ec.message(), "Success");
}

TEST(Error, StandardExceptionMessages) {
    EXPECT_EQ(make_error_code(ErrorCode::IllegalFunction).message(), "Illegal function");
    EXPECT_EQ(make_error_code(ErrorCode::IllegalDataAddress).message(), "Illegal data address");
    EXPECT_EQ(make_error_code(ErrorCode::SlaveDeviceFailure).message(), "Slave device failure");
}

TEST(Error, ExtendedErrorMessages) {
    EXPECT_EQ(make_error_code(ErrorCode::AuthenticationFailed).message(), "Authentication failed");
    EXPECT_EQ(make_error_code(ErrorCode::PipelineOverflow).message(), "Pipeline capacity exceeded");
    EXPECT_EQ(make_error_code(ErrorCode::HMACMismatch).message(), "HMAC verification failed");
    EXPECT_EQ(make_error_code(ErrorCode::TimeoutExpired).message(), "Timeout expired");
    EXPECT_EQ(make_error_code(ErrorCode::CrcMismatch).message(), "CRC mismatch");
}

TEST(Error, ImplicitConversion) {
    // ErrorCode should implicitly convert to std::error_code
    std::error_code ec = ErrorCode::IllegalFunction;
    EXPECT_EQ(ec.value(), 1);
    EXPECT_EQ(&ec.category(), &modbus_category());
}

TEST(Error, BoolConversion) {
    std::error_code success = make_error_code(ErrorCode::Success);
    std::error_code failure = make_error_code(ErrorCode::TimeoutExpired);

    EXPECT_FALSE(static_cast<bool>(success)); // 0 is falsy
    EXPECT_TRUE(static_cast<bool>(failure));
}

// ---------------------------------------------------------------------------
// Result<T>
// ---------------------------------------------------------------------------

TEST(Result, ValueConstruction) {
    Result<int> r{42};
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(r.value(), 42);
}

TEST(Result, ErrorConstruction) {
    Result<int> r{ErrorCode::TimeoutExpired};
    EXPECT_FALSE(r.has_value());
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error(), make_error_code(ErrorCode::TimeoutExpired));
}

TEST(Result, ErrorCodeConstruction) {
    std::error_code ec = ErrorCode::IllegalFunction;
    Result<std::string> r{ec};
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().value(), 1);
}

TEST(Result, ValueOr) {
    Result<int> good{42};
    Result<int> bad{ErrorCode::TimeoutExpired};

    EXPECT_EQ(good.value_or(0), 42);
    EXPECT_EQ(bad.value_or(0), 0);
}

TEST(Result, Map) {
    Result<int> r{10};
    auto doubled = r.map([](int v) { return v * 2; });

    EXPECT_TRUE(doubled.has_value());
    EXPECT_EQ(doubled.value(), 20);
}

TEST(Result, MapError) {
    Result<int> r{ErrorCode::IllegalFunction};
    auto doubled = r.map([](int v) { return v * 2; });

    EXPECT_FALSE(doubled.has_value());
    EXPECT_EQ(doubled.error(), make_error_code(ErrorCode::IllegalFunction));
}

TEST(Result, AndThen) {
    Result<int> r{10};
    auto chained = r.and_then([](int v) -> Result<std::string> {
        return std::to_string(v);
    });

    EXPECT_TRUE(chained.has_value());
    EXPECT_EQ(chained.value(), "10");
}

TEST(Result, AndThenError) {
    Result<int> r{ErrorCode::TimeoutExpired};
    auto chained = r.and_then([](int v) -> Result<std::string> {
        return std::to_string(v);
    });

    EXPECT_FALSE(chained.has_value());
    EXPECT_EQ(chained.error(), make_error_code(ErrorCode::TimeoutExpired));
}

TEST(Result, AndThenChainedFailure) {
    Result<int> r{10};
    auto chained = r.and_then([](int) -> Result<std::string> {
        return ErrorCode::IllegalDataValue;
    });

    EXPECT_FALSE(chained.has_value());
}

TEST(Result, MoveValue) {
    Result<std::string> r{std::string("hello")};
    std::string moved = std::move(r).value();
    EXPECT_EQ(moved, "hello");
}

// ---------------------------------------------------------------------------
// Result<void>
// ---------------------------------------------------------------------------

TEST(ResultVoid, Success) {
    Result<void> r;
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(ResultVoid, Error) {
    Result<void> r{ErrorCode::TransportDisconnected};
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), make_error_code(ErrorCode::TransportDisconnected));
}
