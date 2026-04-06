---
sidebar_position: 2
---

# Module Structure

A breakdown of each module and its responsibilities.

## Layered Architecture

```
┌─────────────────────────────────┐
│  Client / Server Facades        │  User-facing API
├─────────────────────────────────┤
│ Pipeline | PubSub | Security    │  Extension modules
│ Discovery | RegisterMap         │  (opt-in features)
├─────────────────────────────────┤
│  Transport (TCP, RTU, Loopback) │  Communication
├─────────────────────────────────┤
│  Core (types, error, result,    │  Foundation
│  pdu, endian, timestamp, fc)    │
└─────────────────────────────────┘
```

## Core Layer Details

| Module                 | Responsibility                                    |
| ---------------------- | ------------------------------------------------- |
| **types.hpp**          | Base types (`byte_t`, `register_t`, `StrongType`) |
| **error.hpp**          | 25+ error codes, std::error_category              |
| **result.hpp**         | `Result<T>` monad for error handling                |
| **pdu.hpp**            | PDU serialization/deserialization                 |
| **endian.hpp**         | 4 byte order conversions                          |
| **timestamp.hpp**      | Microsecond stamps                                |
| **function_codes.hpp** | FC enums                                          |

## Transport Layer Details

| Module                     | Purpose                     |
| -------------------------- | --------------------------- |
| **tcp_transport.hpp**      | POSIX sockets + MBAP        |
| **rtu_transport.hpp**      | Serial + termios + CRC16    |
| **loopback_transport.hpp** | In-process paired endpoints |
| **frame_codec.hpp**        | ADU wrapping (MBAP/CRC16)   |

## Extension Modules

| Module            | When to Use                |
| ----------------- | -------------------------- |
| **register_map/** | Type-safe register access  |
| **security/**     | HMAC-SHA256 authentication |
| **pipeline/**     | High-latency throughput    |
| **pubsub/**       | Event-driven notifications |
| **discovery/**    | Auto-device discovery      |

## Dependency Flow

```
User Code
   ↓
Client/Server (high-level API)
   ↓
[Pipeline] [PubSub] [Security] [Discovery] [RegisterMap]
   ↓
Transport (TCP/RTU/Loopback)
   ↓
Core (types, error, result, pdu, ...)
```

Every extension module:

- Depends on **Core** (always)
- Depends on **Transport** (often)
- Independent of other extensions

---

**Next:** [Wire Format](./wire-format.md)
