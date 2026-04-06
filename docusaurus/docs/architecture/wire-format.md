---
sidebar_position: 3
---

# Wire Format: PDU & ADU

Detailed specification of how modbus_pp frames are structured.

## Standard Modbus PDU

```
┌───────────────────────────────┐
│ Function Code │ Data          │
│ (1 byte)      │ (0-252 bytes) │
└───────────────────────────────┘
```

**Example: Read 10 registers starting at 0x0100**

```
Function Code: 0x03 (Read Holding Registers)
Start Address: 0x0100 (2 bytes)
Quantity:      0x000A (2 bytes)

Frame: [03] [01] [00] [00] [0A]
```

## TCP ADU (MBAP Wrapper)

```
┌──────────┬──────────┬────────┬──────────┬─────────────────┐
│ Trans ID │ Proto ID │ Length │ Unit ID  │ PDU             │
│ (2B)     │ (2B)     │ (2B)   │ (1B)     │ (N bytes)       │
└──────────┴──────────┴────────┴──────────┴─────────────────┘
```

- **Trans ID**: Matches requests to responses (0x0001–0xFFFF)
- **Proto ID**: Always 0x0000 (Modbus/TCP)
- **Length**: Bytes remaining (Unit ID + PDU)
- **Unit ID**: Slave address (1–247)

## RTU ADU (Serial Wrapper)

```
┌──────────┬─────────────────┬─────────┐
│ Unit ID  │ PDU             │ CRC16   │
│ (1B)     │ (N bytes)       │ (2B)    │
└──────────┴─────────────────┴─────────┘
```

- **Unit ID**: Slave address
- **CRC16**: 16-bit cyclic redundancy check

## Extended PDU (modbus_pp, FC = 0x6E)

```
┌──────┬─────┬───────┬──────────┬────────┬───────┬──────────┬─────────┬──────┐
│ 0x6E │ Ver │ Flags │ CorrID   │ TS?    │ ExtFC │ PayLen   │ Payload │HMAC? │
│ (1B) │(1B) │ (2B)  │ (2B)     │ (8B)   │ (1B)  │ (2B)     │ (N B)   │(32B) │
└──────┴─────┴───────┴──────────┴────────┴───────┴──────────┴─────────┴──────┘
```

- **0x6E**: Extended function code (reserved by Modbus spec)
- **Ver**: Version (currently 1)
- **Flags**: Bitmap: bit 0=Timestamp, bit 1=HMAC, bits 2-15=reserved
- **CorrID**: Correlation ID for pipelining (0x0000–0xFFFF)
- **TS?**: Optional timestamp (8 bytes, only if flag bit 0 set)
- **ExtFC**: Extended function code (0x00–0xFF for sub-operations)
- **PayLen**: Payload length in bytes
- **Payload**: Operation-specific data
- **HMAC?**: Optional HMAC-SHA256 (32 bytes, only if flag bit 1 set)

## Extended Function Codes (ExtFC)

| Code | Operation                         |
| ---- | --------------------------------- |
| 0x00 | Batch read registers (optimized)  |
| 0x01 | Batch write registers (optimized) |
| 0x02 | Pub/Sub subscribe                 |
| 0x03 | Pub/Sub event notification        |
| 0x10 | Device discovery probe            |
| ...  | (more defined in code)            |

---

**Next:** [Design Patterns](./design-patterns.md)
