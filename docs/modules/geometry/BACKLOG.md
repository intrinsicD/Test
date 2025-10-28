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
- 2025-11-12 — Enabled feature-preserving remeshing with crease-aware edge
  protection and tangential smoothing, extending `GE-221+` toward the adaptive
  follow-up milestones.
- 2025-11-15 — Introduced adaptive remeshing driven by surface/normal budgets,
  deriving edge targets when only error tolerances are provided to progress
  `GE-221+` Phase 2.
- 2025-11-21 — Added `geometry_frustum_culling` benchmark measuring 41 ns/test in
  Release (200k AABBs × 256 iterations) to close out `GE-230` performance
  validation while tracking a 203 ns/test Debug baseline for regression
  monitoring.
- 2025-11-24 — `SurfaceMesh` retains per-vertex texture coordinates and halfedge
  conversions round-trip them, establishing the data plumbing required for
  `GE-221+` parameterisation work.
- 2025-11-26 — Remeshing UV reuse now interpolates/smooths coordinates, scales to
  requested texel density, and reports parameterisation metrics to advance
  `GE-221+` telemetry objectives.
- 2025-11-27 — ABF++ parameterisation (`ParameterizationMode::kGenerateAbfpp`) now
  solves constrained angle optimisation and reconstructs UVs from the derived
  edge-length system, fulfilling the conformal generation milestone for `GE-221+`.
- 2025-11-30 — Added remeshing telemetry aggregation (`RemeshTelemetry`) tracking
  per-mode invocation counts, iteration totals, split/collapse rates, and
  duration/job labels to close the observability gap for `GE-221+` diagnostics.
- 2025-12-01 — `geometry_remesh` CLI ships for offline remeshing and UV
- 2025-12-04 — Remeshing attribute transfer now interpolates rest-space positions during splits, averages them during
  collapses, and resynchronises Laplacian relaxation updates so animation bindings retain deterministic offsets after topology
  changes, progressing GE-221+ execution.
  parameterisation, printing telemetry-aligned summaries for `GE-221+` runs.
- 2025-12-17 — Remesh statistics/telemetry now capture triangle counts and normalised quality scores, feeding AI-004 dataset
  validation and extending `GE-221+` observability.
- 2025-12-19 — Frustum triangle clipping landed in `shape_interactions` with symmetric overloads/tests so portal visibility (T-0122)
  can classify geometry without approximating triangles as spheres/boxes.
- 2025-12-20 — Frustum clipping now returns intervals for lines, rays, and segments with symmetric tests, covering picking rays
  and portal edges for T-0122 visibility workloads.
- 2025-12-22 — Capsule intersections gained line, ray, and segment overloads with symmetric interval tests so picking and portal
  classifiers can use the shared primitive tracked by T-0129.
- 2025-12-23 — Introduced the `Capsule` shape primitive with analytic helpers and random sampling so the new intersection paths
  and documentation examples compile against a dedicated geometry type instead of relying on forward declarations.
