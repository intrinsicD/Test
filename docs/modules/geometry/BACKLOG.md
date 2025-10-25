# Geometry Module Roadmap

_Last Updated: 2025-05-06_

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

| Task ID | Description | Dependency | Priority |
| --- | --- | --- | --- |
| `GE-221+` | Scope execution milestones derived from the remeshing/parameterisation RFP. | `GE-212` | TBD |
| `GE-230` | Frustum utilities for visibility culling (T-0128). | None | High |
| `GE-231` | Complete shape intersection test coverage (T-0129). | None | Medium |
>
> **New (2025-10-23):** Tasks `GE-230` (frustum utilities) and `GE-231` (intersection coverage) added to support rendering visibility system. `GE-230` is high priority as it blocks the rendering visibility culling system (T-0122).

> **Staffing Guidance:** `GE-212` focused on planning and RFP authorship, while `GE-220` delivered telemetry instrumentation. With viewer documentation published, direct capacity toward sequencing the remeshing execution milestones captured under `GE-221+`.

The published RFP for `GE-212` lives in
[`docs/design/GE-212-REMESHING_PARAMETERIZATION_RFP.md`](../../design/GE-212-REMESHING_PARAMETERIZATION_RFP.md)
and should be treated as the contract for subsequent implementation milestones.

## Dependencies

- **Testing infrastructure (`TI-002`)** — hosts benchmark harness.
- **Diagnostics initiative (`CC-001`)** — provides telemetry schema (now
  documented in `design/TELEMETRY_SCHEMA.md`).

Document findings in the relevant task files (`T-0112`) and update the central
roadmap after each milestone.

## Maintenance Log

- 2025-05-09 — Defaulted `PointCloudIOFlags::Format::kAuto` to PLY when file
  extensions are missing or unrecognised, ensuring round-trip tooling for
  `T-0112` handles temporary exports without manual flag overrides.
- 2025-11-02 — Added surface topology summary utilities (`AnalyzeSurfaceTopology`)
  to classify boundary, crease, and non-manifold features ahead of the
  `GE-221+` remeshing milestones.
- 2025-11-03 — Introduced remeshing request/validation scaffolding aligning with
  `GE-212` to unblock `GE-221+` execution work.
- 2025-11-04 — Added remeshing edge statistics and target resolution utilities to
  feed GE-221+ execution planning and telemetry baselines.
- 2025-11-05 — Implemented uniform remeshing kernel executing split/collapse loops
  with Laplacian relaxation, producing `RemeshOutput` for GE-221+ Phase 1.
