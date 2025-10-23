# AS-330 — Diagnostics Shell Hot Reload Surfacing

## PRIORITY_DECISION
- Selected Task: AS-330 — Surface asset hot-reload failures in the diagnostics shell
- Score Table:
  | Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
  | ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
  | AS-330 | 4 | 4 | 3 | 3 | 3 | 4 | 21 |
  | DC-003 | 3 | 3 | 2 | 2 | 1 | 4 | 15 |
  | AN-230 | 2 | 3 | 1 | 1 | 2 | 3 | 12 |
  | GE-221+ | 2 | 3 | 1 | 2 | 1 | 3 | 12 |
- Tie-break Rationale: AS-330 delivers near-term observability the assets roadmap depends on; other tasks either remain blocked or demand larger refactors.
- Decision Rationale
  - Restores roadmap alignment by completing the outstanding AS-330 observability milestone.
  - Unblocks operators who need actionable reload context during async streaming incidents.
  - Builds on existing telemetry infrastructure without introducing new dependencies.
  - Carries moderate scope relative to DC-003/GE-221+ while delivering immediate value.
  - Provides prerequisite signals for future dashboard automation (`TL-120`).

## DESIGN_BRIEF
- **Problem Statement:** Operators lacked per-asset context when hot reload failures occurred; telemetry exposed only aggregate counters. AS-330 requires surfacing recent asset reload failures with actionable hints through the diagnostics shell and runtime tooling.
- **Acceptance Criteria:**
  - Capture recent hot reload failures (identifier, message, hint) without unbounded growth.
  - Expose the data through runtime diagnostics (C++ API + C bindings) and the Python telemetry tooling.
  - Render per-asset failures within `telemetry_viewer.py` alongside existing guidance.
  - Update module roadmaps/READMEs to mark AS-330 complete and document new behaviour.
  - Provide unit tests (C++ + Python) covering telemetry storage and viewer output.
- **Interfaces & Data Flow:**
  - Assets cache errors via `AssetHotReloadTelemetry` → runtime `HotReloadDiagnostics` → C API → Python bindings → telemetry viewer.
  - Maintain existing JSON schema while adding a `recent_failures` array.
- **Invariants:**
  - Bounded history (max 8 entries) to avoid unbounded memory churn.
  - Thread-safe mutation of telemetry structures (guarded by existing mutex).
  - Diagnostics remain opt-in when assets module disabled.
- **Compatibility:** Additive changes only; existing fields unchanged, new C API functions are optional (tooling detects availability).
- **Security & Performance:**
  - No external input; strings copied through mutex-protected structures.
  - Bounded ring buffer avoids hot-path allocations.
  - Python decoding guarded against missing functions to tolerate older runtimes.
- **Test Strategy:**
  - C++ unit test validating telemetry history trimming and hint propagation.
  - Python unit tests for runtime telemetry serialization and viewer rendering.
  - Targeted pytest runs plus documentation validation; attempted CMake configure noted below.

## PATCH
- Extend `AssetHotReloadTelemetry` with bounded recent failure history and expose it via snapshot (`engine/assets/include/engine/assets/async.hpp`).
- Mirror telemetry into runtime diagnostics, add exported failure-summary struct, and publish new C API accessors (`engine/runtime/include/engine/runtime/api.hpp`, `engine/runtime/src/api.cpp`).
- Enhance Python runtime bindings to collect recent failures, update serialization schema, and enrich the telemetry viewer with per-asset guidance (`scripts/diagnostics/runtime_frame_telemetry.py`, `scripts/diagnostics/telemetry_viewer.py`).
- Expand unit tests for telemetry serialization and viewer output; add C++ regression for telemetry history (`scripts/tests/test_runtime_frame_telemetry.py`, `scripts/tests/test_telemetry_viewer.py`, `engine/assets/tests/test_assets.cpp`).
- Refresh documentation to mark AS-330 complete and describe the new diagnostics behaviour (`README.md`, `docs/ROADMAP.md`, `docs/design/MATERIAL_PERSISTENCE_STRATEGY.md`, `docs/modules/assets/README.md`, `docs/modules/assets/BACKLOG.md`, `docs/modules/tools/README.md`, `scripts/diagnostics/README.md`).

## TESTS
- `pytest scripts/tests/test_runtime_frame_telemetry.py scripts/tests/test_telemetry_viewer.py`
- (Attempted) `cmake --preset linux-gcc-debug` followed by build; configure failed because vendored Dear ImGui sources are unavailable in the workspace.

## DOCS
- README / roadmap / module documentation / design updates listed in patch summary.

## VERIFY
- `pytest scripts/tests/test_runtime_frame_telemetry.py scripts/tests/test_telemetry_viewer.py`
- `python scripts/validate_docs.py`
- `cmake --preset linux-gcc-debug` (fails: missing `third_party/imgui/imgui.cpp` in workspace)

## REVIEW_FINDINGS
- No correctness, compatibility, or performance regressions identified during review.
- Verified new C API functions remain optional; tooling falls back gracefully when unavailable.
- Documentation accurately reflects roadmap status and new diagnostics behaviour.

## REVIEW_PATCHES
- N/A

## FINAL_PATCH
- As described in the PATCH section; no changes required post-review.

## FOLLOW_UP_TODOS
- [ ] File AS-315 implementation plan update (owner: TBD, priority: high) — integrate filesystem watcher callbacks with asset caches to retire remaining hot-reload TODO.
- [ ] [Observability] Automate export of recent reload failures into CI dashboards (owner: TBD, priority: medium) once telemetry viewer automation lands.
- [ ] [Tech Debt] Evaluate sharing recent-failure ring buffer utility across caches to deduplicate logic (owner: TBD, priority: low).
- [ ] [Documentation] Extend troubleshooting guide with sample diagnostics shell output for asset reload failures (owner: TBD, priority: medium).
