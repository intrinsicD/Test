# Math Module Roadmap

_Last Updated: 2025-05-16_

## Focus Areas

| Area | Description | Status |
| --- | --- | --- |
| SIMD validation (`MA-110`) | Ensure SIMD paths validated in CI. | ✅ Complete |
| Numerical guidance (`MA-118`) | Document solver stability and precision trade-offs. | ✅ Complete |
| Format conversions (`MA-125`) | Improve ergonomics for external format interoperability. | ✅ Complete |

## Active Task

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| _None_ | – | – | – |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `MA-130` | Identify metrics for conversion drift diagnostics. | After `MA-125` |

## Notes

- 2025-05-09: Added deterministic SIMD validation harness and regression
  suite (`engine_math_simd_tests`) covering vector arithmetic, dot/cross,
  and normalisation consistency to complete `MA-110` and unblock the
  documentation work tracked under `MA-118`.
- 2025-05-16: Published [`SOLVER_STABILITY.md`](SOLVER_STABILITY.md) with
  determinant thresholds, SVD tolerances, and operational guidance to close
  `MA-118` and unblock `MA-125` planning.
- 2025-05-20: Authored [`FORMAT_CONVERSIONS.md`](FORMAT_CONVERSIONS.md) and
  landed conversion helper tests to complete `MA-125`. Next step is defining
  telemetry metrics for conversion drift analysis (`MA-130`).

Coordinate updates with consuming modules (animation, physics) to keep guidance
consistent.
