---
sidebar_position: 4
---

# Pipelining

Send multiple requests in parallel for **8x throughput** improvement on high-latency links.

## The Problem

Standard Modbus is strictly sequential:

```
Send Req1 → Wait for Resp1 → Send Req2 → Wait for Resp2
```

Over a 10ms-latency network, 16 requests take **160ms** (16 × 10ms round trip time).

## The Solution: Pipelined Requests

Send multiple requests without waiting:

```
Send Req1, Req2, ..., Req16 → Receive Resp1, Resp2, ..., Resp16
~20ms (near-latency limited)
```

**8x faster!** (160ms → 20ms)

## How It Works

Each request is tagged with a **correlation ID**. Responses are matched back using the same ID:

```cpp
Pipeline pipeline(transport, {.max_in_flight = 16});

// Submit async with callback
pipeline.submit(request1, [](Result<PDU> resp) {
    std::cout << "Got response 1\n";
});

pipeline.submit(request2, [](Result<PDU> resp) {
    std::cout << "Got response 2\n";
});

// Or use sync (blocking)
auto resp = pipeline.submit_sync(request3, timeout);
```

## Configuration

```cpp
struct PipelineConfig {
    uint16_t max_in_flight = 16;  // Max concurrent requests
    milliseconds default_timeout = 1000ms;
};
```

## Full Example

See [Pipeline Throughput Example](../examples/pipeline-throughput.md).

---

**Next:** [Pub/Sub](./pubsub.md) or [Discovery](./discovery.md)
