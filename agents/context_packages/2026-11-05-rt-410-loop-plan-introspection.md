# Context Package — RT-410 Loop Plan Introspection Bridge

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** `agents/task_briefs/2026-11-05-rt-410-loop-plan-introspection.md`
- **Backlog Entry:** [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md)
- **Workflow Phase:** Phase 2 complete → hand-off to Phase 3 (Specialist Engineer)

## 2. Problem Summary
- Current behaviour:
  - `RuntimeDiagnostics::loop_plan_serialization` captures the compiled stage graph, but scripting clients must fetch it indirectly through the full diagnostics structure, which lacks a dedicated C export.
  - Python bindings only expose `presentation_stage_active()`; loop-plan inspection requires bespoke native extensions.
- Desired behaviour:
  - Provide a stable runtime export returning the loop-plan JSON so Python tooling can confirm stage composition when presentation adapters are toggled.
  - Loader API should decode the string and surface an ergonomic accessor with failure handling.
- Constraints / invariants:
  - The serialization must remain valid UTF-8 and stable for the duration of a frame.
  - No additional allocations or copies that could invalidate existing diagnostics ownership semantics.
- Quality budgets / telemetry notes:
  - Change should not affect runtime performance; export is read-only.
  - Ensure empty-plan cases return an empty string rather than `nullptr` to simplify bindings.

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Documents diagnostics serialization but lacks scripting bridge.【F:docs/modules/runtime/README.md†L229-L244】 | 5 |
| ADR / Spec | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Decision item 4 mandates scripting/inspection hooks.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L62-L115】 | 6 |
| Roadmap | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 prioritises RT-410 alongside GPU enablement.【F:docs/ROADMAP.md†L64-L93】 | 3 |
| Backlog | [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD explicitly includes scripting/diagnostics hooks.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】 | 4 |
| Code excerpts | [`engine/runtime/src/api.cpp`](../../engine/runtime/src/api.cpp) | Houses diagnostics + C exports; new symbol will live here. | — |
| Python loader | [`python/engine3g/loader.py`](../../python/engine3g/loader.py) | Needs new accessor + symbol validation. | — |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Runtime flagged “At Risk” pending RT-410 synchronisation hooks.【F:README.md†L26-L27】 | Product Manager | Update status post-merge. |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Confirms roadmap/backlog precedence chain for milestone work. | Knowledge Librarian | None. |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | RT-410 is priority-1 deliverable within Phase 4. | Knowledge Librarian | Ensure roadmap note references scripting hook once merged. |
| 4 | [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | Scripting/diagnostics hook required for DoD; this task fulfils part of it. | Specialist Engineer | Mark checklist progress after QA. |
| 5 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | `diagnostics.loop_plan_serialization` already documented; needs update for scripting accessor. | Docs/DevRel | Add reference to new API. |
| 6 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Section 4 emphasises scripting bridge for loop configuration/inspection. | Specialist Engineer | Ensure implementation matches ADR intent. |
| 7 | Archived RT-410 context packages | Previous sessions covered telemetry, context, diagnostics; no scripting bridge yet. | Knowledge Librarian | None. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  - `cmake --preset linux-gcc-debug`
  - `cmake --build --preset linux-gcc-debug`
  - `ctest --preset linux-gcc-debug`
  - `pytest python/tests scripts/tests`
  - `python scripts/validate_docs.py`
- Additional presets / datasets: None required.
- Benchmark targets & expected deltas: N/A (read-only API addition).
- Tooling updates required: Update Python loader tests and documentation.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Is returning `const char*` to diagnostics string lifetime-safe for scripting? | Specialist Engineer | During implementation | Yes; diagnostics string persists until next plan rebuild. Documented in code comments. |
| Should loader treat missing symbol as fatal? | Specialist Engineer | During implementation | Yes; mirror `presentation_stage_active` behaviour to fail fast on outdated runtimes. |

## 7. Attachments
- Diagrams: N/A
- Data sets: N/A
- Additional notes: Once merged, coordinate with PM-510 weekly demo to highlight new scripting validation capability.

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
