#include "modbus_pp/core/error.hpp"

namespace modbus_pp {

const char* ModbusCategory::name() const noexcept {
    return "modbus_pp";
}

std::string ModbusCategory::message(int ev) const {
    switch (static_cast<ErrorCode>(ev)) {
        case ErrorCode::Success:                  return "Success";
        case ErrorCode::IllegalFunction:          return "Illegal function";
        case ErrorCode::IllegalDataAddress:       return "Illegal data address";
        case ErrorCode::IllegalDataValue:         return "Illegal data value";
        case ErrorCode::SlaveDeviceFailure:       return "Slave device failure";
        case ErrorCode::Acknowledge:              return "Acknowledge";
        case ErrorCode::SlaveDeviceBusy:          return "Slave device busy";
        case ErrorCode::NegativeAcknowledge:      return "Negative acknowledge";
        case ErrorCode::MemoryParityError:        return "Memory parity error";
        case ErrorCode::GatewayPathUnavailable:   return "Gateway path unavailable";
        case ErrorCode::GatewayTargetFailed:      return "Gateway target device failed to respond";
        case ErrorCode::AuthenticationFailed:     return "Authentication failed";
        case ErrorCode::SessionExpired:           return "Session expired";
        case ErrorCode::HMACMismatch:             return "HMAC verification failed";
        case ErrorCode::PipelineOverflow:         return "Pipeline capacity exceeded";
        case ErrorCode::SubscriptionLimitReached: return "Subscription limit reached";
        case ErrorCode::PayloadTooLarge:          return "Payload exceeds maximum size";
        case ErrorCode::FragmentReassemblyFailed: return "Fragment reassembly failed";
        case ErrorCode::TransportDisconnected:    return "Transport disconnected";
        case ErrorCode::TimeoutExpired:           return "Timeout expired";
        case ErrorCode::TypeCodecMismatch:        return "Type codec mismatch";
        case ErrorCode::UnsupportedVersion:       return "Unsupported protocol version";
        case ErrorCode::CorrelationMismatch:      return "Correlation ID mismatch";
        case ErrorCode::BatchPartialFailure:      return "Batch operation partially failed";
        case ErrorCode::InvalidFrame:             return "Invalid frame format";
        case ErrorCode::CrcMismatch:              return "CRC mismatch";
        case ErrorCode::DeserializationFailed:    return "Deserialization failed";
        default:                                  return "Unknown error (" + std::to_string(ev) + ")";
    }
}

const std::error_category& modbus_category() noexcept {
    static const ModbusCategory instance;
    return instance;
}

std::error_code make_error_code(ErrorCode e) noexcept {
    return {static_cast<int>(e), modbus_category()};
}

} // namespace modbus_pp
