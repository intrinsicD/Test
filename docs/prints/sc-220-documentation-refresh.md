## PRIORITY_DECISION
Selected Task: SC-220 — Scene hierarchy diagnostics documentation refresh
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| SC-220 | 4 | 4 | 3 | 3 | 5 | 4 | 23 |
| AS-320 | 3 | 4 | 4 | 3 | 2 | 4 | 20 |
| CO-150 | 3 | 3 | 3 | 3 | 3 | 3 | 18 |
Tie-break Rationale: N/A
Decision Rationale:
- High-priority documentation follow-up flagged by `RT-005.3` with direct downstream consumers.
- Minimal-effort change unlocks accurate guidance for runtime + tooling teams.
- Addresses stale roadmap and README status that currently contradict delivered runtime docs.
- Keeps repository snapshot consistent across root README, roadmap, and module guidance.
- Aligns with observability OKRs by distributing the hierarchy troubleshooting workflow beyond runtime docs.

## DESIGN_BRIEF
Problem Statement: Scene module documentation lags behind the runtime diagnostics updates from `RT-005.3`, leaving scene authors without local guidance and keeping roadmap snapshots out of sync.
Acceptance Criteria:
- Scene README documents the hierarchy diagnostics workflow and links to the runtime playbook.
- A dedicated scene diagnostics guide provides remediation steps tailored to scene authors.
- Scene roadmap and execution checklist mark `SC-220` complete with refreshed "last updated" metadata.
- Root README and central roadmap no longer list `SC-220` as the next scene deliverable and point to the upcoming focus.
- `docs/prints/rt-005-3-hierarchy-diagnostics-docs.md` follow-up checkbox for SC-220 marked complete.
- Documentation validation script passes.
Interfaces & Data Flow: Documentation-only change; references hook into existing runtime diagnostics outputs (`scene_validation` report, telemetry scripts) without altering APIs.
Invariants: Preserve canonical telemetry/schema references; ensure links resolve; maintain TODO section contract enforced by `scripts/validate_docs.py`.
Compatibility & Migration: No API changes—documentation ensures existing workflows are discoverable; note future follow-ups instead of introducing new IDs.
Security/Performance Considerations: Highlight non-blocking callback guidance to prevent regressions; avoid promising unsupported telemetry channels.
Test Strategy: Run `python scripts/validate_docs.py` after edits; manual link verification while authoring.

## PATCH
```diff
diff --git a/README.md b/README.md
index e85b1d4..86f8c9b 100644
--- a/README.md
+++ b/README.md
@@ -24,7 +24,7 @@ The workspace hosts a modular C++20 engine prototype. Each subsystem builds as a
 | Platform | ✅ Stable | Virtual filesystem providers, filesystem watcher abstraction for hot reload, backend selection plumbing, and mocked window/input services pending OS integrations. | `PL-215`: publish SDL backend parity checklist for `DC-003`. |
 | Rendering | 🔄 In Progress | Frame-graph compilation/execution, command encoder hooks, resource lifetime tracking, and Vulkan scheduler prototype. | `RE-530`: backend validation tooling and parity tracking follow-up. |
 | Runtime | ✅ Stable | `RuntimeHost` orchestration advancing animation, compute-driven physics, CPU linear blend skinning, geometry deformation, and submission into the rendering pipeline. | `AI-002`: extend async streaming diagnostics once assets hot-reload callbacks land. |
-| Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | `SC-220`: documentation refresh for hierarchy diagnostics. |
+| Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | `SC-225`: publish hierarchy diagnostics samples and alerting guidance follow-up. |
 | Tools | 🔄 In Progress | Editor/profiling/pipeline automation staging area with the telemetry viewer CLI surfacing runtime snapshots. | `TL-110`: document tooling invocation and troubleshooting. |

diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
index 7955d9a..7d046c5 100644
--- a/docs/ROADMAP.md
+++ b/docs/ROADMAP.md
@@ -165,8 +165,8 @@ Once staffed, execute module-specific queues below.
 - **Rendering** — `RE-520` backend documentation updates building on the completed metadata schema (`AI-003`), followed by `RE-530` backend validation tooling and parity tracking work.
-- **Runtime** — `RT-005` tranche complete; coordinate with `SC-220` for scene module documentation refresh.
-- **Scene** — `SC-220` documentation refresh capturing diagnostics workflows.
+- **Runtime** — `RT-005` tranche complete; support `SC-225` samples and `SC-230` alerting guidance as scene docs expand.
+- **Scene** — `SC-225` diagnostics samples and `SC-230` alerting thresholds extend the hierarchy playbook delivered in `SC-220`.
 - **Tools** — `TL-110` tooling documentation refresh (`CC-001`) and `TL-115` profiling capture export now that the telemetry viewer CLI (`TL-101`) is available.

diff --git a/docs/modules/runtime/README.md b/docs/modules/runtime/README.md
index 2d82fff..13dd3e7 100644
--- a/docs/modules/runtime/README.md
+++ b/docs/modules/runtime/README.md
@@ -48,10 +48,11 @@
 ## TODO / Next Steps
 
-- Coordinate with the scene module documentation refresh (`SC-220`, see the [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation)) so import pipelines reference the runtime troubleshooting guide when explaining hierarchy validation flows and keep `RT-005` artefacts aligned.
+- Support the scene module follow-ups (`SC-225` diagnostics samples, `SC-230` alert thresholds) by providing telemetry examples and cross-linking new fixtures once they land in the shared tooling docs; these items extend `RT-005` and remain tracked in the [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
 
diff --git a/docs/modules/runtime/diagnostics.md b/docs/modules/runtime/diagnostics.md
index 3adeae1..1815acd 100644
--- a/docs/modules/runtime/diagnostics.md
+++ b/docs/modules/runtime/diagnostics.md
@@ -99,6 +99,10 @@ serialises the hierarchy report in its JSON output and console summary,
 surfacing entity IDs, relationship context, and error messages for up to five issues per invocation.
 
+Scene authors should review the [scene diagnostics guide](../scene/diagnostics.md) for module-specific remediation steps and planned follow-up samples introduced by `SC-220`.
+
diff --git a/docs/modules/scene/README.md b/docs/modules/scene/README.md
index e25a795..dfed379 100644
--- a/docs/modules/scene/README.md
+++ b/docs/modules/scene/README.md
@@ -13,11 +13,40 @@
 - Include `<engine/scene/graph/scene_graph_validator.hpp>` for cycle detection checks.
 - Run `ctest --preset <preset> --tests-regex engine_scene`.
 
+## Diagnostics & Validation Workflow
+- Use `scene::validation::validate_hierarchy` (mirrored through `RuntimeHost::diagnostics().scene_validation`) to audit imported or procedurally-generated hierarchies before submitting to rendering or saving authored scenes. The runtime diagnostics guide documents the full [Hierarchy Diagnostics Playbook](../runtime/diagnostics.md#hierarchy-diagnostics-playbook) with CLI tooling and remediation strategies shared across modules.
+- Capture structured telemetry by running `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>` against a runtime-enabled build. The JSON output includes the latest hierarchy validation report so scene tooling can correlate issue types with the authoring context that produced them.
+- Register listeners with `runtime::RuntimeDiagnosticsBridge::register_listener` (see the runtime diagnostics guide) when editor integrations require live feedback. Callbacks execute on the runtime thread—log or enqueue work instead of blocking to preserve determinism.
+- Record remediation steps for common issue codes (`DuplicateRoot`, `MissingParent`, `CycleDetected`, `InvalidTransform`) in scene tooling so authors can resolve problems without cross-referencing runtime internals.
+- Consult [diagnostics.md](diagnostics.md) for the complete scene-focused remediation guide and telemetry alignment notes introduced by `SC-220`.
 
 ## TODO / Next Steps
 
-- Track `SC-220` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — captures ongoing diagnostics documentation work aligned with `RT-005`.
+- Prepare runnable scene import samples that exercise the hierarchy diagnostics workflow end-to-end and link them from this README once the fixtures land (`SC-225`, planned); track progress in the `RT-005` tranche via the [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
+- Coordinate with runtime/tooling owners to define alert thresholds for recurring hierarchy failures so dashboards surface sustained regressions (`SC-230`, planned); status captured under `RT-005` in the [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
@@ -27,6 +56,6 @@ This module tracks actionable work through the execution checklist below.
 | --- | --- | --- | --- |
 | `SC-208` | Implement cycle detection validation (`RT-005`). | Scene validator detects cycles with structured errors and tests. | ✅ Done |
 | `SC-215` | Runtime diagnostics bridge. | Runtime reports invalid hierarchies via telemetry/logs. | ✅ Done |
-| `SC-220` | Documentation refresh. | Update README + troubleshooting guide with validation workflows. | 🟢 Todo |
+| `SC-220` | Documentation refresh. | Update README + troubleshooting guide with validation workflows. | ✅ Done |
 
diff --git a/docs/modules/scene/ROADMAP.md b/docs/modules/scene/ROADMAP.md
index d7cac4a..310e5e3 100644
--- a/docs/modules/scene/ROADMAP.md
+++ b/docs/modules/scene/ROADMAP.md
@@ -1,6 +1,6 @@
 # Scene Module Roadmap
 
-_Last Updated: 2025-02-19_
+_Last Updated: 2025-03-22_
@@
 | `SC-208` | Implement cycle detection and validation errors. | ✅ Done |
 | `SC-215` | Integrate runtime diagnostics bridge. | ✅ Done |
-| `SC-220` | Update documentation for validation workflows. | 🟢 Planned |
+| `SC-220` | Update documentation for validation workflows. | ✅ Done |
+| `SC-225` | Author hierarchy diagnostics samples. | 🟢 Planned |
+| `SC-230` | Define alert thresholds for hierarchy regressions. | 🟢 Planned |
@@
-| Sprint 2 | `SC-220` | Ensure telemetry + docs launched together. |
+| Sprint 2 | `SC-220` | Ensure telemetry + docs launched together. |
+| Sprint 3 | `SC-225`, `SC-230` | Land samples + alerting guidance alongside tooling updates. |

diff --git a/docs/modules/scene/diagnostics.md b/docs/modules/scene/diagnostics.md
new file mode 100644
index 0000000..7997ce9
--- /dev/null
+++ b/docs/modules/scene/diagnostics.md
@@ -0,0 +1,96 @@
+# Scene Diagnostics Guide
+
+## Purpose
+`SC-220` closes the documentation gap between the scene module and the runtime hierarchy diagnostics playbook published during `RT-005.3`. This guide summarises how scene authors and tooling integrations consume the validation workflow, where telemetry lives, and how to respond when issues surface.
+
+Use this document alongside the runtime diagnostics guide ([`docs/modules/runtime/diagnostics.md`](../runtime/diagnostics.md)) to keep troubleshooting material synchronised across modules.
+
+## Entry Points
+
+### Standalone Validation
+- Call `scene::validation::validate_hierarchy(SceneGraphView)` when operating purely within scene tooling or import pipelines. The helper returns a `HierarchyValidationReport` that mirrors the payload exposed through `RuntimeHost::diagnostics().scene_validation`.
+- Log or persist `report.issues` (vector of `HierarchyValidationIssue`) for downstream tooling. Each issue encodes an enum `code`, a human-readable `message`, and optional `context` describing the offending entity path.
+
+### Runtime-Driven Validation
+- Runtime integrations receive the same report via `RuntimeHost::diagnostics().scene_validation`. Access the payload after `RuntimeHost::tick()` to ensure the latest hierarchy snapshot is available before rendering or serialising state.
+- Register callbacks with `runtime::RuntimeDiagnosticsBridge::register_listener` when scene tooling needs push-based notifications. Callbacks run on the runtime thread immediately after validation; forward summaries to asynchronous channels to avoid blocking the frame loop.
+
+### Tooling Scripts
+- Execute `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>` against a runtime-enabled build to capture JSON output containing the hierarchy report, stage timings, and streaming metrics. Use `--output <path>` to persist artefacts in CI.
+- When operating outside the runtime binary, invoke custom tooling that wraps `scene::validation::validate_hierarchy` and emits JSON in the same schema to keep dashboards consistent.
+
+## Remediation Workflow
+
+1. **Detect failure** — check `HierarchyValidationReport::ok()` after validation. Early exit if no issues are present.
+2. **Categorise** — inspect each issue's `code` and map it to the table below. Consider logging the parent/child identifiers stored in `context` for reproducibility.
+3. **Respond** — apply the recommended mitigation, re-run validation, and persist the before/after telemetry when a regression is detected.
+4. **Escalate** — if the same issue reappears across multiple frames or tool sessions, capture consecutive reports and share them with the owning team. Include the runtime telemetry JSON where available.
+
+### Issue Reference
+
+| Code | Symptom | Mitigation |
+| --- | --- | --- |
+| `DuplicateRoot` | More than one entity lacks a parent/root entry. | Adjust import pipelines to enforce a single root; attach detached nodes explicitly. |
+| `MissingParent` | A child references a missing parent handle. | Rebuild parent indices and refresh handles before mutating transforms. |
+| `CycleDetected` | Hierarchy traversal detects a cycle. | Use `scene::HierarchyEditor::set_parent` to re-parent safely and audit editor undo stacks. |
+| `InvalidTransform` | Propagated transforms contain NaNs or degenerate scales. | Clamp authoring data, reset invalid nodes, and inspect physics/animation contributors. |
+
+Augment this table when new `HierarchyValidationIssueCode` values land. Reference `docs/modules/runtime/diagnostics.md` for additional instrumentation context.
+
+## Telemetry & Alerting
+
+- Runtime telemetry exposes metrics under `runtime.scene_validation.*` with counts and last-seen issue codes; scene tooling should subscribe to the same schema to maintain parity.
+- Coordinate with the runtime team on `SC-230` to define alert thresholds for repeated hierarchy failures. Capture decisions in this guide when the alerting policy stabilises.
+- Persist telemetry artefacts in CI by storing the JSON output from the diagnostics scripts. Use consistent filenames (e.g., `scene_validation_<timestamp>.json`) to simplify trend analysis.
+
+## Sample Library (Planned)
+
+`SC-225` will introduce runnable samples that exercise the validation workflow end-to-end. Once merged, link the fixtures here with setup instructions and expected telemetry snapshots.
+
+## Related References
+
+- [`docs/modules/runtime/diagnostics.md`](../runtime/diagnostics.md)
+- [`docs/prints/sc-220-documentation-refresh.md`](../../prints/sc-220-documentation-refresh.md)
+- [`docs/prints/rt-005-3-hierarchy-diagnostics-docs.md`](../../prints/rt-005-3-hierarchy-diagnostics-docs.md)
+- [`docs/design/telemetry_schema.md`](../../design/telemetry_schema.md)
+- [`docs/tasks/T-0104-runtime-frame-graph-integration.md`](../../tasks/T-0104-runtime-frame-graph-integration.md)

diff --git a/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md b/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md
index d954c5d..024ff64 100644
--- a/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md
+++ b/docs/prints/rt-005-3-hierarchy-diagnostics-docs.md
@@ -358,7 +358,7 @@ N/A
 ## FOLLOW_UP_TODOS
-- [ ] Align scene module documentation with the hierarchy troubleshooting guide (owner: TBD, priority: High) to close `SC-220`.
+- [x] Align scene module documentation with the hierarchy troubleshooting guide (owner: Scene, priority: High) to close `SC-220` (completed 2025-03-22; see `docs/prints/sc-220-documentation-refresh.md`).

diff --git a/docs/reviews/2025-03-22-scene-docs.md b/docs/reviews/2025-03-22-scene-docs.md
new file mode 100644
index 0000000..8ae26af
--- /dev/null
+++ b/docs/reviews/2025-03-22-scene-docs.md
@@ -0,0 +1,47 @@
+# Review: Scene hierarchy diagnostics documentation refresh (2025-03-22)
+
+Following the checklist in [`docs/prompts/review-checklist.md`](../prompts/review-checklist.md).
+
+## Summary
+The change completes `SC-220` by adding a dedicated scene diagnostics guide, refreshing module and central roadmaps, and aligning the workspace snapshot with the delivered hierarchy troubleshooting workflow.
+
+## Architectural Impact
+- Documentation-only update scoped to runtime/scene modules and high-level roadmap tables; no code or build assets modified.
+- Keeps `RT-005` documentation consistent across runtime and scene modules while introducing new follow-up tasks (`SC-225`, `SC-230`).
+
+## Findings
+
+### Critical Issues 🔴
+None.
+
+### Warnings ⚠️
+None.
+
+### Suggestions 💡
+1. Consider adding concrete JSON telemetry snippets to the planned samples once `SC-225` lands so tooling teams can diff expected outputs.
+   - Follow-up task: include under `SC-225` when authoring samples.
+
+## Documentation Status
+- [x] `docs/modules/scene/README.md`
+- [x] `docs/modules/scene/ROADMAP.md`
+- [x] `docs/modules/scene/diagnostics.md`
+- [x] `docs/modules/runtime/README.md`
+- [x] `docs/modules/runtime/diagnostics.md`
+- [x] `docs/ROADMAP.md`
+- [x] `README.md`
+- [x] `docs/prints/rt-005-3-hierarchy-diagnostics-docs.md`
+- [x] `docs/prints/sc-220-documentation-refresh.md`
+
+## Test Coverage
+- ✅ `python scripts/validate_docs.py`
+
+## Follow-Up Work
+- [ ] Ensure `SC-225` samples include telemetry snapshots as suggested above.
+
+## Verdict
+- [x] ✅ Approve
```

## TESTS
N/A (documentation-only change).

## DOCS
See ## PATCH.

## VERIFY
- `python scripts/validate_docs.py` — ✅ pass

## REVIEW_FINDINGS
- Independent review recorded in [`docs/reviews/2025-03-22-scene-docs.md`](../reviews/2025-03-22-scene-docs.md) approved the change with no blocking issues and one follow-up suggestion for `SC-225` samples.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
See ## PATCH (no additional edits after review).

## FOLLOW_UP_TODOS
- [ ] Scope scene import pipeline samples that exercise hierarchy diagnostics end-to-end (owner: Scene team, priority: Medium) to reinforce documentation with runnable artefacts.
- [ ] Define telemetry alert thresholds for recurring hierarchy failures (owner: Runtime + Tooling, priority: Medium) so dashboards surface sustained regressions.
- [ ] Evaluate consolidating diagnostics bridge listeners behind a subscription API (owner: Runtime, priority: Low) to reduce coupling as tooling grows.
- [ ] Publish CI artefact packaging guide for hierarchy diagnostics outputs (owner: DevEx, priority: Medium) once tooling workflow stabilises.
