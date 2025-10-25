# Compute Module Roadmap

_Last Updated: 2025-11-18_

## Objectives

| Goal | Description | Status |
| --- | --- | --- |
| Dispatcher maturity | Document extension points, add dependency validation, ensure telemetry coverage. | ✅ Complete |
| CUDA optionality | Maintain feature flag parity across presets and CI. | ✅ Complete |
| Runtime alignment | Coordinate with runtime job graph for future async execution. | ✅ Complete |

## Active Tasks

| Task ID | Description | Owner | Due | Status |
| --- | --- | --- | --- | --- |
| `CO-141` | Author dispatcher extension note covering registration and telemetry. | Compute team | 2025-03-07 | ✅ Complete |
| `CO-150` | Implement kernel dependency cycle detection tooling. | Compute team | 2025-03-21 | ✅ Complete |
| `AN-230` | Partner with animation to deliver GPU/parallel sampling benchmarks using dispatcher telemetry. | Animation & Compute | 2025-11-25 | 🟢 Active |

## Upcoming Tasks

| Task ID | Description | Dependency |
| --- | --- | --- |
| TBD | Scope follow-on dispatcher enhancements informed by `AN-230` results. | Pending benchmarking outcomes |

## Dependencies

- **Runtime module** — consumes dispatcher results, requires consistent API.
- **Build presets** — defined under `scripts/build/` to expose CUDA optionality.

## Notes

- 2025-11-18: Dispatcher-backed GPU scenario landed in the animation benchmark driver, exercising CUDA availability probes and queue/category aggregation for `AN-230.2`.

Log progress in relevant task records (`T-0114`, `T-0104`) to keep sprint boards
accurate.
