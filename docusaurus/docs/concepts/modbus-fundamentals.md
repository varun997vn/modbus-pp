---
sidebar_position: 2
---

# Modbus Fundamentals

To understand modbus_pp, you need to know the basics of Modbus.

## Registers

A **register** is a 16-bit (2-byte) value — the fundamental storage unit in Modbus.

```
Register 0x0000: [0x12] [0x34]  ← 2 bytes = 1 register
Register 0x0001: [0x56] [0x78]
Register 0x0002: [0xAB] [0xCD]
```

Most operations read or write **one or more contiguous registers**:

- Read 1 register: 2 bytes
- Read 4 registers: 8 bytes
- Read 100 registers: 200 bytes

## Function Codes (FC)

A **function code** specifies the operation:

| FC            | Operation                    | Standard? |
| ------------- | ---------------------------- | --------- |
| **0x01**      | Read Coils (on/off bits)     | ✓         |
| **0x02**      | Read Discrete Inputs         | ✓         |
| **0x03**      | **Read Holding Registers**   | ✓         |
| **0x04**      | Read Input Registers         | ✓         |
| **0x05**      | Write Single Coil            | ✓         |
| **0x06**      | Write Single Register        | ✓         |
| **0x0F**      | Write Multiple Coils         | ✓         |
| **0x10**      | **Write Multiple Registers** | ✓         |
| **0x2B/0x0E** | Read Device Identification   | ✓         |
| **0x6E**      | **Extended (modbus_pp)**     | ✗         |

**Most common:** FC 0x03 (read) and FC 0x10 (write).

## Frame Structure: PDU & ADU

### PDU (Protocol Data Unit)

The core operation payload:

```
[Function Code: 1B] [Operation Data: 0-252 B]
```

Example: Read 10 registers starting at address 0x0100:

```
FC    Start Addr   Count
[03]  [01] [00]    [00] [0A]
      ↑
      Read Holding Registers
```

### ADU (Application Data Unit)

The PDU wrapped with transport-specific headers:

#### **TCP ADU (MBAP)**

```
[Trans ID  ] [Proto ID  ] [Length  ] [Unit ID] [PDU          ]
[2 bytes   ] [2 bytes   ] [2 bytes ] [1 byte ] [N bytes      ]
```

- **Trans ID** — Matches requests to responses
- **Proto ID** — Always 0x0000 for Modbus
- **Length** — Bytes remaining (Unit ID + PDU)
- **Unit ID** — Device address (1-247)

#### **RTU ADU (Serial)**

```
[Unit ID] [PDU          ] [CRC16 ]
[1 byte ] [N bytes      ] [2 bytes]
```

- **Unit ID** — Device address
- **CRC16** — Error detection checksum

## Standard Modbus Example: Read 2 Registers

```
CLIENT → SERVER
TCP frame:
  Trans ID:  [00] [01]        (match this ID in response)
  Proto ID:  [00] [00]        (standard Modbus)
  Length:    [00] [06]        (6 bytes: 1 unit + 5 PDU)
  Unit ID:   [01]             (slave address 1)
  FC:        [03]             (read holding registers)
  Addr:      [01] [00]        (start address 0x0100)
  Count:     [00] [02]        (read 2 registers)

SERVER → CLIENT (response)
TCP frame:
  Trans ID:  [00] [01]        (match request ID)
  Proto ID:  [00] [00]
  Length:    [00] [07]        (7 bytes)
  Unit ID:   [01]
  FC:        [03]             (read response)
  Byte Count:[04]             (4 bytes of data)
  Data:      [12] [34] [56] [78]  (register 0x0100, 0x0101)
```

## Limitations of Standard Modbus

1. **No type system** — Everything is raw `uint16`, you track encoding separately
2. **No authentication** — Anyone can impersonate any slave
3. **Poll-only** — Slaves can't push notifications
4. **253-byte limit** — Max payload per frame
5. **Sequential** — Send request, wait, repeat (slow over high-latency links)
6. **Poor errors** — Only 11 standard error codes
7. **Byte-order confusion** — Different vendors use different orderings for multi-register values
8. **Timeless** — No timestamp in frames (hard to debug concurrent ops)

---

**Next:** See how [modbus_pp solves these problems](./problems-solved.md).
