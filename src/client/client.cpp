#include "modbus_pp/client/client.hpp"
#include "modbus_pp/transport/frame_codec.hpp"

namespace modbus_pp {

Client::Client(ClientConfig config)
    : config_(std::move(config))
    , pipeline_(config_.transport, config_.pipeline_config)
    , subscriber_(config_.transport)
{}

Result<PDU> Client::transact(unit_id_t unit, const PDU& request) {
    auto txn_id = next_transaction_id_++;
    auto frame = FrameCodec::wrap_tcp(unit, request, txn_id);

    auto send_result = config_.transport->send(span_t<const byte_t>{frame});
    if (!send_result) return send_result.error();

    auto recv_result = config_.transport->receive(config_.default_timeout);
    if (!recv_result) return recv_result.error();

    auto unwrap = FrameCodec::unwrap_tcp(span_t<const byte_t>{recv_result.value()});
    if (!unwrap) return unwrap.error();

    auto& [resp_unit, resp_pdu, resp_txn] = unwrap.value();

    // Check for Modbus exception response (FC has high bit set)
    auto fc = static_cast<uint8_t>(resp_pdu.function_code());
    if (fc & 0x80) {
        auto payload = resp_pdu.payload();
        if (payload.size() >= 1) {
            return make_error_code(static_cast<ErrorCode>(payload[0]));
        }
        return ErrorCode::InvalidFrame;
    }

    return std::move(resp_pdu);
}

Result<std::vector<register_t>> Client::read_holding_registers(
    unit_id_t unit, address_t addr, quantity_t count) {
    std::vector<byte_t> data = {
        static_cast<byte_t>(addr >> 8), static_cast<byte_t>(addr & 0xFF),
        static_cast<byte_t>(count >> 8), static_cast<byte_t>(count & 0xFF)
    };
    auto request = PDU::make_standard(FunctionCode::ReadHoldingRegisters, std::move(data));

    auto result = transact(unit, request);
    if (!result) return result.error();

    auto& pdu = result.value();
    auto payload = pdu.payload();
    if (payload.size() < 1) return ErrorCode::InvalidFrame;

    auto byte_count = payload[0];
    if (payload.size() < 1u + byte_count) return ErrorCode::InvalidFrame;

    std::vector<register_t> registers;
    registers.reserve(byte_count / 2);
    for (std::size_t i = 1; i + 1 < payload.size(); i += 2) {
        registers.push_back(static_cast<register_t>(
            (static_cast<uint16_t>(payload[i]) << 8) | payload[i + 1]));
    }

    return registers;
}

Result<void> Client::write_single_register(
    unit_id_t unit, address_t addr, register_t value) {
    std::vector<byte_t> data = {
        static_cast<byte_t>(addr >> 8), static_cast<byte_t>(addr & 0xFF),
        static_cast<byte_t>(value >> 8), static_cast<byte_t>(value & 0xFF)
    };
    auto request = PDU::make_standard(FunctionCode::WriteSingleRegister, std::move(data));
    auto result = transact(unit, request);
    if (!result) return result.error();
    return Result<void>{};
}

Result<void> Client::write_multiple_registers(
    unit_id_t unit, address_t addr, span_t<const register_t> values) {
    auto count = static_cast<quantity_t>(values.size());
    auto byte_count = static_cast<byte_t>(count * 2);

    std::vector<byte_t> data;
    data.reserve(5 + byte_count);
    data.push_back(static_cast<byte_t>(addr >> 8));
    data.push_back(static_cast<byte_t>(addr & 0xFF));
    data.push_back(static_cast<byte_t>(count >> 8));
    data.push_back(static_cast<byte_t>(count & 0xFF));
    data.push_back(byte_count);

    for (std::size_t i = 0; i < values.size(); ++i) {
        data.push_back(static_cast<byte_t>(values[i] >> 8));
        data.push_back(static_cast<byte_t>(values[i] & 0xFF));
    }

    auto request = PDU::make_standard(FunctionCode::WriteMultipleRegisters, std::move(data));
    auto result = transact(unit, request);
    if (!result) return result.error();
    return Result<void>{};
}

Result<BatchResponse> Client::batch_read(unit_id_t unit, const BatchRequest& request) {
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::BatchRead;
    if (config_.enable_timestamps) {
        hdr.timestamp = Timestamp::now();
    }

    auto pdu = PDU::make_extended(hdr, request.serialize());
    auto result = transact(unit, pdu);
    if (!result) return result.error();

    return BatchResponse::deserialize(result.value().payload());
}

Result<Session> Client::authenticate(unit_id_t unit) {
    if (!config_.credentials) return ErrorCode::AuthenticationFailed;

    auto key_result = config_.credentials->get_key(unit);
    if (!key_result) return key_result.error();

    auto& shared_secret = key_result.value();
    auto client_nonce = HMACAuth::generate_nonce();

    // Step 1: Send AuthChallenge with client_nonce
    FrameHeader hdr;
    hdr.ext_function_code = ExtendedFunctionCode::AuthChallenge;
    std::vector<byte_t> payload(client_nonce.begin(), client_nonce.end());
    auto challenge_pdu = PDU::make_extended(hdr, std::move(payload));

    auto step2 = transact(unit, challenge_pdu);
    if (!step2) return step2.error();

    // Step 2: Server responds with server_nonce + HMAC
    auto resp_payload = step2.value().payload();
    if (resp_payload.size() < 48) return ErrorCode::AuthenticationFailed; // 16 + 32

    Nonce server_nonce;
    std::copy_n(resp_payload.data(), 16, server_nonce.begin());

    HMACDigest server_hmac;
    std::copy_n(resp_payload.data() + 16, 32, server_hmac.begin());

    // Verify server's response
    auto expected = compute_auth_challenge_response(
        client_nonce, server_nonce, span_t<const byte_t>{shared_secret});
    if (expected != server_hmac) return ErrorCode::HMACMismatch;

    // Step 3: Send AuthResponse
    auto client_response = compute_auth_response(
        client_nonce, server_nonce, span_t<const byte_t>{shared_secret});
    FrameHeader resp_hdr;
    resp_hdr.ext_function_code = ExtendedFunctionCode::AuthResponse;
    std::vector<byte_t> resp_data(client_response.begin(), client_response.end());
    auto auth_resp_pdu = PDU::make_extended(resp_hdr, std::move(resp_data));

    auto step4 = transact(unit, auth_resp_pdu);
    if (!step4) return step4.error();

    // Step 4: Server confirms with session token
    auto session_payload = step4.value().payload();
    if (session_payload.size() < 24) return ErrorCode::AuthenticationFailed; // 16 token + 8 expiry

    SessionToken token;
    std::copy_n(session_payload.data(), 16, token.begin());
    auto expiry = Timestamp::deserialize(session_payload.data() + 16);

    auto session_key = derive_session_key(
        client_nonce, server_nonce, span_t<const byte_t>{shared_secret});

    session_ = Session::create(token, std::move(session_key), expiry);
    return *session_;
}

std::vector<DeviceInfo> Client::discover(ScannerConfig config) {
    Scanner scanner(config_.transport, config);
    return scanner.scan();
}

} // namespace modbus_pp
