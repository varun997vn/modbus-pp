#include "modbus_pp/pipeline/pipeline.hpp"

#include <condition_variable>

namespace modbus_pp {

Pipeline::Pipeline(std::shared_ptr<Transport> transport, PipelineConfig config)
    : transport_(std::move(transport)), config_(config) {
    queue_.set_max_size(config_.max_in_flight);
}

Result<CorrelationID> Pipeline::submit(
    PDU request,
    std::function<void(Result<PDU>)> callback,
    std::chrono::milliseconds timeout) {

    if (timeout.count() == 0) {
        timeout = config_.default_timeout;
    }

    auto corr_id = id_gen_.next();

    // Wrap the request in an extended frame with the correlation ID
    FrameHeader hdr;
    hdr.correlation_id = corr_id;

    // If the request is already extended, update its correlation ID
    PDU to_send = [&]() -> PDU {
        if (request.is_extended() && request.extended_header().has_value()) {
            auto new_hdr = *request.extended_header();
            new_hdr.correlation_id = corr_id;
            return PDU::make_extended(new_hdr,
                                       std::vector<byte_t>(request.payload().begin(),
                                                            request.payload().end()),
                                       request.hmac());
        }
        // Wrap a standard request in an extended frame for correlation
        hdr.ext_function_code = ExtendedFunctionCode::PipelineBegin;
        auto pdu_bytes = request.serialize();
        return PDU::make_extended(hdr, std::move(pdu_bytes));
    }();

    PendingRequest pending;
    pending.id = corr_id;
    pending.request = std::move(to_send);
    pending.sent_at = Timestamp::now();
    pending.timeout = timeout;
    pending.callback = std::move(callback);

    // Send the frame
    auto send_bytes = pending.request.serialize();
    auto send_result = transport_->send(span_t<const byte_t>{send_bytes});
    if (!send_result) {
        return send_result.error();
    }

    // Track the pending request
    if (!queue_.insert(std::move(pending))) {
        return ErrorCode::PipelineOverflow;
    }

    return corr_id;
}

Result<PDU> Pipeline::submit_sync(PDU request, std::chrono::milliseconds timeout) {
    std::mutex mtx;
    std::condition_variable cv;
    Result<PDU> result{ErrorCode::TimeoutExpired};
    bool done = false;

    auto res = submit(std::move(request),
                       [&](Result<PDU> r) {
                           std::lock_guard<std::mutex> lock(mtx);
                           result = std::move(r);
                           done = true;
                           cv.notify_one();
                       },
                       timeout);

    if (!res) return res.error();

    // Poll until response arrives or timeout
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        poll();
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_for(lock, std::chrono::milliseconds{1},
                            [&] { return done; })) {
                return result;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            cancel(res.value());
            return ErrorCode::TimeoutExpired;
        }
    }
}

void Pipeline::poll() {
    // Try to receive a response
    auto recv_result = transport_->receive(std::chrono::milliseconds{0});
    if (!recv_result) return;

    auto& frame = recv_result.value();
    auto pdu_result = PDU::deserialize(span_t<const byte_t>{frame});
    if (!pdu_result) return;

    auto& pdu = pdu_result.value();

    // Extract correlation ID from extended header
    CorrelationID corr_id = 0;
    if (pdu.is_extended() && pdu.extended_header().has_value()) {
        corr_id = pdu.extended_header()->correlation_id;
    }

    // Match with pending request
    auto pending = queue_.remove(corr_id);
    if (!pending) return;

    // Update statistics
    auto now = Timestamp::now();
    auto latency_us = static_cast<std::uint64_t>(
        now.epoch_microseconds() - pending->sent_at.epoch_microseconds());
    total_completed_.fetch_add(1, std::memory_order_relaxed);
    total_latency_us_.fetch_add(latency_us, std::memory_order_relaxed);

    // Dispatch callback
    if (pending->callback) {
        pending->callback(std::move(pdu));
    }

    // Check for expired requests
    auto expired = queue_.remove_expired();
    for (auto& req : expired) {
        if (req.callback) {
            req.callback(ErrorCode::TimeoutExpired);
        }
    }
}

bool Pipeline::cancel(CorrelationID id) {
    return queue_.cancel(id);
}

std::size_t Pipeline::in_flight_count() const noexcept {
    return queue_.size();
}

std::uint64_t Pipeline::completed_count() const noexcept {
    return total_completed_.load(std::memory_order_relaxed);
}

std::chrono::microseconds Pipeline::average_latency() const noexcept {
    auto completed = total_completed_.load(std::memory_order_relaxed);
    if (completed == 0) return std::chrono::microseconds{0};
    auto total = total_latency_us_.load(std::memory_order_relaxed);
    return std::chrono::microseconds{total / completed};
}

} // namespace modbus_pp
