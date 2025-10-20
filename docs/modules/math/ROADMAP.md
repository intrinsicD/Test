# Math Module Roadmap

_Last Updated: 2025-02-19_

## Focus Areas

| Area | Description | Status |
| --- | --- | --- |
| SIMD validation (`MA-110`) | Ensure SIMD paths validated in CI. | 🔄 In Progress |
| Numerical guidance (`MA-118`) | Document solver stability and precision trade-offs. | 🟢 Planned |
| Format conversions (`MA-125`) | Improve ergonomics for external format interoperability. | 🟢 Planned |

## Active Task

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| `MA-110` | Math team | 2025-03-14 | 🔄 In Progress |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `MA-118` | Document solver stability, include benchmarks where applicable. | After `MA-110` |
| `MA-125` | Extend conversion helpers and doc coverage. | After `MA-118` draft |

Coordinate updates with consuming modules (animation, physics) to keep guidance
consistent.
