---
---

# Glossary

Common Modbus and modbus_pp terminology.

## Core Terms

**ADU** (Application Data Unit)
Protocol Data Unit (PDU) wrapped with transport-specific headers (MBAP for TCP, CRC16 for RTU).

**MBAP** (Modbus Application Protocol)
7-byte header for TCP (Transaction ID, Protocol ID, Length, Unit ID).

**PDU** (Protocol Data Unit)
Core payload of a Modbus frame (Function Code + operation data).

**FC** (Function Code)
1-byte operation identifier (0x03 = Read Holding Registers, etc.).

**Register**
16-bit (2-byte) data cell in Modbus. Multi-register values (Float32, Int64) span 2–4 registers.

**Unit ID** (Slave ID, Device Address)
Identifies a device on the Modbus bus (1–247). TCP ignores it; RTU uses it.

**Transaction ID**
Matches read requests to responses in TCP MBAP. Incremented per transaction.

**CRC16** (Cyclic Redundancy Check)
2-byte error detection checksum in RTU frames. Detects transmission errors.

## Protocol Terms

**Coil**
Single bit (on/off) in Modbus. Read via FC 0x01, write via FC 0x05.

**Discrete Input**
Read-only coil. FC 0x02.

**Holding Register**
Writable 16-bit value. Read via FC 0x03, write via FC 0x06 or 0x10.

**Input Register**
Read-only 16-bit value. FC 0x04.

**Broadcast**
Writing to Unit ID 0 (nonstandard, transmitted to all devices).

## modbus_pp-Specific Terms

**Correlation ID**
Unique identifier (0–65535) for matching pipelined requests to responses.

**Dead-band**
Minimum change threshold before a Pub/Sub notification is triggered (reduces noise).

**Extended Frame** (ext PDU)
modbus_pp frame using FC 0x6E with larger payload and optional timestamps/HMAC.

**Pipelining**
Sending multiple requests without waiting for responses (matched by correlation ID).

**Pub/Sub**
Event-driven push notifications from server to client when data changes.

**RegisterDescriptor**
Compile-time definition binding address, type, count, and byte order.

**RegisterMap**
Bundle of RegisterDescriptors with compile-time overlap validation.

**`Result<T>`**
Monadic type holding either value T or error code (no exceptions).

**StrongType**
Type-safe wrapper around primitives (e.g., RegisterAddress vs. RegisterCount).

## Byte-Order Terms

**Byte Order** (Endianness)
How multi-byte values are arranged across Modbus registers:

- **ABCD** — Big-endian (Modbus default)
- **DCBA** — Little-endian
- **BADC** — Byte-swapped
- **CDAB** — Word-swapped

**BSWAP**
CPU instruction to swap byte order (e.g., big ↔ little endian).

**Endian**
Byte ordering convention (big or little). Different vendors use different conventions.

## Transport Terms

**Loopback**
In-process virtual transport for testing (paired endpoints, no network).

**RTU** (Remote Terminal Unit)
Serial Modbus framing (Unit ID + PDU + CRC16).

**TCP**
TCP/IP Modbus framing with MBAP header. Default port 502.

**TCP_NODELAY**
Socket option disabling Nagle's algorithm for lower latency.

**Termios**
POSIX serial port configuration interface (used by RTU transport).

## Error Terms

**Exception Code**
Modbus error response (FC | 0x80). Values 0x01–0x0B per spec.

**Error Category**
std::error_code classification (enables standard C++ error handling).

**HMAC**
Hash-based Message Authentication Code (proves sender identity + message integrity).

**Nonce**
Random value (16 bytes in modbus_pp) preventing replay attacks.

---

**Need a term clarified?** Check the [Architecture](./architecture/overview.md) or [Concepts](./concepts/overview.md) sections.
