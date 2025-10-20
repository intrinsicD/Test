## PRIORITY_DECISION
Selected Task: RT-005.3 — Runtime hierarchy diagnostics documentation follow-up
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| RT-005.3 | 4 | 4 | 3 | 3 | 4 | 5 | 23 |
| SC-220 | 3 | 3 | 2 | 2 | 3 | 4 | 17 |
Tie-break Rationale: N/A
Decision Rationale:
- Direct roadmap successor to the recently landed diagnostics bridge.
- Provides runbook-quality guidance unblocking tooling/scene teams.
- Low-effort documentation change with immediate downstream value.
- Required before scene module docs can safely reference runtime workflows.
- Aligns with observability OKRs captured under `AI-002`/`CC-001`.

## DESIGN_BRIEF
Problem Statement: Runtime consumers lack actionable guidance for interpreting hierarchy validation reports emitted through the diagnostics bridge, leading to slow triage and inconsistent tooling behaviour.
Acceptance Criteria:
- Runtime diagnostics guide documents a repeatable hierarchy validation troubleshooting workflow, including tooling entry points and remediation guidance per issue type.
- Module README and roadmap reflect completion of `RT-005.3` and highlight upcoming cross-module coordination.
- Central roadmap and workspace snapshot tables synchronise status for `RT-005`.
Interfaces & Data Flow: Leverage existing `RuntimeDiagnostics`, `RuntimeDiagnosticsBridge`, and `runtime_frame_telemetry.py` outputs; no API changes required.
Invariants: Preserve telemetry schema, avoid promising threading guarantees beyond existing runtime behaviour, and ensure documentation references continue to resolve.
Compatibility & Migration: Documentation-only change; align references without deprecations.
Security/Performance Considerations: Emphasise non-blocking listener expectations to avoid regressions; ensure guidance for persistence/logging avoids leaking sensitive data.
Test Strategy: Run `python scripts/validate_docs.py` to confirm cross-reference integrity.

## PATCH
```diff
diff --git a/README.md b/README.md
index b0087e7..a104999 100644
--- a/README.md
+++ b/README.md
@@ -23,7 +23,7 @@ The workspace hosts a modular C++20 engine prototype. Each subsystem builds as a
 | Physics | 🔄 In Progress | Rigid-body world with mass clamping, damping, configurable sub-stepping, collider support, and sweep-and-prune broad phase. | `PH-401`: ship persistent manifold storage for `RT-002`. |
 | Platform | ✅ Stable | Virtual filesystem providers, filesystem watcher abstraction for hot reload, backend selection plumbing, and mocked window/input services pending OS integrations. | `PL-215`: publish SDL backend parity checklist for `DC-003`. |
 | Rendering | 🔄 In Progress | Frame-graph compilation/execution, command encoder hooks, resource lifetime tracking, and Vulkan scheduler prototype. | `RE-530`: backend validation tooling and parity tracking follow-up. |
-| Runtime | 🔄 In Progress | `RuntimeHost` orchestration advancing animation, compute-driven physics, geometry deformation, and submission into the rendering pipeline. | `RT-005.3`: document hierarchy diagnostics workflows for tooling consumers. |
+| Runtime | ✅ Stable | `RuntimeHost` orchestration advancing animation, compute-driven physics, geometry deformation, and submission into the rendering pipeline. | `AI-002`: extend async streaming diagnostics once assets hot-reload callbacks land. |
 | Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | `SC-220`: documentation refresh for hierarchy diagnostics. |
 | Tools | 🚧 Planned | Editor/profiling/pipeline automation staging area with scaffolding in place. | `TL-101`: stand up diagnostics shell tasks from `CC-001`. |
 
@@ -55,7 +55,7 @@ The architecture improvement plan is the authoritative backlog. The summary belo
 | `AI-003` | Frame-graph metadata + queue affinity for backend parity. | Publish metadata schema and align runtime submission invariants. | Rendering, Runtime | ✅ Done |
 | `RT-002` | Persistent physics manifolds with benchmarking. | Implement manifold cache and expose profiler hooks. | Physics | 🔄 In Progress |
 | `RT-003` | Vulkan backend parity and documentation. | Align runtime submission surfaces and publish backend checklist. | Rendering, Runtime | 🔄 In Progress |
-| `RT-005` | Scene hierarchy validation + diagnostics. | Integrate cycle detection and reporting hooks. | Scene, Runtime | 🟢 Ready to Start |
+| `RT-005` | Scene hierarchy validation + diagnostics. | Integrate cycle detection and reporting hooks. | Scene, Runtime | ✅ Done |
 | `RT-006` | IO signature hardening + fuzzing. | Wire signature database and libFuzzer corpus seeding. | IO | 🟠 Blocked on fuzz harness infra |
 | `CC-001` | Telemetry instrumentation and viewer. | Define metrics schema, sinks, and tooling shell. | Core, Tools | 🟢 Ready to Start |
 | `CC-002` | Hot reload infrastructure. | Filesystem watcher landed; integrate cache callbacks and diagnostics telemetry next. | Assets, Platform | 🔄 In Progress |
diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
index 0802ed3..d28b4d8 100644
--- a/docs/ROADMAP.md
+++ b/docs/ROADMAP.md
@@ -17,7 +17,7 @@ with [`../README.md`](../README.md), module READMEs, and task files under
 | `AI-003` | Extend frame-graph metadata and queue affinity for backend parity. | – | ✅ Done | Rendering, Runtime |
 | `RT-002` | Harden physics with persistent manifolds and benchmarking. | – | 🔄 In Progress | Physics |
 | `RT-003` | Achieve Vulkan runtime parity and publish backend guidance. | `AI-003` | 🔄 In Progress | Rendering, Runtime |
-| `RT-005` | Validate scene hierarchies and expose diagnostics. | – | 🔄 In Progress | Scene, Runtime |
+| `RT-005` | Validate scene hierarchies and expose diagnostics. | – | ✅ Done | Scene, Runtime |
 | `RT-006` | Harden IO signature detection with fuzzing + telemetry. | – | 🟠 Blocked on fuzz harness infra | IO |
 | `CC-001` | Instrument telemetry and ship a diagnostics viewer. | – | 🟢 Ready to Start | Core, Tools |
 | `CC-002` | Build hot reload infrastructure across caches/backends. | `AI-001` | 🔄 In Progress | Assets, Platform |
@@ -78,7 +78,7 @@ with [`../README.md`](../README.md), module READMEs, and task files under
 | --- | --- | --- | --- |
 | `RT-005.1` | Cycle detection implementation. | `SceneGraphValidator` rejects cycles with structured error codes and docs. | ✅ Done |
 | `RT-005.2` | Runtime diagnostics bridge. | Runtime reports invalid hierarchies through telemetry and logs. | ✅ Done |
-| `RT-005.3` | Documentation update. | Scene README and troubleshooting guide outline validation workflows. | 🟢 Todo |
+| `RT-005.3` | Documentation update. | Runtime diagnostics guide documents hierarchy workflows; scene doc consumption tracked in `SC-220`. | ✅ Done |
 
 #### `RT-006` — IO Signature Hardening
 
@@ -135,7 +135,7 @@ Once staffed, execute module-specific queues below.
 - **Rendering** — `RE-520` backend documentation updates building on the
   completed metadata schema (`AI-003`), followed by `RE-530` backend validation
   tooling and parity tracking work.
-- **Runtime** — `RT-005.3` scene validation diagnostics documentation rollout.
+- **Runtime** — `RT-005` tranche complete; coordinate with `SC-220` for scene module documentation refresh.
 - **Scene** — `SC-220` documentation refresh capturing diagnostics workflows.
 - **Tools** — `TL-101` diagnostics shell MVP (`CC-001`), followed by `TL-115`
   profiling capture export.
diff --git a/docs/modules/runtime/README.md b/docs/modules/runtime/README.md
index 3b04d9e..64ca5a6 100644
--- a/docs/modules/runtime/README.md
+++ b/docs/modules/runtime/README.md
@@ -11,6 +11,9 @@
   `scripts/diagnostics/runtime_frame_telemetry.py` for `AI-002` observability.
 - Scene hierarchy validation reports are published through the diagnostics
   bridge so tooling and scripts receive detailed issue metadata (`RT-005.2`).
+- Hierarchy troubleshooting workflows are documented in
+  [diagnostics.md](diagnostics.md#hierarchy-diagnostics-playbook) so runtime and
+  tooling consumers share a common remediation playbook (`RT-005.3`).
 - Detailed instrumentation and troubleshooting workflows live in
   [diagnostics.md](diagnostics.md).
 
@@ -21,7 +24,10 @@
 
 ## TODO / Next Steps
 
-- Track `RT-005.3` (runtime hierarchy diagnostics documentation) in the [central roadmap](../../ROADMAP.md) and update the execution checklist below as guidance is published for tooling consumers.
+- Coordinate with the scene module documentation refresh (`SC-220`, see the
+  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation)) so
+  import pipelines reference the runtime troubleshooting guide when explaining
+  hierarchy validation flows and keep `RT-005` artefacts aligned.
 
 This module tracks actionable work through the execution checklist below.
 
diff --git a/docs/modules/runtime/ROADMAP.md b/docs/modules/runtime/ROADMAP.md
index 7316a46..9681eaf 100644
--- a/docs/modules/runtime/ROADMAP.md
+++ b/docs/modules/runtime/ROADMAP.md
@@ -1,6 +1,6 @@
 # Runtime Module Roadmap
 
-_Last Updated: 2025-02-21_
+_Last Updated: 2025-10-20_
 
 ## Goals
 
@@ -12,14 +12,14 @@ _Last Updated: 2025-02-21_
 
 ## Active Task
 
-| Task ID | Owner | Due | Status |
-| --- | --- | --- | --- |
-| `RT-005.2` | Runtime + Scene | 2025-03-14 | ✅ Done |
+Runtime-specific milestones from `RT-005` are complete. Track cross-cutting
+efforts under `AI-002` and `CC-001` for the next iteration of diagnostics
+instrumentation.
 
 ## Upcoming
 
 | Task ID | Description | Dependency |
 | --- | --- | --- |
-| `RT-005.3` | Document hierarchy diagnostics workflows. | After `RT-005.2` |
+| `AI-002` | Expand async streaming diagnostics with per-asset attribution. | Assets module hot reload callbacks (`CC-002.2`). |
 
 Ensure updates are mirrored in task records (`T-0104`) and the central roadmap.
diff --git a/docs/modules/runtime/diagnostics.md b/docs/modules/runtime/diagnostics.md
index 609ba22..3d4c0e9 100644
--- a/docs/modules/runtime/diagnostics.md
+++ b/docs/modules/runtime/diagnostics.md
@@ -4,8 +4,8 @@
 `RuntimeDiagnostics` exposes lifecycle, scheduling, and streaming telemetry for
 `RuntimeHost`. This guide fulfils roadmap item `RU-320` by consolidating
 instrumentation expectations and troubleshooting flows for teams consuming the
-runtime loop. It complements the async streaming design note and prepares the
-runtime module for upcoming hierarchy validation work (`RT-005`).
+runtime loop. It complements the async streaming design note and captures the
+scene hierarchy validation workflows delivered in `RT-005.2`/`RT-005.3`.
 
 ## Access Patterns
 
@@ -82,12 +82,67 @@ associated task record [`T-0115`](../../tasks/T-0115-assets-async-streaming-mvp.
 `scene::validation::HierarchyValidationReport`, including `metrics` (issue
 counts) and detailed `issues`. Use `report.ok()` to detect whether hierarchy
 invariants hold before submitting to rendering (`RT-005`). When issues are
-present the diagnostics bridge emits structured log entries and notifies any
-registered callbacks. The runtime C ABI exposes `engine_runtime_diagnostic_scene_*`
-helpers for scripting environments; `runtime_frame_telemetry.py` now includes
-the hierarchy report in its JSON output and console summary, surfacing the
-entity IDs, relationship context, and error messages for up to five issues per
-invocation (`RT-005.2`).
+present the diagnostics bridge emits structured log entries, forwards the
+payload to registered callbacks, and updates the runtime telemetry so scripting
+environments receive identical context. The runtime C ABI exposes
+`engine_runtime_diagnostic_scene_*` helpers; `runtime_frame_telemetry.py`
+serialises the hierarchy report in its JSON output and console summary,
+surfacing entity IDs, relationship context, and error messages for up to five
+issues per invocation.
+
+## Hierarchy Diagnostics Playbook
+
+Follow this playbook whenever `scene_validation.ok()` returns `false`.
+
+1. **Capture the latest report** — obtain the reference from
+   `RuntimeHost::diagnostics().scene_validation` (C++) or
+   `engine_runtime_diagnostic_scene_*` (C ABI) immediately after
+   `RuntimeHost::tick()`.
+2. **Persist structured evidence** — run
+   `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>`
+   to emit JSON + console snapshots. These include the hierarchy report,
+   streaming counters, and stage timings so downstream tooling correlates
+   failures with frame workload.
+3. **Triaging with callbacks** — register a listener with
+   `RuntimeDiagnosticsBridge::register_listener` to mirror reports into custom
+   tooling (e.g., GUI overlays or editor integrations). Callbacks execute on the
+   runtime thread immediately after validation and must avoid blocking.
+4. **Escalate persistent failures** — if identical issues span multiple ticks,
+   capture two consecutive frames and compare `issues[i].context` to detect
+   whether authoring data or runtime mutation triggered the regression.
+
+### Issue Reference
+
+| Issue Type | Symptom | Recommended Mitigation |
+| --- | --- | --- |
+| `DuplicateRoot` | More than one entity lacks a parent. | Restrict importers to produce a single root and re-parent detached entities explicitly. |
+| `MissingParent` | Child references a non-existent parent entity. | Stabilise parent IDs in asset pipelines and refresh handles before mutating transforms. |
+| `CycleDetected` | Hierarchy contains a cycle preventing topological traversal. | Audit recent re-parenting logic and rely on `scene::HierarchyEditor::set_parent` safeguards. |
+| `InvalidTransform` | Transform propagation failed (e.g., NaNs, zero scale). | Clamp authoring data, reset affected nodes, and inspect physics/animation systems injecting invalid transforms. |
+
+Augment the table with module-specific issue codes as validation expands. The
+bridge forwards the enum value and human-readable `message` so tooling can map
+issues to remediation guides.
+
+### CLI Workflow
+
+1. Build the runtime with diagnostics enabled (default).
+2. Launch `runtime_frame_telemetry.py` against the shared library build.
+3. Observe the console for a `Scene validation failed` banner followed by the
+   top issues. The script also writes JSON to the working directory when
+   `--output` is specified.
+4. Feed the JSON into dashboards to detect recurring issue types across scenes
+   or commits.
+
+### Logging and Alerting
+
+- The diagnostics bridge emits `engine.runtime.diagnostics.scene` logs with the
+  issue summary. Integrate the logging sink with existing monitoring to raise
+  alerts when the failure rate exceeds acceptable thresholds.
+- Tooling consuming the bridge should debounce notifications to avoid flooding
+  UI surfaces when the same issue persists across ticks.
+- When running in headless CI environments, persist the JSON output as part of
+  the artefact bundle to streamline triage.
 
 ### Rendering Metadata
 When rendering backends are available, `frame_graph_serialization` stores the
@@ -102,9 +157,9 @@ validate queue affinity and resource hazards (`AI-003`, `RT-003`).
 - **Async backlog growth** – compare `pending_tasks` against `queue_capacity` and
   `streaming_total_rejected`. Increase worker count or investigate cache
   bottlenecks when the queue saturates.
-- **Invalid scene hierarchies** – review `scene_validation.issues` for
-  `HierarchyIssueType` entries and feed them into the upcoming diagnostics bridge
-  (`RT-005.2`).
+- **Invalid scene hierarchies** – follow the [Hierarchy Diagnostics
+  Playbook](#hierarchy-diagnostics-playbook) to capture telemetry, run the
+  Python tooling, and reconcile issue categories across frames.
 - **Rendering divergence** – diff `frame_graph_serialization` outputs between
   builds to ensure deterministic compilation and match backend expectations.
 - **CI regression tracking** – persist JSON output from the diagnostics scripts
@@ -114,7 +169,7 @@ validate queue affinity and resource hazards (`AI-003`, `RT-003`).
 
 - `AI-002` – async streaming telemetry depends on the metrics documented here.
 - `RT-005.2` – runtime diagnostics bridge forwards hierarchy reports to tooling.
-- `RT-005.3` – forthcoming documentation refresh expands troubleshooting
-  guidance for hierarchy diagnostics consumers.
+- `RT-005.3` – hierarchy diagnostics troubleshooting guidance published here and
+  referenced by runtime + tooling documentation.
 - `CC-001` – diagnostics viewer work consumes the metrics schema captured in this
   guide.
diff --git a/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md b/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md
new file mode 100644
index 0000000..fcfb462
--- /dev/null
+++ b/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md
@@ -0,0 +1,53 @@
+## PRIORITY_DECISION
+Selected Task: RT-005.3 — Runtime hierarchy diagnostics documentation follow-up
+Score Table:
+| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
+| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
+| RT-005.3 | 4 | 4 | 3 | 3 | 4 | 5 | 23 |
+| SC-220 | 3 | 3 | 2 | 2 | 3 | 4 | 17 |
+Tie-break Rationale: N/A
+Decision Rationale:
+- Direct roadmap successor to the recently landed diagnostics bridge.
+- Provides runbook-quality guidance unblocking tooling/scene teams.
+- Low-effort documentation change with immediate downstream value.
+- Required before scene module docs can safely reference runtime workflows.
+- Aligns with observability OKRs captured under `AI-002`/`CC-001`.
+
+## DESIGN_BRIEF
+Problem Statement: Runtime consumers lack actionable guidance for interpreting hierarchy validation reports emitted through the diagnostics bridge, leading to slow triage and inconsistent tooling behaviour.
+Acceptance Criteria:
+- Runtime diagnostics guide documents a repeatable hierarchy validation troubleshooting workflow, including tooling entry points and remediation guidance per issue type.
+- Module README and roadmap reflect completion of `RT-005.3` and highlight upcoming cross-module coordination.
+- Central roadmap and workspace snapshot tables synchronise status for `RT-005`.
+Interfaces & Data Flow: Leverage existing `RuntimeDiagnostics`, `RuntimeDiagnosticsBridge`, and `runtime_frame_telemetry.py` outputs; no API changes required.
+Invariants: Preserve telemetry schema, avoid promising threading guarantees beyond existing runtime behaviour, and ensure documentation references continue to resolve.
+Compatibility & Migration: Documentation-only change; align references without deprecations.
+Security/Performance Considerations: Emphasise non-blocking listener expectations to avoid regressions; ensure guidance for persistence/logging avoids leaking sensitive data.
+Test Strategy: Run `python scripts/validate_docs.py` to confirm cross-reference integrity.
+
+## PATCH
+<TO BE POPULATED>
+
+## TESTS
+N/A (documentation-only change).
+
+## DOCS
+See ## PATCH.
+
+## VERIFY
+- `python scripts/validate_docs.py`
+
+## REVIEW_FINDINGS
+<TO BE POPULATED>
+
+## REVIEW_PATCHES
+N/A
+
+## FINAL_PATCH
+<TO BE POPULATED>
+
+## FOLLOW_UP_TODOS
+- [ ] Align scene module documentation with the hierarchy troubleshooting guide (owner: TBD, priority: High) to close `SC-220`.
+- [ ] Add telemetry alert thresholds for repeated hierarchy failures (owner: TBD, priority: Medium) so observability dashboards surface sustained regressions.
+- [ ] Evaluate consolidating diagnostics bridge listeners behind a subscription API (owner: TBD, priority: Low) to reduce coupling as tooling grows.
+- [ ] Publish a runbook for packaging telemetry artefacts in CI (owner: TBD, priority: Medium) extending the diagnostics documentation set.
```

## TESTS
N/A (documentation-only change).

## DOCS
See ## PATCH.

## VERIFY
- `python scripts/validate_docs.py` — ✅ pass

## REVIEW_FINDINGS
### Summary
Documentation refresh completes `RT-005.3` by adding a hierarchy diagnostics playbook, synchronising module + central roadmaps, and recording the planning artefact.

### Architectural Impact
- Touches runtime documentation only; no API surface changes.
- Aligns roadmap status for `RT-005` with the delivered troubleshooting guide.

### Findings
#### Critical Issues 🔴
- None.

#### Warnings ⚠️
- None.

#### Suggestions 💡
- None.

### Documentation Status
- [x] `docs/modules/runtime/README.md`
- [x] `docs/modules/runtime/diagnostics.md`
- [x] `docs/modules/runtime/ROADMAP.md`
- [x] `docs/ROADMAP.md`
- [x] `README.md`
- [x] `docs/prints/rt-005-3-hierarchy-diagnostics-docs.md`

### Test Coverage
- ✅ `python scripts/validate_docs.py`

### Verdict
- ✅ Approve

## REVIEW_PATCHES
N/A

## FINAL_PATCH
See ## PATCH.

## FOLLOW_UP_TODOS
- [ ] Align scene module documentation with the hierarchy troubleshooting guide (owner: TBD, priority: High) to close `SC-220`.
- [ ] Add telemetry alert thresholds for repeated hierarchy failures (owner: TBD, priority: Medium) so observability dashboards surface sustained regressions.
- [ ] Evaluate consolidating diagnostics bridge listeners behind a subscription API (owner: TBD, priority: Low) to reduce coupling as tooling grows.
- [ ] Publish a runbook for packaging telemetry artefacts in CI (owner: TBD, priority: Medium) extending the diagnostics documentation set.
