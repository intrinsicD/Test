# Geometry Module Roadmap

_Last Updated: 2025-02-19_

## Workstreams

| Stream | Description | Status |
| --- | --- | --- |
| Benchmarking (`GE-205`) | Quantify performance of normals/bounds recomputation. | 🔄 In Progress |
| Remeshing proposal (`GE-212`) | Define scope for remeshing & parameterisation upgrades. | 🟢 Planned |
| Telemetry alignment (`GE-220`) | Emit diagnostics compatible with telemetry viewer. | 🟢 Planned |

## Active Tasks

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| `GE-205` | Geometry team | 2025-03-14 | 🔄 In Progress |

## Upcoming Tasks

| Task ID | Description | Dependency |
| --- | --- | --- |
| `GE-212` | Draft remeshing/parameterisation RFP referencing design constraints. | None |
| `GE-220` | Instrument spatial query telemetry and update README. | Schema ready (`CC-001.1`); wire metrics via diagnostics bridge |

> **Staffing Guidance:** `GE-212` focuses on planning and RFP authorship, while `GE-220` delivers telemetry instrumentation. Apart from consuming the diagnostics schema from `CC-001`, the efforts are independent, so assign separate agents and sync via documentation check-ins.

## Dependencies

- **Testing infrastructure (`TI-002`)** — hosts benchmark harness.
- **Diagnostics initiative (`CC-001`)** — provides telemetry schema (now
  documented in `design/telemetry_schema.md`).

Document findings in the relevant task files (`T-0112`) and update the central
roadmap after each milestone.
