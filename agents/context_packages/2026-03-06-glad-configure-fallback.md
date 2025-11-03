# Context Package — GLAD-Optional Geometry Viewer Build Guard

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`../task_briefs/2026-03-06-glad-configure-fallback.md`](../task_briefs/2026-03-06-glad-configure-fallback.md)
- **Backlog Entry:** [`docs/backlog/active/PM-520-backlog-hygiene-remediation.md`](../../docs/backlog/active/PM-520-backlog-hygiene-remediation.md)
- **Roadmap Link:** [`docs/ROADMAP.md#phase-4--gpu-execution--tooling-readiness-priorities-1–2`](../../docs/ROADMAP.md#phase-4--gpu-execution--tooling-readiness-priorities-1–2)
- **Workflow Phase:** Phase 2 — Context Assembly (handoff to Specialist Engineer)

## 2. Problem Summary
- Current behaviour: CMake configure fails on the canonical preset because the geometry_viewer example always links against `glad::gl_core`, which is absent when GLAD generation is skipped due to missing Python/Jinja dependencies.【f7515f†L1-L19】【F:engine/tools/examples/CMakeLists.txt†L1-L19】【F:third_party/cmake/glad.cmake†L1-L33】
- Desired behaviour: Configure should succeed by skipping or adjusting geometry_viewer when its optional dependencies are unavailable so QA can run the canonical build pipeline documented in the README.【F:README.md†L122-L142】
- Constraints / invariants: Must respect platform backend fallback guidance (GLFW optional, mock backend default when dependencies missing) and avoid re-enabling disabled tooling features tracked by TL-310.【F:docs/modules/platform/README.md†L474-L526】【F:docs/modules/tools/README.md†L1-L24】
- Quality budgets / telemetry notes: No runtime performance impact; goal is to restore build/test automation for PM-520 quality follow-up.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L1-L64】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) | Tools module disabled; geometry_viewer sample lives here. | 5 |
| Module README | [`docs/modules/platform/README.md`](../../docs/modules/platform/README.md) | Describes GLFW fallback expectations. | 5 |
| ADR / Spec | N/A | No ADR modifications required; GLAD optionality covered by build scripts. | 6 |
| Code excerpts | [`engine/tools/examples/CMakeLists.txt`](../../engine/tools/examples/CMakeLists.txt) | Unconditional `glad::gl_core` link triggers configure failure. | 7 |
| Build script | [`third_party/cmake/glad.cmake`](../../third_party/cmake/glad.cmake) | Explains when GLAD target is absent. | 6 |
| Prior logs | CMake configure log (`f7515f`) | Documents failure scenario. | 1 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Canonical quality instrumentation block must succeed; currently blocked by configure failure.【F:README.md†L122-L142】【f7515f†L1-L19】 | Knowledge Librarian | Ensure post-fix commands logged |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Confirms documentation precedence and need to sync module READMEs after changes.【F:docs/NAVIGATION.md†L1-L113】 | Knowledge Librarian | Update tools README |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 readiness depends on stable GPU/tooling demos; configure fix supports this cadence.【F:docs/ROADMAP.md†L64-L118】 | Knowledge Librarian | Reference in task brief |
| 4 | [`docs/backlog/active/PM-520-backlog-hygiene-remediation.md`](../../docs/backlog/active/PM-520-backlog-hygiene-remediation.md) | Outstanding follow-up requires rerunning canonical commands after dependency fix.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L1-L64】 | Knowledge Librarian | Close loop in quality report |
| 5 | [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) | Document conditional behaviour for geometry_viewer and keep scope limited to tooling module. | Knowledge Librarian | Add note about optional build |
| 5 | [`docs/modules/platform/README.md`](../../docs/modules/platform/README.md) | Reinforces optional GLFW backend; our change must preserve fallback semantics.【F:docs/modules/platform/README.md†L474-L526】 | Knowledge Librarian | Ensure CMake guard respects flag |
| 6 | [`third_party/cmake/glad.cmake`](../../third_party/cmake/glad.cmake) | GLAD target omitted when Python/Jinja missing; we must not assume it exists. | Knowledge Librarian | Condition example on target |
| 7 | [`engine/tools/examples/CMakeLists.txt`](../../engine/tools/examples/CMakeLists.txt) | Current failure point; plan to guard target creation. | Knowledge Librarian | Implement conditional guard |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied: `cmake --preset linux-gcc-debug`, `cmake --build --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, `pytest python/tests scripts/tests`, `python scripts/validate_docs.py`.
- Additional presets / datasets: None required for this fix.
- Benchmark targets & expected deltas: N/A (no runtime execution expected).
- Tooling updates required: Add CMake status message for skipped example, document behaviour in README.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should the geometry_viewer example be entirely disabled when GLFW unavailable? | Specialist Engineer | 2026-03-06 | Yes; skip target with CMake status message to avoid configure failure. |

## 7. Attachments
- Diagrams: None.
- Data sets: None.
- Additional notes: Once dependencies are installed in future sessions, the guard should allow the example to build automatically without manual toggles.

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
