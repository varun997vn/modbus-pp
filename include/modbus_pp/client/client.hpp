#pragma once

/// @file client.hpp
/// @brief High-level Modbus client facade composing all subsystems.
///
/// Provides a unified API for standard Modbus operations, extended typed
/// reads, batch operations, pipelining, pub/sub, authentication, and
/// device discovery.

#include "../core/pdu.hpp"
#include "../core/result.hpp"
#include "../core/timestamp.hpp"
#include "../core/types.hpp"
#include "../discovery/scanner.hpp"
#include "../pipeline/pipeline.hpp"
#include "../pubsub/subscriber.hpp"
#include "../register_map/batch_request.hpp"
#include "../register_map/type_codec.hpp"
#include "../security/credential_store.hpp"
#include "../security/session.hpp"
#include "../transport/transport.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace modbus_pp {

/// Client configuration.
struct ClientConfig {
    std::shared_ptr<Transport> transport;
    std::shared_ptr<CredentialStore> credentials;
    PipelineConfig pipeline_config{};
    bool enable_timestamps{true};
    std::chrono::milliseconds default_timeout{1000};
};

/// High-level Modbus client supporting both standard and extended operations.
class Client {
public:
    explicit Client(ClientConfig config);

    // -----------------------------------------------------------------------
    // Standard Modbus operations (wire-compatible)
    // -----------------------------------------------------------------------

    /// Read holding registers (FC 0x03).
    Result<std::vector<register_t>> read_holding_registers(
        unit_id_t unit, address_t addr, quantity_t count);

    /// Write a single register (FC 0x06).
    Result<void> write_single_register(
        unit_id_t unit, address_t addr, register_t value);

    /// Write multiple registers (FC 0x10).
    Result<void> write_multiple_registers(
        unit_id_t unit, address_t addr, span_t<const register_t> values);

    // -----------------------------------------------------------------------
    // Extended operations
    // -----------------------------------------------------------------------

    /// Execute a batch read.
    Result<BatchResponse> batch_read(unit_id_t unit, const BatchRequest& request);

    /// Access the pipeline for async operations.
    Pipeline& pipeline() { return pipeline_; }

    /// Access the subscriber for event-driven data.
    Subscriber& subscriber() { return subscriber_; }

    /// Authenticate with a device using HMAC challenge-response.
    Result<Session> authenticate(unit_id_t unit);

    /// Discover devices on the bus.
    std::vector<DeviceInfo> discover(ScannerConfig config = {});

    /// Send a raw PDU and receive the response.
    Result<PDU> transact(unit_id_t unit, const PDU& request);

private:
    ClientConfig config_;
    Pipeline pipeline_;
    Subscriber subscriber_;
    std::optional<Session> session_;
    std::uint16_t next_transaction_id_{1};
};

} // namespace modbus_pp
