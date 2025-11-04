# Context Package Template

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-02-27-RT_410_PRESENTATION_LOOP_REFRESH.md`](../task_briefs/2026-02-27-RT_410_PRESENTATION_LOOP_REFRESH.md)
- **Backlog Entry:** [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../../ROADMAP.md)
- **Workflow Phase:** Phase 2 — Context Assembly / Next gate: Specialist Engineer implementation approval.

## 2. Problem Summary
- Current behaviour: `RuntimeHost` builds its default loop plan during configuration; attaching presentation callbacks or backends later does not refresh the plan, so `presentation.dispatch` may remain excluded from execution reports when callbacks are registered post-initialization.【F:engine/runtime/src/api.cpp†L1913-L2187】
- Desired behaviour: Detect presentation callback/backend toggles and queue a default loop plan rebuild so diagnostics/telemetry reflect the active presentation stage without requiring reconfiguration.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L118】
- Constraints / invariants: Preserve ADR-0008 loop configurability, avoid disrupting GPU enablement milestones tracked on the roadmap, and maintain deterministic stage ordering/telemetry expected by tooling.【F:docs/ROADMAP.md†L64-L102】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L118】
- Quality budgets / telemetry notes: Presentation stage timing must remain deterministic and reported when active; no regression to runtime diagnostics or PM-510 telemetry cadence.【F:docs/modules/runtime/README.md†L96-L118】【F:docs/ROADMAP.md†L75-L102】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Describes presentation dispatch expectations and outstanding RT-410 work. | 5 |
| ADR / Spec | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Defines declarative loop planner and presentation backend contracts. | 6 |
| Code excerpts | [`engine/runtime/src/api.cpp`](../../../../engine/runtime/src/api.cpp) | Shows default loop plan build and lack of plan refresh in `set_presentation_callback`. | 7 |
| Telemetry / Benchmarks | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 cadence and PM-510 telemetry expectations inform validation scope. | 3 |
| Prior PRs / Discussions | [`agents/task_briefs/2026-02-26-RT_410_PRESENTATION_TELEMETRY.md`](../task_briefs/2026-02-26-RT_410_PRESENTATION_TELEMETRY.md) | Previous session focused on presentation telemetry validation; informs risk context. | 7 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [README.md](../../../../README.md) | Runtime module flagged “At Risk”; standard build/test workflow recorded for QA parity.【F:README.md†L13-L144】 | Knowledge Librarian | None |
| 2 | [docs/NAVIGATION.md](../../../NAVIGATION.md) | Confirms precedence chain and template requirements for agents.【F:docs/NAVIGATION.md†L5-L116】 | Knowledge Librarian | None |
| 3 | [docs/ROADMAP.md](../../../ROADMAP.md) | Phase 4 priorities place RT-410 alongside GPU tasks; change must not destabilise milestone cadence.【F:docs/ROADMAP.md†L64-L102】 | Knowledge Librarian | Coordinate updates with PM-510 notes if telemetry changes. |
| 4 | [RT-410 backlog](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD calls for presentation adapters and synchronisation hooks with automated tests; plan rebuild aids telemetry coverage.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】 | Knowledge Librarian | Ensure DoD checklist updated once tests land. |
| 5 | [Runtime module README](../../../modules/runtime/README.md) | Highlights presentation dispatch bridging tooling and runtime; documentation must reflect hot-swapping behaviour.【F:docs/modules/runtime/README.md†L96-L118】 | Knowledge Librarian | Update README after implementation. |
| 6 | [ADR-0008](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Requires declarative loop + presentation backend separation; rebuild logic must comply.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L118】 | Knowledge Librarian | Verify no ADR updates needed. |
| 7 | [Runtime loop implementation](../../../../engine/runtime/src/api.cpp) | Identified absence of plan refresh when callbacks/backends change, motivating hook addition.【F:engine/runtime/src/api.cpp†L1913-L2187】 | Knowledge Librarian | Draft implementation plan. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied: `cmake --preset linux-gcc-debug`, `cmake --build --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, `pytest python/tests scripts/tests`, `python scripts/validate_docs.py`.【F:README.md†L120-L144】
- Additional presets / datasets: None required; runtime changes exercised via existing unit tests.
- Benchmark targets & expected deltas: No performance benchmarks expected; monitor runtime stage timing output for regressions.
- Tooling updates required: Update runtime README; no additional tooling scripts anticipated.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Do we need a public setter for presentation backends in addition to callback refresh? | Specialist Engineer | Implementation phase | Pending — evaluate during design; likely yes for RT-410 readiness.【F:engine/runtime/src/api.cpp†L1913-L2187】 |
| Should plan rebuild failures fall back to previous plan or empty plan? | Specialist Engineer | Implementation phase | Pending — propose logging error and retaining prior plan unless build succeeds. |

## 7. Attachments
- Diagrams: None.
- Data sets: N/A.
- Additional notes: Coordinate with QA to extend runtime tests validating presentation stage telemetry once rebuild hook lands.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
