---
sidebar_position: 4
---

# What's Next?

Congratulations! You've run your first modbus_pp program. Here's where to go next based on your interest:

## 🚀 **I want to build something real**

Start with the practical examples:

1. **[Basic Client/Server](../examples/basic-client-server.md)** — Full end-to-end communication with register storage
2. **[Typed Registers](../examples/typed-registers-example.md)** — Use compile-time type safety (Float32, Int64, etc.)
3. **[Batch Operations](../features/batch-operations.md)** — Optimize multiple reads into single requests
4. **[Security (HMAC)](../examples/security-authentication.md)** — Add authentication to your protocol
5. **[Pipelining](../examples/pipeline-throughput.md)** — 8x throughput improvement over high-latency links

## 📚 **I want to understand the fundamentals**

Learn the building blocks:

1. **[Modbus 101](../concepts/modbus-fundamentals.md)** — PDUs, registers, function codes, framing
2. **[Design Philosophy](../concepts/design-philosophy.md)** — Why modbus_pp is different
3. **[The 10 Problems We Solve](../concepts/problems-solved.md)** — Deep dive into each limitation and solution
4. **[Module Tour](../concepts/module-tour.md)** — What each module does and when to use it

## 🏗️ **I want to understand the architecture**

Explore the design:

1. **[Architecture Overview](../architecture/overview.md)** — Module dependency graph
2. **[Wire Formats](../architecture/wire-format.md)** — Standard Modbus + extended frames
3. **[Design Patterns](../architecture/design-patterns.md)** — Compile-time safety, monadic errors, zero-cost abstractions
4. **[Transport Abstraction](../architecture/transport-abstraction.md)** — TCP, RTU, Loopback

## 🔧 **I need the API reference**

Look up specific modules:

- **[Core Module](../api/core-module.md)** — types, error codes, PDU, result
- **[Transport Module](../api/transport-module.md)** — TCP, RTU, Loopback
- **[Security Module](../api/security-module.md)** — HMAC, authentication
- **[Pipeline Module](../api/pipeline-module.md)** — Correlation IDs, request queue
- **[Client/Server Facades](../api/client-server-facades.md)** — High-level APIs

## 📊 **I want to see performance data**

Check out the benchmarks:

- **[Benchmarks Overview](../benchmarks.md)** — 5 key benchmarks with results
  - Sequential vs. Pipelined throughput (the big win)
  - Byte-order encoding performance
  - HMAC overhead
  - Type codec efficiency

## 🧪 **I want to see what's tested**

Explore the test coverage:

- **[Test Coverage](../testing/test-coverage.md)** — 13 suites mapping features → tests
- Unit tests for each module
- Integration tests for client/server interaction
- See which examples double as integration tests

## 📖 **I need help**

- **[Glossary](../glossary.md)** — Modbus terminology (PDU, MBAP, RTU, Unit ID, etc.)
- **[FAQ](../faq.md)** — Common questions and gotchas
- **[GitHub Issues](https://github.com/varuvenk/modbus_pp/issues)** — Report bugs or ask questions

---

## Recommended Learning Path

### Path A: **"I want to ship code ASAP"** (2 hours)

1. Installation ✅ (done)
2. Hello World ✅ (done)
3. Basic Client/Server (15 min)
4. Typed Registers (15 min)
5. Pick a feature you need (Security? Pipelining?), read example + feature doc (30 min each)
6. Check API reference for details (as needed)
7. Write your code!

### Path B: **"I want to understand everything"** (4-5 hours)

1. Installation ✅ (done)
2. Hello World ✅ (done)
3. Modbus Fundamentals (20 min)
4. Design Philosophy (15 min)
5. The 10 Problems Solved (30 min)
6. Module Tour (15 min)
7. Architecture Overview (20 min)
8. Wire Formats (20 min)
9. Then pick features/examples based on what interests you
10. Benchmarks to see real-world performance (15 min)

### Path C: **"I need to integrate this into a system"** (3-4 hours)

1. Installation ✅ (done)
2. Hello World ✅ (done)
3. Architecture Overview (20 min)
4. Transport Abstraction (20 min)
5. All 7 examples, focusing on integration scenarios (1 hour)
6. Security + Authentication (30 min) — often required for production
7. Pipelining (20 min) — for high-latency networks
8. Test Coverage (20 min) — understand what's tested
9. API Reference (as needed)

---

**Pick your path above and follow it. You'll be productive in minutes, not hours!** 🎯
