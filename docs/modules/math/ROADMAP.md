# Math Module Roadmap

_Last Updated: 2025-05-09_

## Focus Areas

| Area | Description | Status |
| --- | --- | --- |
| SIMD validation (`MA-110`) | Ensure SIMD paths validated in CI. | ✅ Complete |
| Numerical guidance (`MA-118`) | Document solver stability and precision trade-offs. | 🟢 Planned |
| Format conversions (`MA-125`) | Improve ergonomics for external format interoperability. | 🟢 Planned |

## Active Task

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| _None_ | – | – | – |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `MA-118` | Document solver stability, include benchmarks where applicable. | After `MA-110` |
| `MA-125` | Extend conversion helpers and doc coverage. | After `MA-118` draft |

## Notes

- 2025-05-09: Added deterministic SIMD validation harness and regression
  suite (`engine_math_simd_tests`) covering vector arithmetic, dot/cross,
  and normalisation consistency to complete `MA-110` and unblock the
  documentation work tracked under `MA-118`.

Coordinate updates with consuming modules (animation, physics) to keep guidance
consistent.
