# Geometry Module Roadmap

_Last Updated: 2025-03-25_

## Workstreams

| Stream | Description | Status |
| --- | --- | --- |
| Benchmarking (`GE-205`) | Quantify performance of normals/bounds recomputation. | ✅ Done |
| Remeshing proposal (`GE-212`) | Define scope for remeshing & parameterisation upgrades. | ✅ Done |
| Telemetry alignment (`GE-220`) | Emit diagnostics compatible with telemetry viewer. | ✅ Done |

## Active Tasks

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| `GE-205` | Geometry team | 2025-03-14 | ✅ Done |

## Upcoming Tasks

| Task ID | Description | Dependency |
| --- | --- | --- |
| `GE-212` | Draft remeshing/parameterisation RFP referencing design constraints. | None |
| `CC-001` (viewer docs) | Document geometry telemetry metrics in the diagnostics viewer. | Requires `GE-220` instrumentation snapshot |

> **Staffing Guidance:** `GE-212` focuses on planning and RFP authorship, while `GE-220` delivered telemetry instrumentation. With metrics now flowing into diagnostics, partner with CC-001 owners on the viewer documentation follow-up noted above.

The published RFP for `GE-212` lives in
[`docs/design/ge-212-remeshing_parameterization_rfp.md`](../../design/ge-212-remeshing_parameterization_rfp.md)
and should be treated as the contract for subsequent implementation milestones.

## Dependencies

- **Testing infrastructure (`TI-002`)** — hosts benchmark harness.
- **Diagnostics initiative (`CC-001`)** — provides telemetry schema (now
  documented in `design/telemetry_schema.md`).

Document findings in the relevant task files (`T-0112`) and update the central
roadmap after each milestone.
