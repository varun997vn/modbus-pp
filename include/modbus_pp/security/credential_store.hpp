#pragma once

/// @file credential_store.hpp
/// @brief Abstract credential storage and an in-memory implementation.

#include "../core/result.hpp"
#include "../core/types.hpp"

#include <unordered_map>
#include <vector>

namespace modbus_pp {

/// Abstract credential store — maps device IDs to shared secrets.
class CredentialStore {
public:
    virtual ~CredentialStore() = default;

    /// Look up the shared secret for a device.
    virtual Result<std::vector<byte_t>> get_key(unit_id_t device_id) = 0;
};

/// Simple in-memory credential store for testing and development.
class InMemoryCredentialStore final : public CredentialStore {
public:
    void add_credential(unit_id_t device_id, std::vector<byte_t> key) {
        keys_[device_id] = std::move(key);
    }

    Result<std::vector<byte_t>> get_key(unit_id_t device_id) override {
        auto it = keys_.find(device_id);
        if (it == keys_.end()) {
            return ErrorCode::AuthenticationFailed;
        }
        return it->second;
    }

private:
    std::unordered_map<unit_id_t, std::vector<byte_t>> keys_;
};

} // namespace modbus_pp
