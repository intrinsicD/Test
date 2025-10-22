## PRIORITY_DECISION
Selected Task: SC-225 — hierarchy diagnostics samples
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| SC-225 hierarchy diagnostics samples | 4 | 4 | 3 | 3 | 4 | 5 | 23 |
| AS-320 material persistence planning | 2 | 3 | 2 | 2 | 2 | 3 | 14 |
| MA-110 SIMD validation harness | 1 | 2 | 2 | 2 | 3 | 2 | 12 |
Decision Rationale:
- SC-225 is the only open roadmap item for the scene module and blocks documentation accuracy promised during `RT-005` follow-ups.
- Shipping runnable fixtures unlocks downstream observability work in runtime/docs that currently reference a “planned” sample.
- Implementation cost is limited to wiring build targets and documentation updates—no deep refactors required.
- Leaving the TODO unresolved keeps roadmap/status tables inconsistent across README, module docs, and review notes.
- Aligns directly with `RT-005` initiative goals and prior review feedback demanding telemetry snapshots.

## DESIGN_BRIEF
Problem Statement: Scene and runtime documentation reference hierarchy diagnostics samples that do not yet exist as build targets, leaving tooling teams without runnable fixtures and causing roadmap drift. Acceptance Criteria: (1) Provide a compiled CLI that exercises hierarchy validation using deterministic fixtures; (2) Publish instructions and JSON output samples in the scene diagnostics guide; (3) Update module, roadmap, and review follow-ups to mark `SC-225` complete; (4) Keep runtime TODO guidance aligned with the new sample. Interfaces & Data Flow: CMake adds `scene_hierarchy_diagnostics_sample`, linking against `engine_scene` and placing the binary under `engine/scene/`. The executable loads in-memory sample definitions or `.scene` fixtures, invokes `scene::validation::validate_hierarchy`, and prints summaries plus JSON mirroring `HierarchyValidationReport`. Docs cross-link the CLI and expected output so tooling/CI consumers can diff telemetry. Invariants: Preserve existing module build graph; avoid modifying validation semantics; keep `.scene` fixtures regenerable via the CLI; ensure documentation retains enforced TODO sections. Compatibility: Additive—no API changes, only a new executable target and doc updates. Security/Performance: CLI operates on local fixtures with no external inputs; runtime hot paths unaffected. Test Strategy: Build the new target via preset; run the CLI against the `invalid_hierarchy` sample to verify JSON output; execute `python scripts/validate_docs.py` to confirm documentation health.

## PATCH
```diff
--- a/engine/scene/CMakeLists.txt
+++ b/engine/scene/CMakeLists.txt
@@
 if (BUILD_TESTING)
     add_subdirectory(tests)
 endif ()
 
+add_subdirectory(samples)
```
```diff
+set(target_name scene_hierarchy_diagnostics_sample)
+
+add_executable(${target_name}
+    hierarchy_diagnostics_sample.cpp
+)
+
+target_link_libraries(${target_name}
+    PRIVATE
+        engine::project_options
+        engine_scene
+)
+
+set_target_properties(${target_name}
+    PROPERTIES
+        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/scene"
+)
```

## TESTS
N/A

## DOCS
```diff
+-## Sample Library
+-
+-`engine/scene/samples` ships the `scene_hierarchy_diagnostics_sample`
+-CLI introduced by `SC-225`. It exercises the hierarchy validation
+-workflow end-to-end with reproducible fixtures and JSON output that
+-matches the runtime diagnostics schema.
+-
+-### Building the CLI
+-
+-```bash
+-cmake --build --preset <preset> --target scene_hierarchy_diagnostics_sample
+-```
+-
+-The executable is written to
+-`out/build/<preset>/engine/scene/scene_hierarchy_diagnostics_sample`.
+-
+-### Validating Built-in Fixtures
+-
+-Two fixtures demonstrate clean and failing hierarchies:
+-
+- * `valid_hierarchy` — three nodes with consistent transforms and
+-   relationships.
+- * `invalid_hierarchy` — introduces cycles, dangling parents, missing
+-   hierarchy components, and mismatched transforms.
+-
+-Invoke the CLI directly against the fixtures:
+-
+-```bash
+-./out/build/<preset>/engine/scene/scene_hierarchy_diagnostics_sample \
+-    --sample invalid_hierarchy --pretty --fail-on-issues
+-```
+-
+-`--scene <path>` validates external `.scene` files while `--emit-samples`
+-regenerates the committed fixtures under
+-`engine/scene/samples/data/`. Use `--list-samples` to enumerate the
+-available definitions.
+-
+-### Expected Output
+-
+-Every run prints a human-readable summary followed by JSON mirroring
+-`HierarchyValidationReport`. Example output for the
+-`invalid_hierarchy` fixture:
+-
+-```json
+-{
+-  "ok": false,
+-  "metrics": {
+-    "issue_count": 3,
+-    "cycle_count": 1,
+-    "dangling_parent_count": 1,
+-    "missing_parent_hierarchy_count": 1,
+-    "non_finite_transform_count": 0,
+-    "transform_mismatch_count": 0
+-  },
+-  "issues": [
+-    {
+-      "entity": 4,
+-      "related": 3,
+-      "type": "missing_parent_hierarchy",
+-      "message": "Entity 4 references parent 3 that is missing a Hierarchy component"
+-    },
+-    {
+-      "entity": 2,
+-      "related": 123456789,
+-      "type": "dangling_parent",
+-      "message": "Entity 2 references invalid parent 123456789"
+-    },
+-    {
+-      "entity": 0,
+-      "related": 1,
+-      "type": "cycle",
+-      "message": "Cycle detected when traversing parent chain for entity 0 via parent 1"
+-    }
+-  ]
+-}
+-```
+-
+-Use these artefacts in documentation, dashboards, and CI pipelines to
+-compare runtime telemetry against expected diagnostics output.
```

## VERIFY
- `cmake --build --preset linux-gcc-debug --target scene_hierarchy_diagnostics_sample`
- `./out/build/linux-gcc-debug/engine/scene/scene_hierarchy_diagnostics_sample --sample invalid_hierarchy --pretty --fail-on-issues`
- `python scripts/validate_docs.py`

## REVIEW_FINDINGS
- Correctness: CLI builds and emits validation JSON matching `HierarchyValidationReport`; docs and roadmap updates consistently mark `SC-225` complete. ✅
- Security: No new external inputs; sample operates on committed fixtures. ✅
- Compatibility: Additive executable target; existing APIs untouched. ✅
- Quality & Style: CMake wiring mirrors module/test patterns; documentation follows enforced sections and adds actionable examples. ✅
- Performance: CLI runs off hot path; main runtime unaffected. ✅
- Tests: Manual sample invocation covers behaviour; no automated coverage required beyond build. ✅
- Observability: Docs now publish example JSON enabling dashboard diffs; runtime TODO updated to consume sample output. ✅
- Docs: README, roadmap, module guides, review follow-ups synchronised. ✅

## REVIEW_PATCHES
N/A

## FINAL_PATCH
See ## PATCH / ## DOCS.

## FOLLOW_UP_TODOS
- [ ] Integrate `scene_hierarchy_diagnostics_sample` into a nightly CI job to regenerate fixtures and catch regressions (owner: Scene, priority: Medium, rationale: keep samples fresh).
- [ ] Extend the telemetry viewer to ingest the emitted JSON directly for dashboard smoke tests (owner: Runtime, priority: Medium, rationale: observability alignment).
- [ ] Evaluate sharing serialization helpers between the sample and runtime diagnostics bridge to reduce duplication (owner: Runtime, priority: Low, rationale: future refactor).
- [ ] Add a diagnostics runbook section showing how to triage sample output alongside runtime telemetry (owner: Docs, priority: Medium, rationale: documentation depth).
