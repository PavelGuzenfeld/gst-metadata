# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.1.x   | Yes       |

## Reporting a Vulnerability

To report a security vulnerability, please open a GitHub issue or contact the maintainer directly.

## Security Testing

Every CI run includes:

- **AddressSanitizer (ASan)** -- detects heap/stack/global buffer overflows, use-after-free, and memory leaks.
- **UndefinedBehaviorSanitizer (UBSan)** -- detects signed integer overflow, null pointer dereference, and other undefined behavior.
- **ThreadSanitizer (TSan)** -- detects data races and deadlocks.
- **cppcheck** -- static analysis for warnings, style, and performance issues.
- **clang-tidy** -- warnings-as-errors static analysis.

## Design Principles

- **POD-only payloads** -- no virtual destructors, no heap allocations in metadata.
- **Zero-copy** -- metadata is allocated inline in the GStreamer buffer metadata system.
- **No string operations** -- eliminates buffer overflow risk in the core library.
- **Defensive null checks** -- all public methods use `g_return_val_if_fail` / `g_return_if_fail`.
