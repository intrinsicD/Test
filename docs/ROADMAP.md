# Central Roadmap

<!-- Anchor to support references from prompts and docs -->
<a id="architecture-improvement-plan"></a>

This roadmap aggregates cross-cutting initiatives and module-specific execution
queues. It is the single source of truth for prioritisation; keep it in sync
with [`../README.md`](../README.md), module READMEs, and task files under
[`tasks/`](tasks/).

---

## 🎯 Active Work (Q1 2026)

| ID | Intent | Dependencies | Next Milestone | Owning Groups | Owner |
| --- | --- | --- | --- | --- | --- |
| `AI-004` | Application prototyping enablement: rendering baseline, runtime harness, tooling sandbox, datasets, comparative benchmarks. | `AI-001`, `AI-002`, `AI-003` | Kickoff review + baseline plan sign-off (target 2025-11-15) | Rendering, Runtime, Tools, Assets, Performance | @pm-agent |
| `RT-006` | Harden IO signature detection with fuzzing + telemetry. | – | CI integration (blocked on infra) | IO | @io-lead |

### Active Task Details

<!-- Anchor for RT-002 task references -->
<a id="rt-002-physics-contact-manifolds"></a>
#### `AI-004` — Application Prototyping Enablement (🚀 Active)

| Task ID | Description | Exit Criteria | Status | Owner |
| --- | --- | --- | --- | --- |
| `RE-610` | Ship research-grade rendering baseline with deferred/forward presets, debug overlays, and telemetry integration. | Rendering preset renders reference scenes at ≥120 FPS, exposes debug views, and exports telemetry counters documented in module README. | 🟡 Planning | @rendering-lead |
| `RT-320` | Deliver runtime prototyping harness with interactive + headless modes, configuration schema, and scripting hooks. | Harness loads dataset manifest, toggles algorithm variants, and exports benchmark artefacts validated by integration tests. | 🟡 Planning | @runtime-lead |
| `TL-210` | Build experiment sandbox UI with dataset/pipeline selection, telemetry charts, and benchmark controls. | Sandbox enumerates manifests, edits parameters live, and captures benchmark runs with persisted layouts/screenshots. | 🟡 Planning | @tools-lead |
| `AS-330` | Curate reference dataset packages with manifests, ingestion scripts, and provenance/licensing notes. | Datasets download via scripted workflow, register with runtime caches, and surface metadata in harness/UI. | 🟡 Planning | @assets-lead |
| `CC-310` | Automate comparative benchmarks across engine and reference implementations with CI smoke coverage. | Benchmark orchestrator emits comparison reports, CI gate enforces thresholds, and telemetry viewer renders comparative plots. | 🟡 Planning | @perf-lead |
| `DC-040` | Align AI-004 configuration schema across rendering, runtime, tools, assets, and benchmarking. | Shared schema ADR approved, validators integrated behind feature flag, and migration notes published for downstream teams. | 🟡 Planning | @architect-lead |

**Key Dates:**
- 2025-10-20: Initiative charter approved; cross-module leads assigned.
- 2025-11-15: Kickoff review to confirm shared configuration schema and dataset shortlist.
- 2025-12-20: Runtime harness + rendering baseline integration demo with sandbox UI prototype.
- 2026-01-15: Benchmark automation smoke suite active in CI.

**Risks:**
- Coordinated delivery requires shared configuration schema—Owner: @pm-agent (Product Manager), Due: 2025-11-20.
- Dataset licensing review could delay publication—Owner: @assets-lead (Assets), Due: 2025-11-18.
- Benchmarking hardware pool request pending infrastructure approval—Owner: @perf-lead (Performance), Due: 2025-11-22.

**Mitigations:**
- Bi-weekly integration demos to surface drift early.
- Dedicated benchmarking hardware pool request submitted to infrastructure (awaiting approval).

**Success Metrics:**
- Rendering baseline achieves ≥120 FPS on reference scenes (4K triangles, PBR materials)
- Runtime harness processes ≥5 datasets with <2% configuration overhead
- Benchmark variance ≤2% across repeated runs
- Sandbox UI supports ≥10 concurrent telemetry charts without frame drops
- Dataset ingestion completes in ≤30 seconds for standard packages
- CI smoke tests execute full AI-004 workflow in ≤5 minutes

**Execution Phases:**
- **Phase 1 (Nov 2025):** Configuration schema (`DC-040`) + rendering baseline (`RE-610`) + runtime harness core (`RT-320`)
- **Phase 2 (Dec 2025):** Dataset integration (`AS-330`) + sandbox UI prototype (`TL-210`)
- **Phase 3 (Jan 2026):** Comparative benchmarking (`CC-310`) + CI automation

**Recent Progress:**
- 2025-10-24: Drafted `ADR-0007` to define the shared AI-004 configuration schema and published coordination task `DC-040`.
- 2025-10-23: Added comparative benchmark orchestrator (`scripts/benchmarks/run_comparative_benchmarks.py`) executing
  CC-310 scenarios from declarative configurations and enforcing regression thresholds.
- 2025-10-22: Captured how `GE-221+` remeshing outputs populate AI-004 datasets in the geometry module README note, ensuring prototyping harness reuse.
- 2025-10-26: `geometry_remesh` CLI emits AI-004-compatible `datasets` manifest snippets so remeshing jobs register directly with
  the shared configuration schema during prototyping runs.
- 2025-12-06: Introduced `rendering::configure_research_baseline` to scaffold the RE-610 preset with forward/deferred toggles
  and overlay targets, unblocking runtime and tools integration work.
- 2025-12-07: Research baseline telemetry publishes shading mode selections and per-pass draw/timing metrics to the runtime
  diagnostics bridge, satisfying RE-610 instrumentation requirements.

---

## 📎 Backlog (Prioritized)

### Immediate Next (Ready for Sprint Planning)

- **RT-006** — IO signature hardening remains blocked on CI infrastructure provisioning. Monitor infra updates and reprioritise when capacity restored.
- **GE-221+** — Remeshing execution milestones (depends on published `GE-212` RFP)
  - 2025-11-05: Uniform remeshing baseline (split/collapse + relaxation) available via
    `Remesh`, returning `RemeshOutput` statistics for downstream tooling.
  - 2025-11-03: Remesh request/validation scaffolding merged to capture
    configuration semantics ahead of kernel implementation.
  - 2025-11-02: Surface topology summary utilities landed (`AnalyzeSurfaceTopology`) to
    provide deterministic boundary and crease classification for Phase 0 planning.
  - 2025-11-12: Feature-preserving remeshing adds crease-aware edge protection and
    tangential smoothing; next milestone targets adaptive error budgeting.
  - 2025-11-26: UV reuse in remeshing now interpolates/smooths coordinates, scales to
    target texel density budgets, and records parameterisation summaries for
    telemetry tracking.
- 2025-11-27: ABF++ parameterisation pipeline lands, generating conformal UV charts
  from constrained angle optimisation and circle-intersection reconstruction to
  progress the `GE-221+` execution milestone.
- 2025-11-30: Remesh telemetry instrumentation captures per-mode invocation counts,
  iteration totals, and split/collapse statistics with job labels to close the
  observability gap for `GE-221+` planning.
- 2025-12-01: `geometry_remesh` CLI delivers offline remeshing execution with
  telemetry-aligned summaries, enabling scripted `GE-221+` workflows without
  embedding the geometry module.
- 2025-12-04: Remeshing preserves rest-space offsets by interpolating and averaging `SurfaceMesh::rest_positions` and resynchronises Laplacian relaxation so animation bindings remain stable as GE-221+ progresses.
- **DC-003** — SDL backend parity (deferred now that GLFW satisfies headless automation)

#### `DC-003` — SDL Backend Implementation (⏸ Deferred)

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `DC-003.1` | Stand up native SDL window lifecycle and deterministic event pumping. | SDL backend creates native windows, translates input/events, and demotes to the mock backend when SDL is unavailable; unit smoke tests green. | ⏸ Deferred |
| `DC-003.2` | Integrate swapchain surface export and backend selection fallbacks. | Vulkan/OpenGL surfaces exposed through `SwapchainSurface`, build presets updated, runtime override respects fallback order. | ⏸ Deferred |
| `DC-003.3` | Expand validation, CI, and telemetry coverage. | Platform + runtime integration tests execute with SDL enabled in CI, diagnostics capture SDL errors, README/checklist updated. | ⏸ Deferred |

**Recent Updates:**
- 2025-10-28: Moved `DC-003` to the backlog after GLFW gained headless automation support; SDL parity tasks remain staged for later execution.
- 2025-10-27: Task card [`DC-003.3`](tasks/DC-003.3-sdl-ci-telemetry.md) published to enable SDL CI coverage and telemetry instrumentation, closing the remaining roadmap gap.
- 2025-10-26: Task card [`DC-003.2`](tasks/DC-003.2-sdl-swapchain-surface-export.md) created and prioritised to follow `DC-003.1`, covering SDL swapchain surface export and fallback alignment.
- 2025-10-25: Task card [`DC-003.1`](tasks/DC-003.1-sdl-window-lifecycle.md) published to deliver native SDL window lifecycle and deterministic event pumping.
- 2025-10-24: `PL-215` published the SDL parity checklist; implementation work now tracked under `DC-003`.

### Mid-term (3-6 months)

- **PY-001** — Core bindings and `.pyi` stubs for Python integration *(loader
  module ships manual stub; extend coverage to compiled bindings next)*
- **TL-120** — Advanced diagnostics dashboard with Chrome trace export
### Long-term / Research

| ID | Intent | Preliminary Timeline | Research Phase |
| --- | --- | --- | --- |
| `AN-240` | Advanced state machine authoring (see `specs/AN-240-state-machine-authoring.md`) | Q3 2026 | Spike & prototyping |
| `PL-300` | Plugin hot-reload — Architecture for dynamic plugin loading/unloading | Q2-Q3 2026 | Design exploration |
| `RE-700` | Distributed rendering — Multi-GPU frame graph scheduling | Q4 2026+ | Research & feasibility |
| `GE-400` | GPU-accelerated mesh processing pipeline | Q3 2026 | Performance analysis |
| `PH-300` | Advanced constraint solver with GPU acceleration | Q4 2026 | Algorithm research |

**Notes:**
- All research items require design review and feasibility assessment before transitioning to active work
- Timeline estimates subject to change based on foundational work completion (AI-004, etc.)
- Research spikes should produce ADRs or design documents before implementation begins

---

## ✅ Recently Completed (Archive after 30 days)

| ID | Intent | Completed | Key Deliverables |
| --- | --- | --- | --- |
| `CO-170` | Runtime dispatcher integration sample + telemetry workflow | 2025-11-11 | `engine_compute_runtime_sample`, diagnostics reports, integration playbook |
| `DC-005` | Headless automation support for GLFW | 2025-10-28 | Hidden window creation for headless configs, updated capability matrix, roadmap/docs refresh |
| `DC-004` | Standardise error handling on `engine::Result<T, Error>` | 2025-03-17 | `design/ERROR_HANDLING_MIGRATION.md`, IO migration |
| `AI-001` | Handle-based lifetime management and validation hooks | 2025-04-30 | Debug validation, telemetry counters |
| `AI-002` | Async asset streaming with telemetry and runtime integration | 2025-10-24 | IO thread pool, runtime futures, streaming diagnostics |
| `AI-003` | Frame-graph metadata and queue affinity for backend parity | 2025-03 | Metadata schema, Vulkan integration |
| `RT-002` | Physics persistent manifolds and benchmarking | 2025-03-15 | Manifold cache, collision benchmark harness |
| `RT-003` | Vulkan runtime parity and backend guidance | 2025-03 | Backend checklist, integration regression |
| `RT-005` | Scene hierarchy validation and diagnostics | 2025-03 | Cycle detection, runtime diagnostics bridge |
| `CC-001` | Telemetry instrumentation and diagnostics viewer | 2025-03-25 | Telemetry schema, viewer CLI, instrumentation guide |
| `CC-002` | Hot reload infrastructure | 2025-03-24 | Filesystem watcher, cache callbacks, diagnostics |
| `AS-315` | Integrate filesystem watcher callbacks for hot reload telemetry | 2025-05-24 | Watcher-driven reloads feed runtime telemetry and diagnostics report |
| `AS-330` | Asset hot-reload diagnostics integration | 2025-05-20 | Telemetry viewer surfaces recent reload failures with per-asset hints |
| `TL-120` | Diagnostics dashboard with Chrome trace export for asset telemetry | 2025-10-24 | Streaming report text dashboard, Chrome trace counters, documentation refresh |
| `AN-230` | GPU parallel sampling benchmarks | 2025-10-19 | Animation sampling analysis workflow, comparative benchmarking reference implementation |

<details>
<summary><b>Completed Initiative Details (Click to expand)</b></summary>

#### `CO-170` — Runtime Integration Sample

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CO-170.1` | Author runtime/compute integration sample harness. | Sample executable (`engine_compute_runtime_sample`) builds on all presets, drives dispatcher queues, and emits telemetry snapshots. | ✅ Done |
| `CO-170.2` | Publish telemetry + analysis workflow. | `scripts/diagnostics/compute_dispatch_report.py` ingests sample output, dashboards updated, docs refreshed. | ✅ Done |
| `CO-170.3` | Document integration playbook and update module READMEs. | `docs/design/CO-170-runtime-integration-playbook.md` merged; compute/runtime READMEs include usage guide. | ✅ Done |

- 2025-10-30: Initial `engine_compute_runtime_sample` harness and
  `compute_dispatch_report.py` landed to capture dispatcher telemetry and
  support jitter analysis for `CO-170.1`.
- 2025-10-31: Workload profiles and queue instrumentation added to the runtime
  sample; telemetry now records per-queue aggregates and the analysis script
  surfaces queue utilisation alongside jitter warnings (`CO-170.1`, `CO-170.2`).
- 2025-11-01: Queue naming and per-category overrides added to the runtime
  sample; telemetry exports `queue_assignments` so diagnostics reflect queue
  affinity decisions (`CO-170.2`).
- 2025-11-03: Queue attribution switched to deterministic FNV-1a hashing to
  keep telemetry stable across toolchains and CI environments (`CO-170.2`).
- 2025-11-04: `--baseline` instrumentation captures single-queue reference runs
  and reports speed-up deltas, enabling automated enforcement of the 1.5×
  performance target in diagnostics (`CO-170.2`).
- 2025-11-05: Telemetry export now embeds GPU staging estimates and warns when
  the runtime sample exceeds the 256 MiB animation budget, satisfying the
  memory acceptance criteria for `CO-170`.
- 2025-11-06: Runtime sample exposes `--jitter-budget-ms` and diagnostics warn
  when frame dispatch jitter exceeds the 0.5 ms budget for both optimised and
  baseline captures, meeting the latency acceptance criteria.
- 2025-11-07: Shared workload configuration header and unit tests validate mesh
  subdivisions and physics body counts across sample profiles,
  strengthening dispatcher workload adapters (`CO-170.1`).
- 2025-11-08: Documentation refreshed to highlight dispatcher backend selection
  metadata, jitter/memory budgets, and diagnostics workflow for the runtime
  sample (`CO-170.2`, `CO-170.3`).
- 2025-11-09: Runtime sample gains `--repeat`/`--output-dir`, emits
  per-capture metadata (`run_index`, `run_count`), and the diagnostics report
  surfaces active run context to simplify benchmark variance tracking.
- 2025-11-10: Diagnostics script adds `--exit-on-warning` so CI automation can
  fail when the runtime capture reports jitter, memory, or performance
  regressions (`CO-170.2`).
- 2025-11-11: Added `compute_dispatch_benchmark.py` to automate ≤2% variance
  enforcement and surface jitter/speed-up regressions in CI (`CO-170.2`).

#### `DC-004` — Error Handling Standardisation

### Task Breakdowns

#### `DC-004` — Error Handling Standardisation

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `DC-004.1` | Publish canonical error-handling guide. | `docs/design/ERROR_HANDLING_MIGRATION.md` updated with examples and linked from module READMEs. | ✅ Done |
| `DC-004.2` | Migrate IO module APIs to `Result<T>`. | All IO entry points return `Result<T>`, tests cover error paths, and module README updated. | ✅ Done |
| `DC-004.3` | Add lint/check tooling. | Static check preventing legacy error patterns integrated into CI. | ✅ Done |

- 2025-02-25: Animation clip importer/exporter now return
  `AnimationIoResult<T>` values, extending the `DC-004.2` migration beyond the
  geometry pipelines.
- 2025-03-17: Geometry import/export registry wraps plugin operations in
  `GeometryIoResult` conversions, eliminating legacy exception paths and
  surfacing structured `GeometryIoErrorCode` values across IO tests.

#### `AI-001` — Resource Lifetime Management

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AI-001.1` | Document ownership patterns. | `docs/design/resource_management.md` extended with cache + rendering examples. | ✅ Done |
| `AI-001.2` | Enable debug validation hooks. | Debug builds assert on stale handles across assets/rendering; telemetry logs emitted. | ✅ Done |
| `AI-001.3` | Update module READMEs. | Assets and rendering READMEs include handle lifecycle guidance. | ✅ Done |

- 2025-04-30: Handle validation registry now guards asset and rendering usage, logging telemetry and exposing
  `runtime.handles.*` counters in diagnostics to complete `AI-001.2`.

#### `AI-002` — Async Asset Streaming

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AI-002.1` | Instrument async queue telemetry. | Runtime exposes queue metrics, `scripts/diagnostics/streaming_report.py` documents usage. | ✅ Done |
| `AI-002.2` | Harden cancellation + failure flows. | Futures support cancellation with integration tests covering failure propagation. | ✅ Done |
| `AI-002.3` | Publish runtime integration guide. | Runtime README explains streaming lifecycle and telemetry expectations. | ✅ Done |

- 2025-02-20: Published [`runtime/ASYNC_STREAMING_INTEGRATION.md`](modules/runtime/ASYNC_STREAMING_INTEGRATION.md)
  detailing configuration, request workflows, and telemetry consumption for the
  runtime streaming path (`AI-002.3`).
- 2025-10-22: `RuntimeHost::request_mesh_asset` / `request_point_cloud_asset` now
  schedule asynchronous loads through configured caches, completing the runtime
  integration workstream for `AI-002.2`.
- 2025-10-23: OBJ hot-reload validation rejects geometry-free updates so
  `AssetHotReloadTelemetry` records failures accurately, finalising
  cancellation/failure hardening for `AI-002.2`.

#### `AI-003` — Frame-Graph Metadata

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AI-003.1` | Finalise resource descriptor schema. | Schema reviewed with runtime team; encoded in rendering headers with migration guide. | ✅ Done |
| `AI-003.2` | Implement queue affinity validation. | Frame-graph compilation rejects invalid queue transitions with regression coverage. | ✅ Done |
| `AI-003.3` | Sync runtime submission hooks. | Runtime submission path aligned with new metadata, integration tests green. | ✅ Done |

#### `RT-002` — Persistent Physics Manifolds

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RT-002.1` | Implement manifold cache. | Collider pairs persist contacts across frames with debug visualisation. | ✅ Done |
| `RT-002.2` | Integrate telemetry hooks. | Physics telemetry exposes manifold churn metrics consumed by diagnostics shell. | ✅ Done |
| `RT-002.3` | Benchmark harness. | Automated benchmark records collision throughput and is tracked in CI. | ✅ Done |

- 2025-03-10: Runtime diagnostics now expose physics manifold telemetry via
  `runtime.physics.*` metrics, completing `RT-002.2` and aligning with the
  shared telemetry schema.
- 2025-03-15: Added `engine_physics_benchmarks` to capture collision throughput
  baselines, closing `RT-002.3` and seeding CI artefacts for physics trend
  analysis.

#### `RT-003` — Vulkan Runtime Parity

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RT-003.1` | Align runtime submission API. | Runtime + Vulkan backend share a unified submission struct with documentation. | ✅ Done |
| `RT-003.2` | Author backend checklist. | Public checklist covering prerequisites, platform dependencies, and validation steps (see [`BACKEND_CHECKLIST.md`](modules/rendering/BACKEND_CHECKLIST.md)). | ✅ Done |
| `RT-003.3` | Integration regression. | Cross-module test ensures runtime submits deterministic workloads to Vulkan path. | ✅ Done |

#### `RT-005` — Scene Hierarchy Validation

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RT-005.1` | Cycle detection implementation. | `SceneGraphValidator` rejects cycles with structured error codes and docs. | ✅ Done |
| `RT-005.2` | Runtime diagnostics bridge. | Runtime reports invalid hierarchies through telemetry and logs. | ✅ Done |
| `RT-005.3` | Documentation update. | Runtime diagnostics guide documents hierarchy workflows; scene doc consumption tracked in `SC-220`. | ✅ Done |

#### `RT-006` — IO Signature Hardening

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RT-006.1` | Build signature database. | Curated signature set committed with provenance notes; fuzz harness consumes it. | ✅ Done |
| `RT-006.2` | Integrate libFuzzer harness. | Harness built with curated corpus; CI automation tracked separately. | ✅ Done |
| `RT-006.3` | Author detection docs. | IO README explains detection workflow, failure handling, and fuzzing steps. | ✅ Done |

#### `CC-001` — Telemetry & Diagnostics Viewer

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CC-001.1` | Define metrics schema. | Schema documented, implemented in core telemetry module, adopted by runtime. | ✅ Done |
| `CC-001.2` | Implement viewer shell. | Tools module exposes CLI/UI to inspect metrics with scripted smoke tests. | ✅ Done |
| `CC-001.3` | Publish instrumentation guide. | Cross-module doc outlining how to emit and consume metrics. | ✅ Done |

- 2025-02-28: Published [`design/TELEMETRY_SCHEMA.md`](design/TELEMETRY_SCHEMA.md)
  and integrated runtime diagnostics with the shared schema.
- 2025-03-20: Added `scripts/diagnostics/telemetry_viewer.py` with smoke tests to
  deliver `CC-001.2`/`TL-101`, enabling operators to inspect runtime telemetry
  snapshots without rebuilding the C++ tooling.
- 2025-03-25: Geometry signature catalogue landed at
  `engine/io/signatures/geometry_signatures.json`; runtime loader honours the
  `ENGINE_IO_GEOMETRY_SIGNATURE_PATH` override for experimentation and fuzzing
  workflows, unblocking the remaining `RT-006` harness work once CI capacity is
  restored.
- 2025-03-22: Published [`design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](design/TELEMETRY_INSTRUMENTATION_GUIDE.md)
  detailing module authoring patterns and closing `CC-001.3`.

#### `CC-002` — Hot Reload Infrastructure

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CC-002.1` | Filesystem watcher abstraction. | Platform module exposes cross-platform watcher with tests and documentation. | ✅ Done |
| `CC-002.2` | Cache callback integration. | Assets caches react to change notifications with error telemetry. | ✅ Done |
| `CC-002.3` | Failure diagnostics. | Diagnostics shell surfaces reload failures with actionable hints. | ✅ Done |

- 2025-03-04: Assets mesh/graph/point cloud/shader/texture caches subscribe to
  the platform filesystem watcher and unregister on unload, completing
  `CC-002.2` while preserving pending callback hand-offs.
- 2025-03-24: Runtime diagnostics bridge exports hot reload telemetry snapshots
  and the diagnostics viewer surfaces failure guidance, completing
  `CC-002.3` and closing the hot reload infrastructure initiative.


</details>

---

## 🔁 Process & Maintenance

### Weekly Triage
- Update active work table with progress notes
- Rotate blockers to top with escalation paths
- Note owner changes and dependency updates

### Sprint Planning
- Refresh sprint horizon in `../README.md`
- Align module queues with sprint commitments
- Create task records in `tasks/` for planned work

### Post-Merge
- Update relevant tables (Active/Completed/Backlog)
- Update module README and roadmap within same commit
- Archive completed initiatives after 30 days

### Monthly Archival
- Move initiatives completed >30 days ago to `archive/tasks/done/`
- Update module summaries to reflect current priorities
- Run `scripts/validate_docs.py` to verify link integrity

---

<!-- Anchor for TI-001 integration suites references -->
<a id="ti-001-integration-suites"></a>
**Last updated:** 2025-10-26 (Fixed date inconsistencies, added success metrics and phasing to AI-004, expanded research section)
