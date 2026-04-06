---
sidebar_position: 5
---

# Pub/Sub: Event-Driven Notifications

Push-based data delivery with dead-band filtering to reduce bandwidth.

## The Problem

Standard Modbus is **poll-based**:

- Client repeatedly asks "has anything changed?"
- Even unchanged data triggers responses
- Wastes bandwidth and adds latency

## The Solution: Server-Driven Pub/Sub

**Server monitors data and pushes notifications** only when meaningful changes occur:

```cpp
// Server registers a data source
Publisher& pub = server.publisher();
pub.register_data_source(0x0000, 100, []() {
    return get_sensor_readings();
});

// Periodically scan for changes
pub.scan_and_notify();
```

**Client subscribes with dead-band filtering**:

```cpp
// Subscribe: notify only if value changes by ≥5
SubscriptionRequest sub{
    .start_address = 0x0000,
    .count = 100,
    .trigger_mode = SubscriptionMode::OnChange,
    .dead_band = 5,
};

subscriber.subscribe(unit_id, sub);

// Handler called when notification arrives
```

## Benefits

- **Event-driven** — Low latency when data changes
- **Bandwidth-efficient** — Dead-band filters noise (e.g., from noisy sensors)
- **Backward compatible** — Standard clients still poll if they want
- **Optional** — Can mix Pub/Sub and polling on the same server

## Full Example

See [Pub/Sub Events Example](../examples/pubsub-events.md).

---

**Next:** [Batch Operations](./batch-operations.md) or [Security](./security-hmac.md)
