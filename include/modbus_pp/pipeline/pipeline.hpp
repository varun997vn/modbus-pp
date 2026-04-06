#pragma once

/// @file pipeline.hpp
/// @brief Pipelined transaction manager — multiple requests in flight.
///
/// Standard Modbus is strictly sequential: send request, wait for response,
/// repeat. The Pipeline allows submitting multiple requests concurrently,
/// matching responses by correlation ID, dramatically improving throughput
/// over high-latency transports.

#include "correlation_id.hpp"
#include "request_queue.hpp"
#include "../core/pdu.hpp"
#include "../core/result.hpp"
#include "../transport/transport.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

namespace modbus_pp {

/// Configuration for the Pipeline.
struct PipelineConfig {
    std::uint16_t max_in_flight{16};
    std::chrono::milliseconds default_timeout{1000};
};

/// Pipelined transaction manager.
///
/// Wraps a Transport and enables multiple concurrent requests by tagging
/// each with a correlation ID in the extended frame header.
class Pipeline {
public:
    Pipeline(std::shared_ptr<Transport> transport, PipelineConfig config = {});

    /// Submit a request asynchronously with a callback.
    /// @return The assigned correlation ID, or an error if the pipeline is full.
    Result<CorrelationID> submit(PDU request,
                                  std::function<void(Result<PDU>)> callback,
                                  std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

    /// Submit a request and block until the response arrives.
    Result<PDU> submit_sync(PDU request,
                             std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});

    /// Process incoming responses and dispatch callbacks.
    /// Call this from your event loop.
    void poll();

    /// Cancel a pending request.
    bool cancel(CorrelationID id);

    // Statistics
    [[nodiscard]] std::size_t in_flight_count() const noexcept;
    [[nodiscard]] std::uint64_t completed_count() const noexcept;
    [[nodiscard]] std::chrono::microseconds average_latency() const noexcept;

private:
    std::shared_ptr<Transport> transport_;
    PipelineConfig config_;
    RequestQueue queue_;
    CorrelationIDGenerator id_gen_;

    std::atomic<std::uint64_t> total_completed_{0};
    std::atomic<std::uint64_t> total_latency_us_{0};
};

} // namespace modbus_pp
