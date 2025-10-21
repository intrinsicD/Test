# CR-135 Subsystem Dependency Diagnostics Implementation

## PRIORITY_DECISION
- Selected Task: CR-135 — Subsystem dependency diagnostics
- Score Table:

| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| CR-135 — Subsystem dependency diagnostics | 3 | 4 | 4 | 4 | 3 | 5 | 23 |
| IO-221 — Signature database & fuzz harness | 2 | 5 | 1 | 4 | 1 | 4 | 17 |
| PL-215 — SDL parity checklist | 3 | 2 | 2 | 2 | 5 | 3 | 17 |

- Tie-break Rationale: Higher aggregate score and directly reinforces `DC-001` invariants.
- Decision Rationale:
  - Cycle detection closes the remaining gap from the CR-125 lifecycle review.
  - Prevents hard-to-debug runtime startup failures by surfacing explicit cycle paths.
  - Unblocks follow-on observability work by documenting telemetry expectations.
  - Work is self-contained within core/runtime and can land without external dependencies.
  - Aligns with architecture guidance prioritising correctness and clarity over new features.

## DESIGN_BRIEF
- **Problem Statement:** The subsystem registry allowed cyclic dependency graphs, leaving runtime startup failures unexplained and undocumented. RuntimeHost dependency validation did not detect cycles when subsystems were injected manually.
- **Acceptance Criteria:**
  - Registry rejects cyclic descriptor graphs and preserves previous state on failure.
  - RuntimeHost dependency validation reports `RuntimeError::dependency_cycle` with the offending path.
  - Unit tests cover registry and explicit plugin cycle scenarios.
  - Documentation (core README/roadmap, plugin architecture design, runtime diagnostics guide, central roadmap, root README) reflects the new diagnostics and telemetry guidance.
- **Interfaces & Contracts:**
  - `SubsystemRegistry::register_subsystem` throws `std::invalid_argument` when a cycle is detected.
  - `RuntimeError` enum gains `dependency_cycle`; aggregated validation errors include the new identifier and message.
  - `RuntimeHostDependencies` validation inspects `subsystem_plugins` before configuration mutates runtime state.
- **Data Flow:**
  - Depth-first traversal over descriptor graphs records recursion stacks and extracts cycle paths on back-edges.
  - Runtime dependency validation maps plugin names to indices, reusing DFS to detect cycles among explicitly provided subsystems.
- **Invariants:**
  - Registry state remains unchanged when validation fails.
  - Unknown dependencies (not yet registered or provided) are ignored—cycles are reported only when every vertex is present.
  - Error messages include the cycle path to support diagnostics tooling and logs.
- **Compatibility/Migration:** No API surface changes; only new failure modes for invalid configurations. Valid registries continue to function.
- **Security/Performance:** Detection runs in O(V + E) during registration/validation; negligible compared to subsystem startup costs. No unbounded allocations.
- **Test Strategy:** Extend runtime unit tests with registry and manual plugin cycle regressions; rely on existing runtime test preset.

## PATCH
- Introduced cycle detection helpers in `engine/runtime/src/subsystem_registry.cpp` and rolled back descriptor updates when validation fails.
- Added plugin cycle analysis and new `dependency_cycle` error handling in `engine/runtime/src/api.cpp` and `engine/runtime/include/engine/runtime/errors.hpp`.
- Expanded runtime unit tests (`engine/runtime/tests/test_module.cpp`) covering registry and explicit plugin cycle scenarios.
- Updated documentation (`docs/modules/core/README.md`, `docs/modules/core/ROADMAP.md`, `docs/design/plugin_architecture.md`, `docs/modules/runtime/diagnostics.md`, `docs/ROADMAP.md`, `README.md`, `docs/reviews/2025-03-28-core-plugin-lifecycle.md`) to mark CR-135 complete and describe diagnostics/telemetry guidance.
- Extended `scripts/validate_docs.py` roadmap identifier detection to include `CR-###` entries and avoid false positives in module TODO sections.

## TESTS
- `ctest --preset linux-gcc-debug --tests-regex engine_runtime_tests`

## DOCS
- See PATCH section for the full list of documentation updates.

## VERIFY
- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug --target engine_runtime_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_runtime_tests`
- `python scripts/validate_docs.py`

## REVIEW_FINDINGS
- Correctness: Cycle detection rejects registry and manual plugin graphs; tests exercise both failure paths.
- Quality & Style: Error messages include cycle paths and reuse `RuntimeError::dependency_cycle`; documentation updates align with roadmap tables.
- Compatibility: No API shape changes; new exceptions only trigger on invalid configurations.
- Tests: Runtime unit suite extended with regression coverage.

## REVIEW_PATCHES
- N/A

## FINAL_PATCH
- No additional adjustments required after review.

## FOLLOW_UP_TODOS
- [ ] `CR-136`: Structured logging for subsystem initialization failures (owner: Core, medium priority, follows CR-135 telemetry guidance).
- [ ] Update diagnostics viewer to surface dependency cycle failures (observability task).
- [ ] Evaluate registry tooling reuse for editor-facing dependency graphs (tech debt).
- [ ] Extend runtime runbook with dependency failure troubleshooting workflow (documentation).
