# Context Package Template

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [2026-03-01-rt-410-presentation-stage-capi](../task_briefs/2026-03-01-RT_410_PRESENTATION_STAGE_CAPI.md)
- **Backlog Entry:** [RT-410 — Runtime Stage Planner & Presentation Loop](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)
- **Roadmap Link:** [docs/ROADMAP.md](../../../ROADMAP.md)
- **Workflow Phase:** Phase 2 complete → entering Phase 3 (Execution) once bindings plan approved

## 2. Problem Summary
- Current behaviour: The runtime exposes `RuntimeHost::presentation_stage_active()` and a global C++ helper, but the C interface and Python loader lack equivalent entry points, forcing tooling to inspect diagnostics or duplicate logic when determining whether presentation handlers are active.【F:engine/runtime/include/engine/runtime/api.hpp†L293-L346】【F:python/engine3g/loader.py†L317-L360】
- Desired behaviour: Export a C ABI function (`engine_runtime_presentation_stage_active`) and surface it through `EngineRuntimeHandle` so scripting and harness code can conditionally engage presentation features without parsing execution reports, satisfying RT-410’s synchronisation hook expectations.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:docs/modules/runtime/README.md†L52-L128】
- Constraints / invariants: Maintain ADR-0008’s declarative loop semantics—bindings must remain read-only, avoid implicit initialization, and respect existing toggles for callbacks/backends.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L129】【F:engine/runtime/tests/test_module.cpp†L2330-L2684】
- Quality budgets / telemetry notes: Loader tests must enforce symbol availability; runtime presentation telemetry should remain deterministic across callback/backend permutations to keep PM-510 demos reproducible.【F:python/tests/test_loader.py†L38-L120】【F:docs/ROADMAP.md†L75-L108】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [docs/modules/runtime/README.md](../../../modules/runtime/README.md) | Documents presentation stage behaviour, outstanding synchronisation work, and helper expectations.【F:docs/modules/runtime/README.md†L5-L128】 | 5 |
| ADR / Spec | [ADR-0008](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Requires scripting hooks via C API and presentation instrumentation alignment.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L129】 | 6 |
| Code excerpts | [engine/runtime/include/engine/runtime/api.hpp](../../../../engine/runtime/include/engine/runtime/api.hpp) & [engine/runtime/src/api.cpp](../../../../engine/runtime/src/api.cpp) | C++ accessor exists but no exported C symbol; implementation toggles telemetry on callback/backend changes.【F:engine/runtime/include/engine/runtime/api.hpp†L293-L346】【F:engine/runtime/src/api.cpp†L2169-L2237】 | 7 |
| Python bindings | [python/engine3g/loader.py](../../../../python/engine3g/loader.py) | Loader initialises runtime symbols but lacks presentation-stage accessor, prompting this change.【F:python/engine3g/loader.py†L317-L360】 | 7 |
| Tests | [engine/runtime/tests/test_module.cpp](../../../../engine/runtime/tests/test_module.cpp), [python/tests/test_loader.py](../../../../python/tests/test_loader.py) | Runtime tests validate stage toggles; loader tests stub runtime namespace and need expansion to cover the new symbol.【F:engine/runtime/tests/test_module.cpp†L2330-L2684】【F:python/tests/test_loader.py†L38-L120】 | 7 |
| Roadmap context | [docs/ROADMAP.md](../../../ROADMAP.md) | Phase 4 priority on RT-410 emphasises scripting/tooling readiness alongside GPU milestones.【F:docs/ROADMAP.md†L64-L108】 | 3 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [README.md](../../../../README.md) | Runtime “At Risk”; standard build/test workflow captured for validation logs.【F:README.md†L15-L144】 | Knowledge Librarian | Ensure command block executed post-change. |
| 2 | [docs/NAVIGATION.md](../../../NAVIGATION.md) | Reiterated documentation precedence and template usage for artefacts.【F:docs/NAVIGATION.md†L5-L116】 | Knowledge Librarian | None |
| 3 | [docs/ROADMAP.md](../../../ROADMAP.md) | Phase 4 timeline shows RT-410 deliverables needed before PM-510 demos; bindings unblock tooling readiness.【F:docs/ROADMAP.md†L64-L108】 | Knowledge Librarian | Note progress in PM-510 updates. |
| 4 | [RT-410 backlog](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD checklist demands synchronisation hooks with documentation; C API bridge addresses scripting facet.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】 | Knowledge Librarian | Append backlog note after merge. |
| 5 | [docs/modules/runtime/README.md](../../../modules/runtime/README.md) | README emphasises presentation stage gating and global helper; will expand with C bindings guidance.【F:docs/modules/runtime/README.md†L52-L128】 | Knowledge Librarian | Draft doc update in execution phase. |
| 6 | [ADR-0008](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Confirms requirement for scripting hooks across C/Python surfaces; no ADR change required.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L129】 | Knowledge Librarian | Ensure implementation references ADR where appropriate. |
| 7 | [python/engine3g/loader.py](../../../../python/engine3g/loader.py) & tests | Loader sets ctypes prototypes for runtime functions but lacks presentation stage query; tests will stub symbol to guard integration.【F:python/engine3g/loader.py†L317-L360】【F:python/tests/test_loader.py†L38-L120】 | Knowledge Librarian | Coordinate with Specialist Engineer on test additions. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None.
- Benchmark targets & expected deltas: No performance-sensitive paths touched; runtime diagnostics should remain unchanged.
- Tooling updates required: Update loader documentation and backlog note; run `python/scripts/validate_docs.py` after editing README/backlog.【F:README.md†L120-L144】

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should the loader raise a descriptive `RuntimeError` if the symbol is missing to aid older binaries? | Specialist Engineer | During implementation | Pending — propose explicit error message guarded by unit test.【F:python/tests/test_loader.py†L38-L120】 |
| Do Python bindings require a high-level convenience method or property for presentation stage activity? | Specialist Engineer | During implementation | Pending — evaluate ergonomics while wiring ctypes handle. |

## 7. Attachments
- Diagrams: None.
- Data sets: N/A.
- Additional notes: Ensure backlog note records availability date so downstream teams know when to depend on the C API.

> **Checklist:** Confirm symbol export names follow existing conventions, cite changes in [`CONTRIBUTION.md`](../../../../CONTRIBUTION.md) if style questions arise, and notify Docs/DevRel once README/backlog updates merge.
