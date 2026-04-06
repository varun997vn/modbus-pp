---
sidebar_position: 3
---

# Security & HMAC Authentication

Add HMAC-SHA256 authentication to verify device identity and message integrity.

## The Problem

Standard Modbus has **no authentication**:

- Any device can impersonate any slave address
- No integrity verification
- Vulnerable to man-in-the-middle attacks

## The Solution: Challenge-Response Handshake

modbus_pp uses a **4-step HMAC-SHA256 handshake**:

```
Step 1: Client sends nonce Nc (16 random bytes)
Step 2: Server responds with nonce Ns + HMAC(Nc || Ns, shared_key)
Step 3: Client proves knowledge of key with HMAC(Ns || Nc, shared_key)
Step 4: Session established; subsequent frames can include HMAC
```

## Key Features

- **Mutual authentication** — both sides prove they know the shared secret
- **No passwords over the wire** — only nonces and HMACs
- **Constant-time comparison** — prevents timing attacks
- **OpenSSL integration** — standard HMAC-SHA256

## Usage Example

```cpp
// Pre-share credentials
auto cred_store = std::make_shared<CredentialStore>();
cred_store->add_credential(unit_id_1, "shared_secret_key_123");
cred_store->add_credential(unit_id_2, "another_secret_key_456");

// Create client with authentication
ClientConfig cfg{
    .transport = transport,
    .credentials = cred_store,
};
Client client(cfg);

// All extended frames are now authenticated!
auto result = client.read_holding_registers(unit_id_1, 0, 10);
```

## Server-Side Authentication

```cpp
ServerConfig srv_cfg{
    .transport = server_transport,
    .credentials = cred_store,
};
Server server(srv_cfg);

// Server authenticates clients automatically
```

## Full Example

See [Security & Authentication Example](../examples/security-authentication.md).

---

**Next:** [Pipelining](./pipelining.md) or [Pub/Sub](./pubsub.md)
