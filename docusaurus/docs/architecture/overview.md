---
sidebar_position: 1
---

# Architecture Overview

Understanding the design and structure of modbus_pp.

## Core Design Principles

**1. Wire Compatibility First**

- Standard Modbus frames pass through unchanged
- Legacy devices work with modbus_pp clients
- Extended features opt-in on both sides

**2. Compile-Time Safety**

- Register overlaps detected by `static_assert`
- Byte orders compile to direct instructions
- Type checking at compile time, zero runtime

**3. Zero-Overhead Abstractions**

- Pay only for features you use
- No exceptions in hot path
- Template-based optimization

## Module Dependency Graph

Every module depends on `core/`. No circular dependencies between extensions.

See [Module Tour](../concepts/module-tour.md) for detailed module descriptions.

## Next Steps

- **[Module Structure](./module-structure.md)**
- **[Wire Format](./wire-format.md)**
- **[Design Patterns](./design-patterns.md)**
- **[Transport Abstraction](./transport-abstraction.md)**
- **[Error Handling](./error-handling.md)**
