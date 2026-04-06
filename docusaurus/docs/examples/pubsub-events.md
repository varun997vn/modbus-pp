---
sidebar_position: 5
---

# Pub/Sub Events & Dead-Band Filtering

Event-driven push notifications with change detection.

## Overview

Demonstrates:

- Server-side change detection
- Pub/Sub subscriptions with dead-band filtering
- Client-side event handlers

**Result:** Server only pushes when data changes by more than the dead-band threshold, reducing noise.

## See the Code

Full source: [`examples/05_pubsub_events.cpp`](https://github.com/varuvenk/modbus_pp/blob/main/examples/05_pubsub_events.cpp)

```cpp
// Server monitors data
publisher.register_data_source(0x0000, 100, []() {
    return get_readings();
});
publisher.scan_and_notify();  // Call periodically

// Client subscribes with dead-band
subscriber.subscribe(unit, start_addr, count,
                    SubscriptionMode::OnChange, dead_band=5);
// Only notified if value changes by ≥5
```

---

Next: [Security & Authentication](./security-authentication.md)
