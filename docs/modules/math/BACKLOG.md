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

| Task ID | Description | Dependency | Priority |
| --- | --- | --- | --- |
| `MA-131` | Documentation alignment audit (T-0126). | None | Medium |
| `MA-133` | Curve/spline utilities evaluation (T-0127). | Investigation phase | Optional |

## Recently Completed

| Task ID | Description | Completed |
| --- | --- | --- |
| `MA-130` | Conversion drift telemetry instrumentation. | 2025-12-02 |
| `MA-132` | Convenience rotation matrix builders (T-0125). | 2025-10-23 |

## Notes

- 2025-05-09: Added deterministic SIMD validation harness and regression
  suite (`engine_math_simd_tests`) covering vector arithmetic, dot/cross,
  and normalisation consistency to complete `MA-110` and unblock the
  documentation work tracked under `MA-118`.
- 2025-05-16: Published [`SOLVER_STABILITY.md`](SOLVER_STABILITY.md) with
  determinant thresholds, SVD tolerances, and operational guidance to close
  `MA-118` and unblock `MA-125` planning.
- 2025-05-20: Authored [`FORMAT_CONVERSIONS.md`](FORMAT_CONVERSIONS.md) and
  landed conversion helper tests to complete `MA-125`, setting up the
  conversion telemetry work delivered in `MA-130`.
- 2025-05-30: Added vector component-wise arithmetic and comparison utilities
  to close documentation gaps around advanced vector helpers and unblock
  downstream consumers requiring deterministic clamp/min/max workflows.
- 2025-10-23: Created tasks for optional enhancements: `T-0125` (convenience
  rotation builders), `T-0126` (documentation alignment), and `T-0127` (curve
  utilities evaluation). These are non-blocking improvements for API ergonomics
  and documentation accuracy.
- 2025-10-23: Created geometry module tasks `T-0128` (frustum utilities) and
  `T-0129` (shape intersection coverage). Note that frustum culling math belongs
- 2025-10-23: Completed `MA-132` (T-0125): Added convenience rotation functions
  `rotate_x()`, `rotate_y()`, `rotate_z()` in `utils_rotation.hpp` with 
  comprehensive test coverage. Updated documentation with proper API examples
  and usage guidance comparing single-axis, axis-angle, and quaternion approaches.
  in the geometry module, not math module, following proper module separation.
  The rendering visibility system (T-0122) depends on T-0128.
- 2025-10-24: Landed linear-system and polynomial solvers with stability
  guidance and regression tests to support physics/geometry intersection
  workflows ahead of the conversion telemetry instrumentation completed in `MA-130`.
- 2025-12-02: Implemented conversion telemetry aggregating vector and matrix
  drift metrics and surfaced them through runtime diagnostics to complete
  `MA-130`.

Coordinate updates with consuming modules (animation, physics) to keep guidance
consistent.
