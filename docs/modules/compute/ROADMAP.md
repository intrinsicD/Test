# Compute Module Roadmap

_Last Updated: 2025-10-20_

## Objectives

| Goal | Description | Status |
| --- | --- | --- |
| Dispatcher maturity | Document extension points, add dependency validation, ensure telemetry coverage. | 🔄 In Progress |
| CUDA optionality | Maintain feature flag parity across presets and CI. | 🟢 Planned |
| Runtime alignment | Coordinate with runtime job graph for future async execution. | 🟢 Planned |

## Active Tasks

| Task ID | Description | Owner | Due | Status |
| --- | --- | --- | --- | --- |
| `CO-141` | Author dispatcher extension note covering registration and telemetry. | Compute team | 2025-03-07 | ✅ Complete |
| `CO-150` | Implement kernel dependency cycle detection tooling. | Compute team | 2025-03-21 | ✅ Complete |

## Upcoming Tasks

| Task ID | Description | Dependency |
| --- | --- | --- |
| `CO-170` | Prototype runtime integration sample showing dispatcher orchestration. | After `RU-307` complete |

## Dependencies

- **Runtime module** — consumes dispatcher results, requires consistent API.
- **Build presets** — defined under `scripts/build/` to expose CUDA optionality.

Log progress in relevant task records (`T-0114`, `T-0104`) to keep sprint boards
accurate.
