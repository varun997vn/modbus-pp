#pragma once

/// @file batch_request.hpp
/// @brief Heterogeneous batch read/write operations with auto-optimization.
///
/// Standard Modbus requires individual read requests for each register range.
/// BatchRequest aggregates multiple heterogeneous reads into a single extended
/// frame, with automatic contiguous-range merging to minimize frame count.

#include "register_descriptor.hpp"
#include "../core/error.hpp"
#include "../core/result.hpp"
#include "../core/types.hpp"

#include <algorithm>
#include <vector>

namespace modbus_pp {

/// A single item in a batch request.
struct BatchItem {
    address_t    start_address;
    quantity_t   count;
    RegisterType type;
    ByteOrder    byte_order;
};

/// Aggregates multiple register read/write operations into a single request.
///
/// Use `optimized()` to merge contiguous address ranges and reduce the
/// number of individual reads required.
class BatchRequest {
public:
    /// Add a register range to the batch.
    BatchRequest& add(address_t addr, quantity_t count,
                      RegisterType type = RegisterType::UInt16,
                      ByteOrder order = ByteOrder::ABCD) {
        items_.push_back({addr, count, type, order});
        return *this;
    }

    /// Create an optimized version that merges contiguous address ranges.
    ///
    /// Ranges that are adjacent or overlapping are merged into single reads,
    /// reducing round trips. Only merges items with the same byte order.
    [[nodiscard]] BatchRequest optimized() const {
        if (items_.size() <= 1) return *this;

        auto sorted = items_;
        std::sort(sorted.begin(), sorted.end(),
                  [](const BatchItem& a, const BatchItem& b) {
                      return a.start_address < b.start_address;
                  });

        BatchRequest result;
        result.items_.push_back(sorted[0]);

        for (std::size_t i = 1; i < sorted.size(); ++i) {
            auto& last = result.items_.back();
            auto& curr = sorted[i];

            // Merge if contiguous/overlapping and same byte order
            address_t last_end = last.start_address + last.count;
            if (curr.start_address <= last_end && last.byte_order == curr.byte_order) {
                address_t new_end = std::max(last_end,
                    static_cast<address_t>(curr.start_address + curr.count));
                last.count = static_cast<quantity_t>(new_end - last.start_address);
                last.type = RegisterType::UInt16; // merged range loses specific type
            } else {
                result.items_.push_back(curr);
            }
        }

        return result;
    }

    [[nodiscard]] const std::vector<BatchItem>& items() const noexcept { return items_; }
    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }

    /// Total number of registers across all items.
    [[nodiscard]] quantity_t total_registers() const noexcept {
        quantity_t total = 0;
        for (const auto& item : items_) total += item.count;
        return total;
    }

    /// Serialize for inclusion in an extended PDU payload.
    [[nodiscard]] std::vector<byte_t> serialize() const {
        std::vector<byte_t> out;
        // Format: [item_count:2] { [addr:2][count:2][type:1][order:1] }*
        auto n = static_cast<std::uint16_t>(items_.size());
        out.push_back(static_cast<byte_t>(n >> 8));
        out.push_back(static_cast<byte_t>(n & 0xFF));

        for (const auto& item : items_) {
            out.push_back(static_cast<byte_t>(item.start_address >> 8));
            out.push_back(static_cast<byte_t>(item.start_address & 0xFF));
            out.push_back(static_cast<byte_t>(item.count >> 8));
            out.push_back(static_cast<byte_t>(item.count & 0xFF));
            out.push_back(static_cast<byte_t>(item.type));
            out.push_back(static_cast<byte_t>(item.byte_order));
        }

        return out;
    }

    /// Deserialize from a byte span.
    static Result<BatchRequest> deserialize(span_t<const byte_t> data) {
        if (data.size() < 2) return ErrorCode::DeserializationFailed;

        auto item_count = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(data[0]) << 8) | data[1]);

        if (data.size() < 2u + item_count * 6u) {
            return ErrorCode::DeserializationFailed;
        }

        BatchRequest req;
        std::size_t offset = 2;
        for (std::uint16_t i = 0; i < item_count; ++i) {
            BatchItem item;
            item.start_address = static_cast<address_t>(
                (static_cast<address_t>(data[offset]) << 8) | data[offset + 1]);
            offset += 2;
            item.count = static_cast<quantity_t>(
                (static_cast<quantity_t>(data[offset]) << 8) | data[offset + 1]);
            offset += 2;
            item.type = static_cast<RegisterType>(data[offset++]);
            item.byte_order = static_cast<ByteOrder>(data[offset++]);
            req.items_.push_back(item);
        }

        return req;
    }

private:
    std::vector<BatchItem> items_;
};

/// Result of a batch read operation, with per-item status.
class BatchResponse {
public:
    struct ItemResult {
        address_t              address;
        std::vector<register_t> registers;
        std::error_code        status;
    };

    void add_result(address_t addr, std::vector<register_t> regs,
                    std::error_code status = {}) {
        results_.push_back({addr, std::move(regs), status});
    }

    [[nodiscard]] const std::vector<ItemResult>& results() const noexcept { return results_; }
    [[nodiscard]] std::size_t size() const noexcept { return results_.size(); }

    /// @return true if all items succeeded.
    [[nodiscard]] bool all_succeeded() const noexcept {
        return std::all_of(results_.begin(), results_.end(),
                           [](const ItemResult& r) { return !r.status; });
    }

    /// Find result by starting address.
    [[nodiscard]] const ItemResult* find(address_t addr) const noexcept {
        for (const auto& r : results_) {
            if (r.address == addr) return &r;
        }
        return nullptr;
    }

    /// Serialize for transport.
    [[nodiscard]] std::vector<byte_t> serialize() const {
        std::vector<byte_t> out;
        auto n = static_cast<std::uint16_t>(results_.size());
        out.push_back(static_cast<byte_t>(n >> 8));
        out.push_back(static_cast<byte_t>(n & 0xFF));

        for (const auto& item : results_) {
            out.push_back(static_cast<byte_t>(item.address >> 8));
            out.push_back(static_cast<byte_t>(item.address & 0xFF));
            out.push_back(static_cast<byte_t>(item.status.value()));

            auto reg_count = static_cast<std::uint16_t>(item.registers.size());
            out.push_back(static_cast<byte_t>(reg_count >> 8));
            out.push_back(static_cast<byte_t>(reg_count & 0xFF));

            for (auto reg : item.registers) {
                out.push_back(static_cast<byte_t>(reg >> 8));
                out.push_back(static_cast<byte_t>(reg & 0xFF));
            }
        }

        return out;
    }

    /// Deserialize from a byte span.
    static Result<BatchResponse> deserialize(span_t<const byte_t> data) {
        if (data.size() < 2) return ErrorCode::DeserializationFailed;

        auto item_count = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(data[0]) << 8) | data[1]);

        BatchResponse resp;
        std::size_t offset = 2;

        for (std::uint16_t i = 0; i < item_count; ++i) {
            if (offset + 5 > data.size()) return ErrorCode::DeserializationFailed;

            address_t addr = static_cast<address_t>(
                (static_cast<address_t>(data[offset]) << 8) | data[offset + 1]);
            offset += 2;

            auto status_val = data[offset++];
            auto ec = status_val == 0 ? std::error_code{} :
                      make_error_code(static_cast<ErrorCode>(status_val));

            auto reg_count = static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(data[offset]) << 8) | data[offset + 1]);
            offset += 2;

            if (offset + reg_count * 2 > data.size()) {
                return ErrorCode::DeserializationFailed;
            }

            std::vector<register_t> regs(reg_count);
            for (std::uint16_t r = 0; r < reg_count; ++r) {
                regs[r] = static_cast<register_t>(
                    (static_cast<register_t>(data[offset]) << 8) | data[offset + 1]);
                offset += 2;
            }

            resp.add_result(addr, std::move(regs), ec);
        }

        return resp;
    }

private:
    std::vector<ItemResult> results_;
};

} // namespace modbus_pp
