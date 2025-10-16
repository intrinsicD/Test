# Error Handling Migration Guide

## Overview

`DC-004` standardises error handling across the engine by introducing typed error
codes and the `engine::Result<T, ErrorCode>` helper for propagating recoverable
failures. This document explains the new primitives, outlines migration
expectations, and captures best practices for module authors.

## Core Types

- **`engine::ErrorCode`** – Describes an error using a stable domain string,
  integral value, and identifier. Modules provide context via derived or
  enumerated codes. Use the `with_message` helper when runtime context (file
  paths, configuration values) is required.
- **`engine::EnumeratedErrorCode<Enum>`** – Convenience wrapper for enum-based
  error families. The IO module publishes `GeometryIoErrorCode` via this helper
  and exposes a string identifier for telemetry.
- **`engine::Result<T, Error>`** – Lightweight carrier that stores either a
  value or an error. `Result<void, Error>` represents success/failure when no
  payload is required. Always mark API functions `[[nodiscard]]` when returning
  `Result` to enforce call-site checking.

## Migration Expectations

1. Replace public functions that previously threw exceptions with
   `engine::Result`. Throwing remains acceptable only for programmer errors or
   unrecoverable invariants (e.g., allocation failures, logic bugs).
2. Define a module-specific error enum and helper that returns an
   `EnumeratedErrorCode`. This keeps telemetry stable and avoids stringly typed
   comparisons.
3. Update call sites to branch on `result.has_value()` (or `if (result)`), log or
   surface `result.error()`, and avoid assuming success.
4. Bubble documentation updates through module READMEs and the central roadmap
   when behaviour changes, and record test coverage that exercises both success
   and failure paths.

## Implementation Checklist

- [x] `engine/core/diagnostics/error.hpp` defines `ErrorCode` and
      `EnumeratedErrorCode`.
- [x] `engine/core/diagnostics/result.hpp` provides `Result<T, Error>` with a
      dedicated `void` specialisation.
- [x] The IO module consumes the new primitives via `GeometryIoResult<T>` and
      `GeometryIoErrorCode`, eliminating exception-based flow control from its
      public API.
- [x] Unit tests assert on error codes (e.g., missing files, invalid
      arguments), ensuring regression coverage.

## Next Steps

- Roll the pattern into the Assets and Runtime modules so cache reload paths
  and subsystem orchestration emit structured errors instead of exceptions.
- Extend telemetry scripts to serialise error identifiers for diagnostics.
- Update Python bindings to translate `engine::Result` into idiomatic
  exceptions with preserved error metadata.
