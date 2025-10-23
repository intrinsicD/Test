# Central Roadmap

<!-- Anchor to support references from prompts and docs -->
<a id="architecture-improvement-plan"></a>

This roadmap aggregates cross-cutting initiatives and module-specific execution
queues. It is the single source of truth for prioritisation; keep it in sync
with [`../README.md`](../README.md), module READMEs, and task files under
[`tasks/`](tasks/).

---

## 🎯 Active Work (Q4 2025)

| ID | Intent | Dependencies | Next Milestone | Owning Groups |
| --- | --- | --- | --- | --- |
| `AI-002` | Deliver async asset streaming with telemetry and runtime integration. | `AI-001`, `DC-001` | Cancellation hardening (`AI-002.2`) | Assets, Runtime |
| `RT-006` | Harden IO signature detection with fuzzing + telemetry. | – | CI integration (blocked on infra) | IO |

### Active Task Details

#### `AI-002` — Async Asset Streaming (🔄 In Progress)

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AI-002.1` | Instrument async queue telemetry. | Runtime exposes queue metrics, `scripts/diagnostics/streaming_report.py` documents usage. | ✅ Done |
| `AI-002.2` | Harden cancellation + failure flows. | Futures support cancellation with integration tests covering failure propagation. | 🔄 In Progress |
| `AI-002.3` | Publish runtime integration guide. | Runtime README explains streaming lifecycle and telemetry expectations. | ✅ Done |

**Recent Updates:**
- 2025-02-20: Published `runtime/ASYNC_STREAMING_INTEGRATION.md` detailing configuration and telemetry consumption (`AI-002.3`).

<!-- Anchor for RT-002 task references -->
<a id="rt-002-physics-contact-manifolds"></a>
#### `RT-006` — IO Signature Hardening (🟠 Blocked)

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RT-006.1` | Build signature database. | Curated signature set committed with provenance notes; fuzz harness consumes it. | ✅ Done |
| `RT-006.2` | Integrate libFuzzer harness. | Harness built with curated corpus; CI automation tracked separately. | ✅ Done |
| `RT-006.3` | Author detection docs. | IO README explains detection workflow, failure handling, and fuzzing steps. | ✅ Done |

**Blocker:** CI fuzzing infrastructure provisioning pending.

---

## 📎 Backlog (Prioritized)

### Immediate Next (Ready for Sprint Planning)

- **AN-230** — GPU/parallel sampling benchmarks (blocked on `CO-170` compute queue extensions)
- **AS-315** — Integrate filesystem watcher callbacks for hot reload telemetry
- **GE-221+** — Remeshing execution milestones (depends on published `GE-212` RFP)
- **DC-003** — SDL backend implementation (see `platform/SDL_BACKEND_CHECKLIST.md`)

### Mid-term (3-6 months)

- **PY-001** — Core bindings and `.pyi` stubs for Python integration
- **TL-120** — Advanced diagnostics dashboard with Chrome trace export
- **CO-170** — Runtime integration sample showing dispatcher orchestration

### Long-term / Research

- **AN-240** — Advanced state machine authoring (see `specs/AN-240-state-machine-authoring.md`)
- **Plugin hot-reload** — Architecture for dynamic plugin loading/unloading
- **Distributed rendering** — Multi-GPU frame graph scheduling

---

## ✅ Recently Completed (Archive after 30 days)

| ID | Intent | Completed | Key Deliverables |
| --- | --- | --- | --- |
| `DC-004` | Standardise error handling on `engine::Result<T, Error>` | 2025-03-17 | `design/ERROR_HANDLING_MIGRATION.md`, IO migration |
| `AI-001` | Handle-based lifetime management and validation hooks | 2025-04-30 | Debug validation, telemetry counters |
| `AI-003` | Frame-graph metadata and queue affinity for backend parity | 2025-03 | Metadata schema, Vulkan integration |
| `RT-002` | Physics persistent manifolds and benchmarking | 2025-03-15 | Manifold cache, collision benchmark harness |
| `RT-003` | Vulkan runtime parity and backend guidance | 2025-03 | Backend checklist, integration regression |
| `RT-005` | Scene hierarchy validation and diagnostics | 2025-03 | Cycle detection, runtime diagnostics bridge |
| `CC-001` | Telemetry instrumentation and diagnostics viewer | 2025-03-25 | Telemetry schema, viewer CLI, instrumentation guide |
| `CC-002` | Hot reload infrastructure | 2025-03-24 | Filesystem watcher, cache callbacks, diagnostics |
| `AS-330` | Asset hot-reload diagnostics integration | 2025-05-20 | Telemetry viewer surfaces recent reload failures with per-asset hints |

<details>
<summary><b>Completed Initiative Details (Click to expand)</b></summary>

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
**Last updated:** 2025-10-22 (Restructured for improved active work visibility)
