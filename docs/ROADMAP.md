# Central Roadmap

This roadmap aggregates cross-cutting initiatives and module-specific execution
queues. It is the single source of truth for prioritisation; keep it in sync
with [`../README.md`](../README.md), module READMEs, and task files under
[`tasks/`](tasks/).

## Architecture Improvement Plan

### Initiative Summary

| ID | Intent | Dependencies | Status | Owning Groups |
| --- | --- | --- | --- | --- |
| `DC-004` | Standardise error handling on `engine::Result<T, Error>` across modules. | – | ✅ Done | Core, IO |
| `AI-001` | Propagate handle-based lifetime management and validation hooks. | `DC-004` | ✅ Done | Assets, Rendering |
| `AI-002` | Deliver async asset streaming with telemetry and runtime integration. | `AI-001`, `DC-001` | 🔄 In Progress | Assets, Runtime |
| `AI-003` | Extend frame-graph metadata and queue affinity for backend parity. | – | ✅ Done | Rendering, Runtime |
| `RT-002` | Harden physics with persistent manifolds and benchmarking. | – | ✅ Done | Physics |
| `RT-003` | Achieve Vulkan runtime parity and publish backend guidance. | `AI-003` | ✅ Done | Rendering, Runtime |
| `RT-005` | Validate scene hierarchies and expose diagnostics. | – | ✅ Done | Scene, Runtime |
| `RT-006` | Harden IO signature detection with fuzzing + telemetry. | – | 🟠 Blocked on fuzz harness infra | IO |
| `CC-001` | Instrument telemetry and ship a diagnostics viewer. | – | ✅ Done | Core, Tools |
| `CC-002` | Build hot reload infrastructure across caches/backends. | `AI-001` | ✅ Done | Assets, Platform |

### Task Breakdowns

#### `DC-004` — Error Handling Standardisation

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `DC-004.1` | Publish canonical error-handling guide. | `docs/design/error_handling_migration.md` updated with examples and linked from module READMEs. | ✅ Done |
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

- 2025-02-20: Published [`runtime/async_streaming_integration.md`](modules/runtime/async_streaming_integration.md)
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
| `RT-003.2` | Author backend checklist. | Public checklist covering prerequisites, platform dependencies, and validation steps (see [`backend_checklist.md`](modules/rendering/backend_checklist.md)). | ✅ Done |
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
| `RT-006.2` | Integrate libFuzzer harness. | Harness runs in CI with seed corpus, failures captured in telemetry. | 🟠 Blocked on CI runner capacity |
| `RT-006.3` | Author detection docs. | IO README explains detection workflow, failure handling, and fuzzing steps. | ✅ Done |

#### `CC-001` — Telemetry & Diagnostics Viewer

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CC-001.1` | Define metrics schema. | Schema documented, implemented in core telemetry module, adopted by runtime. | ✅ Done |
| `CC-001.2` | Implement viewer shell. | Tools module exposes CLI/UI to inspect metrics with scripted smoke tests. | ✅ Done |
| `CC-001.3` | Publish instrumentation guide. | Cross-module doc outlining how to emit and consume metrics. | ✅ Done |

- 2025-02-28: Published [`design/telemetry_schema.md`](design/telemetry_schema.md)
  and integrated runtime diagnostics with the shared schema.
- 2025-03-20: Added `scripts/diagnostics/telemetry_viewer.py` with smoke tests to
  deliver `CC-001.2`/`TL-101`, enabling operators to inspect runtime telemetry
  snapshots without rebuilding the C++ tooling.
- 2025-03-25: Geometry signature catalogue landed at
  `engine/io/signatures/geometry_signatures.json`; runtime loader honours the
  `ENGINE_IO_GEOMETRY_SIGNATURE_PATH` override for experimentation and fuzzing
  workflows, unblocking the remaining `RT-006` harness work once CI capacity is
  restored.
- 2025-03-22: Published [`design/telemetry_instrumentation_guide.md`](design/telemetry_instrumentation_guide.md)
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

## Outstanding Backlog Focus

Prioritise cross-cutting items first to maintain a stable architectural base.
Once staffed, execute module-specific queues below.

### Module Execution Queues

- **Animation** — `AN-230` GPU/parallel sampling benchmarks once compute queue
  extensions land, followed by `AN-240` state-machine authoring spec work.
- **Assets** — `AS-315` hot reload callback integration (`CC-002`), then
  `AS-320` material persistence planning and `AS-330` diagnostics shell reload
  surfacing.
- **Compute** — `CO-150` cycle detection tooling and `CO-160` CUDA preset
  alignment completed; focus shifts to `CO-170` runtime integration sample
  work.
- **Core** — Completed `CR-125` lifecycle audit, `CR-130` configuration
  refresh, `CR-135` dependency diagnostics, and `CR-136` structured logging.
  With `CR-137` delivered, coordinate with Tools on telemetry viewer smoke-test
  coverage once runtime shared libraries are packaged in CI so initialization
  failure guidance stays validated.
- **Geometry** — `GE-205` normal recompute benchmark landed with reporting;
  `GE-212` remeshing/parameterisation RFP is published (see
  `docs/design/ge-212-remeshing_parameterization_rfp.md`), and `GE-220`
  telemetry alignment instrumentation now feeds diagnostics metrics. Coordinate
  viewer documentation updates so CC-001 consumers can discover the new spatial
  query counters.
- **IO** — `IO-221` signature catalogue landed; fuzz harness wiring tracks
  `RT-006.2` once CI capacity returns. Detection & fuzzing playbook published
  (`RT-006.3`). Structured error catalog (`IO-230`) published; telemetry
  alignment (`IO-240`) remains available for follow-up instrumentation work.
- **Math** — `MA-110` SIMD validation harness (`TI-003`), then `MA-118`
  documentation of solver stability ranges.
- **Physics** — `PH-430` collision throughput telemetry surfaced in diagnostics;
  next sprint should scope automation for publishing long-term trends.
- **Platform** — SDL parity checklist published (`PL-215`); keep presets and the
  new checklist aligned as SDL backend implementation tasks are scoped for
  `DC-003` follow-up work.
- **Rendering** — `RE-520` backend documentation updates building on the
  completed metadata schema (`AI-003`), followed by `RE-530` backend validation
  tooling and parity tracking work.
- **Runtime** — `RT-005` tranche complete; support `SC-225` samples and `SC-230` alerting guidance as scene docs expand.
- **Scene** — `SC-225` diagnostics samples and `SC-230` alerting thresholds extend the hierarchy playbook delivered in `SC-220`.
- **Tools** — TL-110 documentation refresh completed; next focus is surfacing
  metric descriptions in the diagnostics viewer while monitoring the Chrome
  trace export from `TL-115`.

### Process & Audit Items

- Architecture audit checklist – reopen items covering frame-graph determinism,
  handle safety, geometry fidelity, physics invariants, documentation
  completeness, telemetry coverage, and dependency graphs before the next audit
  cycle.
- Milestone hygiene – ensure sprint boards under [`tasks/`](tasks/) reference
  the latest roadmap positions and that status icons in [`../README.md`](../README.md)
  match this document.

## Status Review Cadence

- **Weekly triage** — update task tables above, rotating blockers to the top and
  noting owner changes.
- **Sprint planning** — refresh `Sprint Horizon` in [`../README.md`](../README.md)
  and align module queues with sprint commitments.
- **Post-merge** — whenever an initiative task completes, update the relevant
  table, module README, and task record within the same change.

Maintaining this roadmap keeps the architecture modular, performant, and
predictable for the entire agentic workflow.
