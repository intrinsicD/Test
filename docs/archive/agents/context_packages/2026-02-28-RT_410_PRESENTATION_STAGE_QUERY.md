# Context Package Template

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [2026-02-28-rt-410-presentation-stage-query](../task_briefs/2026-02-28-RT_410_PRESENTATION_STAGE_QUERY.md)
- **Backlog Entry:** [RT-410 — Runtime Stage Planner & Presentation Loop](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)
- **Roadmap Link:** [docs/ROADMAP.md](../../../ROADMAP.md)
- **Workflow Phase:** Phase 2 → Phase 3 hand-off (context assembled; implementation pending Specialist Engineer approval)

## 2. Problem Summary
- Current behaviour: `RuntimeHost` rebuilds its loop plan when presentation callbacks/backends change, but whether the `presentation.dispatch` stage is active is only observable by inspecting diagnostics/dispatch reports; there is no public API exposing the boolean guard even though the implementation tracks it internally via `presentation_stage_enabled()`.【F:engine/runtime/src/api.cpp†L1975-L2229】【F:engine/runtime/tests/test_module.cpp†L2332-L2557】
- Desired behaviour: Expose a stable accessor (and corresponding global helper) so tooling, scripts, and harnesses can ask if presentation handlers are currently engaged without parsing diagnostics output, satisfying RT-410’s synchronisation hook requirement.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:docs/modules/runtime/README.md†L52-L124】
- Constraints / invariants: Maintain ADR-0008’s declarative loop model—accessor must remain a read-only view that does not mutate scheduling, and it must respect feature toggles when rendering is disabled.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L129】
- Quality budgets / telemetry notes: Presentation stage visibility drives diagnostics/telemetry consumers (stage timings, dependency graphs); new API should align with existing tests ensuring callbacks/backends toggle execution order and reports deterministically.【F:engine/runtime/tests/test_module.cpp†L2332-L2557】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [docs/modules/runtime/README.md](../../../modules/runtime/README.md) | Documents presentation stage behaviour, hot-swapping semantics, outstanding synchronisation work. | 5 |
| ADR / Spec | [ADR-0008](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Requires scripting/instrumentation hooks for presentation-aware loop configuration. | 6 |
| Code excerpts | [engine/runtime/src/api.cpp](../../../../engine/runtime/src/api.cpp) | Contains `presentation_stage_enabled()`, loop-plan rebuild, and callback/backend mutators. | 7 |
| Tests | [engine/runtime/tests/test_module.cpp](../../../../engine/runtime/tests/test_module.cpp) | Validates presentation stage toggles for callbacks/backends, ensuring behaviour to preserve. | 7 |
| Roadmap context | [docs/ROADMAP.md](../../../ROADMAP.md) | Phase 4 priorities emphasise RT-410 coordination alongside GPU enablement. | 3 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [README.md](../../../../README.md) | Runtime flagged “At Risk”; standard build/test workflow reaffirmed. | Knowledge Librarian | None |
| 2 | [docs/NAVIGATION.md](../../../NAVIGATION.md) | Confirmed precedence and template usage for brief/context capture. | Knowledge Librarian | None |
| 3 | [docs/ROADMAP.md](../../../ROADMAP.md) | Phase 4 lists RT-410 with priority 1 alongside T-0120/T-0119; risk table highlights need for stage planner progress. | Knowledge Librarian | Keep roadmap entry aligned post-change. |
| 4 | [RT-410 backlog](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD requires synchronisation hooks and tooling integration; accessor will address discoverability gap. | Knowledge Librarian | Update backlog status notes if accessor satisfies sub-requirement. |
| 5 | [docs/modules/runtime/README.md](../../../modules/runtime/README.md) | README describes hot-swapping and telemetry; will need update referencing new accessor. | Knowledge Librarian | Draft doc snippet after implementation. |
| 6 | [ADR-0008](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Confirms instrumentation/scripting expectations; accessor must align without altering plan semantics. | Knowledge Librarian | Verify no ADR amendment needed. |
| 7 | [engine/runtime/src/api.cpp](../../../../engine/runtime/src/api.cpp) & tests | Identified existing private boolean helper and regression coverage to leverage for new API/test additions. | Knowledge Librarian | Coordinate with Specialist Engineer on implementation/test strategy. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None required; runtime tests run in default preset.
- Benchmark targets & expected deltas: No performance benchmarks affected; ensure runtime test suite remains deterministic.
- Tooling updates required: Update runtime module README; no automation changes anticipated.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should the global runtime facade expose the same accessor for parity with other presentation helpers? | Specialist Engineer | During implementation | Pending (assumed yes for ergonomics). |

## 7. Attachments
- Diagrams: None.
- Data sets: N/A.
- Additional notes: Ensure documentation cites new API name and emphasises that it mirrors diagnostics behaviour rather than introducing new state.

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
