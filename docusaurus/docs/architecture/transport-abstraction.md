---
sidebar_position: 5
---

# Transport Abstraction

How modbus_pp abstracts TCP, RTU, and other transports.

## Abstract Interface

```cpp
class Transport {
public:
    virtual ~Transport() = default;

    virtual Result<void> send(span_t<const byte_t> frame) = 0;
    virtual Result<std::vector<byte_t>> receive(
        std::chrono::milliseconds timeout) = 0;
    virtual Result<void> connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const noexcept = 0;
    virtual std::string_view transport_name() const noexcept = 0;
};
```

## Implementations

### TCP Transport (MBAP)

- **POSIX sockets** for networking
- **MBAP header** (7 bytes: Trans ID, Proto ID, Length, Unit ID)
- **Transaction IDs** for request/response matching
- **TCP_NODELAY** for low latency
- Configurable **connect timeout** and **read timeout**

### RTU Transport (Serial)

- **termios** for serial configuration
- **CRC16** error detection
- **Baud rate** configuration
- **8N1** (8 data bits, no parity, 1 stop bit)
- Configurable **inter-frame delay**

### Loopback Transport (Testing)

- **In-process paired endpoints**
- **Optional simulated latency** (for benchmarking)
- **Optional error injection** (for fault tolerance testing)
- No network/serial hardware needed

## Frame Codec

Responsible for ADU wrapping/unwrapping:

```cpp
class FrameCodec {
public:
    // TCP (MBAP)
    std::vector<byte_t> wrap_mbap(const PDU& pdu, uint16_t trans_id, ...);
    Result<PDU> unwrap_mbap(span_t<const byte_t> frame);

    // RTU (CRC16)
    std::vector<byte_t> wrap_rtu(const PDU& pdu, unit_id_t unit_id);
    Result<PDU> unwrap_rtu(span_t<const byte_t> frame);
};
```

## Using Transports

```cpp
// Create a transport
std::shared_ptr<Transport> transport =
    std::make_shared<TCPTransport>(TCPConfig{
        .host = "192.168.1.10",
        .port = 502,
    });

// Or use loopback for testing
auto [client_tp, server_tp] = LoopbackTransport::create_pair();

// Pass to Client/Server
ClientConfig cfg{.transport = transport};
Client client(cfg);
```

---

**Next:** [Error Handling](./error-handling.md)
