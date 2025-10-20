# Central Roadmap

This roadmap aggregates cross-cutting initiatives and module-specific execution
queues. It is the single source of truth for prioritisation; keep it in sync
with [`../README.md`](../README.md), module READMEs, and task files under
[`tasks/`](tasks/).

## Architecture Improvement Plan

### Initiative Summary

| ID | Intent | Dependencies | Status | Owning Groups |
| --- | --- | --- | --- | --- |
| `DC-004` | Standardise error handling on `engine::Result<T, Error>` across modules. | – | 🔄 In Progress | Core, IO |
| `AI-001` | Propagate handle-based lifetime management and validation hooks. | `DC-004` | 🔄 In Progress | Assets, Rendering |
| `AI-002` | Deliver async asset streaming with telemetry and runtime integration. | `AI-001`, `DC-001` | 🔄 In Progress | Assets, Runtime |
| `AI-003` | Extend frame-graph metadata and queue affinity for backend parity. | – | ✅ Done | Rendering, Runtime |
| `RT-002` | Harden physics with persistent manifolds and benchmarking. | – | 🔄 In Progress | Physics |
| `RT-003` | Achieve Vulkan runtime parity and publish backend guidance. | `AI-003` | 🔄 In Progress | Rendering, Runtime |
| `RT-005` | Validate scene hierarchies and expose diagnostics. | – | ✅ Done | Scene, Runtime |
| `RT-006` | Harden IO signature detection with fuzzing + telemetry. | – | 🟠 Blocked on fuzz harness infra | IO |
| `CC-001` | Instrument telemetry and ship a diagnostics viewer. | – | 🟢 Ready to Start | Core, Tools |
| `CC-002` | Build hot reload infrastructure across caches/backends. | `AI-001` | 🔄 In Progress | Assets, Platform |

### Task Breakdowns

#### `DC-004` — Error Handling Standardisation

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `DC-004.1` | Publish canonical error-handling guide. | `docs/design/error_handling_migration.md` updated with examples and linked from module READMEs. | ✅ Done |
| `DC-004.2` | Migrate IO module APIs to `Result<T>`. | All IO entry points return `Result<T>`, tests cover error paths, and module README updated. | 🔄 In Progress |
| `DC-004.3` | Add lint/check tooling. | Static check preventing legacy error patterns integrated into CI. | ✅ Done |

- 2025-02-25: Animation clip importer/exporter now return
  `AnimationIoResult<T>` values, extending the `DC-004.2` migration beyond the
  geometry pipelines.

#### `AI-001` — Resource Lifetime Management

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AI-001.1` | Document ownership patterns. | `docs/design/resource_management.md` extended with cache + rendering examples. | ✅ Done |
| `AI-001.2` | Enable debug validation hooks. | Debug builds assert on stale handles across assets/rendering; telemetry logs emitted. | 🔄 In Progress |
| `AI-001.3` | Update module READMEs. | Assets and rendering READMEs include handle lifecycle guidance. | ✅ Done |

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
| `RT-002.1` | Implement manifold cache. | Collider pairs persist contacts across frames with debug visualisation. | 🔄 In Progress |
| `RT-002.2` | Integrate telemetry hooks. | Physics telemetry exposes manifold churn metrics consumed by diagnostics shell. | 🟢 Todo |
| `RT-002.3` | Benchmark harness. | Automated benchmark records collision throughput and is tracked in CI. | 🟢 Todo |

#### `RT-003` — Vulkan Runtime Parity

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RT-003.1` | Align runtime submission API. | Runtime + Vulkan backend share a unified submission struct with documentation. | 🔄 In Progress |
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
| `RT-006.1` | Build signature database. | Curated signature set committed with provenance notes; fuzz harness consumes it. | 🔄 In Progress |
| `RT-006.2` | Integrate libFuzzer harness. | Harness runs in CI with seed corpus, failures captured in telemetry. | 🟠 Blocked on CI runner capacity |
| `RT-006.3` | Author detection docs. | IO README explains detection workflow, failure handling, and fuzzing steps. | 🟢 Todo |

#### `CC-001` — Telemetry & Diagnostics Viewer

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CC-001.1` | Define metrics schema. | Schema documented, implemented in core telemetry module, adopted by runtime. | 🟢 Todo |
| `CC-001.2` | Implement viewer shell. | Tools module exposes CLI/UI to inspect metrics with scripted smoke tests. | 🟢 Todo |
| `CC-001.3` | Publish instrumentation guide. | Cross-module doc outlining how to emit and consume metrics. | 🟢 Todo |

#### `CC-002` — Hot Reload Infrastructure

| Task ID | Description | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CC-002.1` | Filesystem watcher abstraction. | Platform module exposes cross-platform watcher with tests and documentation. | ✅ Done |
| `CC-002.2` | Cache callback integration. | Assets caches react to change notifications with error telemetry. | 🟢 Todo |
| `CC-002.3` | Failure diagnostics. | Diagnostics shell surfaces reload failures with actionable hints. | 🟢 Todo |

## Outstanding Backlog Focus

Prioritise cross-cutting items first to maintain a stable architectural base.
Once staffed, execute module-specific queues below.

### Module Execution Queues

- **Animation** — `AN-230` GPU/parallel sampling benchmarks once compute queue
  extensions land, followed by `AN-240` state-machine authoring spec work.
- **Assets** — `AS-315` hot reload callback integration (`CC-002`), then
  `AS-320` material persistence planning.
- **Compute** — `CO-150` cycle detection tooling is next after landing the
  dispatcher extension documentation (`CO-141`), followed by `CO-160` CUDA
  preset alignment.
- **Core** — `CR-118` diagnostics bridge specification (`CC-001`), followed by
  `CR-125` plugin lifecycle audit to keep `DC-001` fresh.
- **Geometry** — `GE-205` accelerated normals benchmark for `TI-002`, then
  `GE-212` remeshing RFP draft.
- **IO** — `IO-221` signature + fuzz integration (`RT-006`) in progress;
  structured error catalog (`IO-230`) published; next focus `IO-240`
  telemetry alignment once diagnostics schema lands.
- **Math** — `MA-110` SIMD validation harness (`TI-003`), then `MA-118`
  documentation of solver stability ranges.
- **Physics** — `PH-401` manifold cache (`RT-002`), followed by `PH-410`
  benchmarking harness.
- **Platform** — `PL-215` SDL parity checklist (`DC-003`); filesystem watcher
  abstraction (`PL-222`) is complete, so focus shifts to `PL-230` backend
  selection documentation.
- **Rendering** — `RE-520` backend documentation updates building on the
  completed metadata schema (`AI-003`), followed by `RE-530` backend validation
  tooling and parity tracking work.
- **Runtime** — `RT-005` tranche complete; coordinate with `SC-220` for scene module documentation refresh.
- **Scene** — `SC-220` documentation refresh capturing diagnostics workflows.
- **Tools** — `TL-101` diagnostics shell MVP (`CC-001`), followed by `TL-115`
  profiling capture export.

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
