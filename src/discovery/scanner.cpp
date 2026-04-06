#include "modbus_pp/discovery/scanner.hpp"
#include "modbus_pp/core/function_codes.hpp"
#include "modbus_pp/core/pdu.hpp"
#include "modbus_pp/transport/frame_codec.hpp"

namespace modbus_pp {

Scanner::Scanner(std::shared_ptr<Transport> transport, ScannerConfig config)
    : transport_(std::move(transport)), config_(config) {}

Result<DeviceInfo> Scanner::probe(unit_id_t unit_id) {
    // Send a Read Holding Registers request for register 0, count 1
    // This is the simplest probe: if the device responds, it's alive
    auto pdu = PDU::make_standard(FunctionCode::ReadHoldingRegisters,
                                   {0x00, 0x00, 0x00, 0x01});
    auto frame = FrameCodec::wrap_tcp(unit_id, pdu, unit_id);

    auto send_result = transport_->send(span_t<const byte_t>{frame});
    if (!send_result) return send_result.error();

    auto recv_result = transport_->receive(config_.scan_timeout);
    if (!recv_result) return recv_result.error();

    // Device responded — it's alive
    DeviceInfo info;
    info.unit_id = unit_id;
    info.discovered_at = Timestamp::now();

    // Try to detect extended capabilities by sending an extended discover PDU
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::Discover;
    auto ext_pdu = PDU::make_extended(hdr, {});
    auto ext_frame = FrameCodec::wrap_tcp(unit_id, ext_pdu, static_cast<uint16_t>(unit_id + 256));

    auto ext_send = transport_->send(span_t<const byte_t>{ext_frame});
    if (ext_send) {
        auto ext_recv = transport_->receive(config_.scan_timeout);
        if (ext_recv) {
            auto unwrap = FrameCodec::unwrap_tcp(span_t<const byte_t>{ext_recv.value()});
            if (unwrap) {
                auto& [uid, resp_pdu, txn] = unwrap.value();
                if (resp_pdu.is_extended() &&
                    resp_pdu.extended_header()->ext_function_code ==
                        ExtendedFunctionCode::DiscoverResponse) {
                    info.supports_extended = true;
                    info.extension_version = 1;

                    // Parse capabilities from payload if present
                    auto payload = resp_pdu.payload();
                    if (payload.size() >= 4) {
                        info.capabilities = static_cast<Capability>(
                            (static_cast<uint32_t>(payload[0]) << 24) |
                            (static_cast<uint32_t>(payload[1]) << 16) |
                            (static_cast<uint32_t>(payload[2]) << 8) |
                            static_cast<uint32_t>(payload[3]));
                    }
                }
            }
        }
    }

    return info;
}

std::vector<DeviceInfo> Scanner::scan() {
    std::vector<DeviceInfo> devices;

    for (unit_id_t id = config_.range_start; id <= config_.range_end; ++id) {
        auto result = probe(id);
        if (result.has_value()) {
            devices.push_back(std::move(result).value());
        }

        if (id == config_.range_end) break; // prevent overflow for uint8_t
    }

    return devices;
}

void Scanner::scan_async(std::function<void(DeviceInfo)> on_found,
                          std::function<void()> on_complete) {
    for (unit_id_t id = config_.range_start; id <= config_.range_end; ++id) {
        auto result = probe(id);
        if (result.has_value()) {
            on_found(std::move(result).value());
        }
        if (id == config_.range_end) break;
    }
    if (on_complete) on_complete();
}

} // namespace modbus_pp
