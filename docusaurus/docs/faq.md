---
---

# FAQ

Frequently asked questions about modbus_pp.

## Installation & Setup

**Q: Can I use modbus_pp on Windows?**

A: Yes. Install OpenSSL via [vcpkg](https://github.com/Microsoft/vcpkg) or [pre-built binaries](https://slproweb.com/products/Win32OpenSSL.html), then build with CMake and Visual Studio 2017+.

**Q: Do I need Boost?**

A: No. modbus_pp uses only C++17 standard library + OpenSSL (optional, for HMAC).

**Q: How do I integrate modbus_pp into my CMake project?**

A: Three ways:

1. `find_package(modbus_pp)` after installing
2. `add_subdirectory(external/modbus_pp)` if using git submodule
3. Copy headers and link against lib

---

## Core Features

**Q: Should I use typed registers or raw uint16?**

A: Always use typed registers (`RegisterDescriptor`). Compile-time safety catches bugs early, zero runtime cost.

**Q: How much speedup do I get from pipelining?**

A: ~8x on high-latency links (e.g., 160ms → 20ms for 16 reqs over 10ms latency). On low-latency LAN, ~2-3x.

**Q: Is HMAC authentication required?**

A: No, it's optional. Standard Modbus frames work as-is. Enable authentication in `ClientConfig` if needed.

**Q: How does Pub/Sub dead-band filtering work?**

A: Server only pushes when value changes by **more than** the dead-band threshold. Reduces noisy sensor spam.

---

## Performance & Limitations

**Q: What's the maximum payload size in extended frames?**

A: Theoretically unlimited (2-byte length field), practically limited by your transport. TCP can handle KC of bytes comfortably.

**Q: Can I use modbus_pp on embedded systems?**

A: Yes. ~4.3K LOC, ~350KB binary, works with -fno-exceptions, fast startup.

**Q: Are there any real-time guarantees?**

A: No hard real-time guarantees. `Result<T>` avoids exceptions and dynamic allocation in hot paths, suitable for soft real-time (~millisecond jitter).

---

## Security

**Q: Is HMAC-SHA256 secure enough?**

A: Yes. HMAC-SHA256 is resistant to timing attacks, uses OpenSSL (well-audited), and provides 128-bit security (same as AES-128).

**Q: How do I share secrets between client and server?**

A: Pre-configure shared secrets in `CredentialStore`. Out-of-band secure key exchange (e.g., during device provisioning) is your responsibility.

**Q: Can I authenticate individual transactions?**

A: Yes. Extended frames can include optional HMAC digests. Set the HMAC flag in the frame header.

---

## Compatibility

**Q: Will modbus_pp work with standard Modbus devices?**

A: Yes. Standard function codes (0x01–0x10) pass through unchanged. Extended features only activate with 0x6E (extended frames).

**Q: Can I mix modbus_pp and standard Modbus clients on the same network?**

A: Yes. Standard clients ignore 0x6E frames; modbus_pp clients fallback to standard frames for legacy devices.

**Q: What Modbus versions does modbus_pp support?**

A: Modbus TCP (RFC 1006) and Modbus RTU serial. Both supported fully.

---

## Testing & Debugging

**Q: How do I test without hardware?**

A: Use `LoopbackTransport::create_pair()` for in-process client/server testing. All examples use this.

**Q: Can I simulate latency?**

A: Yes. `LoopbackTransport` supports `set_simulated_latency()` to test pipelining behavior.

**Q: How do I debug a pipelined request not receiving a response?**

A: Check:

1. Correlation ID in request matches response
2. Timeout hasn't expired (default 1000ms)
3. Server is actually sending responses (use `process_one()` loop)
4. Check error codes in `Result<PDU>`

---

## Contributing & Building

**Q: How do I build tests/benchmarks?**

A: Pass `-DMODBUS_PP_BUILD_TESTS=ON -DMODBUS_PP_BUILD_BENCHMARKS=ON` to CMake, then run `ctest`.

**Q: Can I report bugs or request features?**

A: Yes, use [GitHub Issues](https://github.com/varuvenk/modbus_pp/issues).

**Q: What coding standards does modbus_pp follow?**

A: Modern C++17, Google C++ style guide, consistent formatting, zero compiler warnings.

---

**Didn't find your answer?** Check the [Architecture](./architecture/overview.md) section or open an [issue](https://github.com/varuvenk/modbus_pp/issues).
