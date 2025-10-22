# Math Module Roadmap

_Last Updated: 2025-05-16_

## Focus Areas

| Area | Description | Status |
| --- | --- | --- |
| SIMD validation (`MA-110`) | Ensure SIMD paths validated in CI. | ✅ Complete |
| Numerical guidance (`MA-118`) | Document solver stability and precision trade-offs. | ✅ Complete |
| Format conversions (`MA-125`) | Improve ergonomics for external format interoperability. | 🟢 Planned |

## Active Task

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| _None_ | – | – | – |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `MA-125` | Extend conversion helpers and doc coverage. | After `MA-118` draft |

## Notes

- 2025-05-09: Added deterministic SIMD validation harness and regression
  suite (`engine_math_simd_tests`) covering vector arithmetic, dot/cross,
  and normalisation consistency to complete `MA-110` and unblock the
  documentation work tracked under `MA-118`.
- 2025-05-16: Published [`solver_stability.md`](solver_stability.md) with
  determinant thresholds, SVD tolerances, and operational guidance to close
  `MA-118` and unblock `MA-125` planning.

Coordinate updates with consuming modules (animation, physics) to keep guidance
consistent.
