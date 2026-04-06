---
sidebar_position: 4
---

# Pipelined Throughput

Sequential vs. pipelined request performance comparison.

## Overview

Demonstrates:

- Submitting multiple async requests to a pipeline
- Matching responses by correlation ID
- Measuring wall-clock time improvement

**Expected result:** 8x speedup (160ms → 20ms for 16 requests over 10ms-latency link)

## See the Code

Full source: [`examples/04_pipeline_throughput.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/04_pipeline_throughput.cpp)

```cpp
// Sequential: wait for each
client.read_holding_registers(1, 0, 4);  // Wait
client.read_holding_registers(1, 4, 4);  // Wait
...

// Pipelined: all in flight
for (int i = 0; i < 16; ++i) {
    pipeline.submit(request, [](Result<PDU> resp) { /* callback */ });
}
```

---

Next: [Pub/Sub Events](./pubsub-events.md)
