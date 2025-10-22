# Math Module

## Current State
- Provides vector, matrix, quaternion, and transform primitives plus
  orthonormal basis helpers used across animation, geometry, and physics.
- Supplies numerical utilities and conversions consumed by runtime systems.
- Ships solver stability guidance covering determinant thresholds, SVD
  tolerances, and transform decomposition caveats in
  [`solver_stability.md`](solver_stability.md).
- Documents external format conversions in
  [`format_conversions.md`](format_conversions.md), including glTF/USD/DirectX
  layout guidance.

## Usage
- Build with `cmake --build --preset <preset> --target engine_math`.
- Include `<engine/math/*>` headers as needed by dependent modules.
- Run `ctest --preset <preset> --tests-regex engine_math` to execute unit tests,
  and `ctest --preset <preset> --tests-regex engine_math_simd` to run the SIMD
  validation harness introduced in `MA-110`.

## TODO / Next Steps

- Monitor adoption of the new [format conversion helpers](format_conversions.md)
  and scope telemetry hooks tracked as `MA-130` in
  [`docs/ROADMAP.md`](../../ROADMAP.md) to surface conversion drift diagnostics.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `MA-110` | Introduce SIMD validation harness (`TI-003`). | Add SIMD regression suite and hook into CI. | ✅ Done |
| `MA-118` | Document solver stability ranges. | Guidance captured in [`solver_stability.md`](solver_stability.md) and linked here. | ✅ Done |
| `MA-125` | Provide external-format conversion cheatsheet. | Document conversion helpers and add sample tests. | ✅ Done |

Refer to [ROADMAP.md](ROADMAP.md) for scheduling.
