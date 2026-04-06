---
sidebar_position: 5
---

# Pub/Sub Module API

## Overview

The Pub/Sub module provides event-driven push notifications from Modbus servers to clients.

## Classes

### `Publisher`

Server-side event publisher.

```cpp
class Publisher {
public:
  void subscribe(uint16_t registerId, Callback listener);
  void publish(uint16_t registerId, const Value& newValue);
};
```

### `Subscriber`

Client-side subscription receiver.

```cpp
class Subscriber {
public:
  Result<void> subscribe(uint16_t registerId);
  Result<Value> waitForNotification(std::chrono::milliseconds timeout);
};
```

### `DeadBandFilter`

Configurable hysteresis filter for notifications.

```cpp
class DeadBandFilter {
public:
  bool shouldNotify(const Value& oldValue, const Value& newValue, float deadBand);
};
```

---

**Full API documentation coming soon.**

[View source on GitHub](https://github.com/varuvenk/modbus_pp/tree/main/src/pubsub)
