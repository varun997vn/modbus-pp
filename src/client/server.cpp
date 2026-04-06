#include "modbus_pp/client/server.hpp"

namespace modbus_pp {

Server::Server(ServerConfig config)
    : config_(std::move(config))
    , publisher_(config_.transport)
{}

void Server::on_read_holding_registers(ReadHandler handler) {
    read_handler_ = std::move(handler);
}

void Server::on_write_multiple_registers(WriteHandler handler) {
    write_handler_ = std::move(handler);
}

bool Server::process_one() {
    auto recv = config_.transport->receive(std::chrono::milliseconds{10});
    if (!recv) return false;

    auto unwrap = FrameCodec::unwrap_tcp(span_t<const byte_t>{recv.value()});
    if (!unwrap) return false;

    auto& [unit, pdu, txn_id] = unwrap.value();

    // Only handle requests for our unit ID (or broadcast 0)
    if (unit != config_.unit_id && unit != 0) return false;

    handle_request(unit, pdu, txn_id);
    return true;
}

void Server::handle_request(unit_id_t unit, const PDU& request, std::uint16_t txn_id) {
    auto fc = request.function_code();
    auto payload = request.payload();

    if (fc == FunctionCode::ReadHoldingRegisters && read_handler_) {
        if (payload.size() < 4) return;
        auto addr = static_cast<address_t>(
            (static_cast<uint16_t>(payload[0]) << 8) | payload[1]);
        auto count = static_cast<quantity_t>(
            (static_cast<uint16_t>(payload[2]) << 8) | payload[3]);

        auto result = read_handler_(addr, count);
        if (result) {
            auto& regs = result.value();
            std::vector<byte_t> resp_data;
            resp_data.push_back(static_cast<byte_t>(regs.size() * 2));
            for (auto reg : regs) {
                resp_data.push_back(static_cast<byte_t>(reg >> 8));
                resp_data.push_back(static_cast<byte_t>(reg & 0xFF));
            }
            auto resp_pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                                std::move(resp_data));
            auto frame = FrameCodec::wrap_tcp(config_.unit_id, resp_pdu, txn_id);
            config_.transport->send(span_t<const byte_t>{frame});
        } else {
            // Send exception response
            std::vector<byte_t> err_data = {
                static_cast<byte_t>(result.error().value())};
            auto err_fc = static_cast<FunctionCode>(
                static_cast<uint8_t>(fc) | 0x80);
            auto resp_pdu = PDU::make_standard(err_fc, std::move(err_data));
            auto frame = FrameCodec::wrap_tcp(config_.unit_id, resp_pdu, txn_id);
            config_.transport->send(span_t<const byte_t>{frame});
        }
    } else if (fc == FunctionCode::WriteSingleRegister && write_handler_) {
        if (payload.size() < 4) return;
        auto addr = static_cast<address_t>(
            (static_cast<uint16_t>(payload[0]) << 8) | payload[1]);
        auto val = static_cast<register_t>(
            (static_cast<uint16_t>(payload[2]) << 8) | payload[3]);

        std::vector<register_t> single_reg = {val};
        auto result = write_handler_(addr, span_t<const register_t>{single_reg});

        // Echo back the request per Modbus spec
        std::vector<byte_t> resp_data(payload.data(), payload.data() + 4);
        auto resp_pdu = PDU::make_standard(FunctionCode::WriteSingleRegister,
                                            std::move(resp_data));
        auto frame = FrameCodec::wrap_tcp(config_.unit_id, resp_pdu, txn_id);
        config_.transport->send(span_t<const byte_t>{frame});
    } else if (fc == FunctionCode::WriteMultipleRegisters && write_handler_) {
        if (payload.size() < 5) return;
        auto addr = static_cast<address_t>(
            (static_cast<uint16_t>(payload[0]) << 8) | payload[1]);
        auto count = static_cast<quantity_t>(
            (static_cast<uint16_t>(payload[2]) << 8) | payload[3]);
        // auto byte_count = payload[4]; // not needed

        std::vector<register_t> regs;
        regs.reserve(count);
        for (std::size_t i = 5; i + 1 < payload.size(); i += 2) {
            regs.push_back(static_cast<register_t>(
                (static_cast<uint16_t>(payload[i]) << 8) | payload[i + 1]));
        }

        auto result = write_handler_(addr, span_t<const register_t>{regs});
        // Echo back address and count
        std::vector<byte_t> resp_data(payload.data(), payload.data() + 4);
        auto resp_pdu = PDU::make_standard(FunctionCode::WriteMultipleRegisters,
                                            std::move(resp_data));
        auto frame = FrameCodec::wrap_tcp(config_.unit_id, resp_pdu, txn_id);
        config_.transport->send(span_t<const byte_t>{frame});
    }
}

void Server::run() {
    running_ = true;
    while (running_) {
        process_one();
        publisher_.scan_and_notify();
    }
}

void Server::stop() {
    running_ = false;
}

} // namespace modbus_pp
