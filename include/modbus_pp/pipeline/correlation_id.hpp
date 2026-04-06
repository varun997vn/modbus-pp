#pragma once

/// @file correlation_id.hpp
/// @brief Correlation ID generation for pipelined request/response matching.

#include <atomic>
#include <cstdint>

namespace modbus_pp {

using CorrelationID = std::uint16_t;

/// Thread-safe monotonic correlation ID generator.
class CorrelationIDGenerator {
public:
    /// Generate the next unique correlation ID.
    CorrelationID next() noexcept {
        return counter_.fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic<CorrelationID> counter_{1};
};

} // namespace modbus_pp
