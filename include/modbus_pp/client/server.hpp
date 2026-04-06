#pragma once

/// @file server.hpp
/// @brief High-level Modbus server/slave with handler registration.

#include "../core/pdu.hpp"
#include "../core/result.hpp"
#include "../core/types.hpp"
#include "../pubsub/publisher.hpp"
#include "../transport/transport.hpp"
#include "../transport/frame_codec.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

namespace modbus_pp {

/// Server configuration.
struct ServerConfig {
    std::shared_ptr<Transport> transport;
    unit_id_t unit_id{1};
    bool enable_timestamps{true};
};

/// High-level Modbus server that dispatches requests to registered handlers.
class Server {
public:
    explicit Server(ServerConfig config);

    using ReadHandler = std::function<Result<std::vector<register_t>>(address_t, quantity_t)>;
    using WriteHandler = std::function<Result<void>(address_t, span_t<const register_t>)>;

    /// Register a handler for Read Holding Registers (FC 0x03).
    void on_read_holding_registers(ReadHandler handler);

    /// Register a handler for Write Multiple Registers (FC 0x10).
    void on_write_multiple_registers(WriteHandler handler);

    /// Access the publisher for push notifications.
    Publisher& publisher() { return publisher_; }

    /// Process one incoming request (non-blocking).
    /// @return true if a request was processed.
    bool process_one();

    /// Run the server event loop (blocking).
    void run();

    /// Stop the server event loop.
    void stop();

    [[nodiscard]] bool is_running() const noexcept { return running_; }

private:
    void handle_request(unit_id_t unit, const PDU& request, std::uint16_t txn_id);

    ServerConfig config_;
    Publisher publisher_;
    ReadHandler read_handler_;
    WriteHandler write_handler_;
    std::atomic<bool> running_{false};
};

} // namespace modbus_pp
