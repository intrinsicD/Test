# Context Package Template

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [agents/task_briefs/2026-02-26-rt-410-presentation-telemetry.md](../task_briefs/2026-02-26-rt-410-presentation-telemetry.md)
- **Backlog Entry:** [RT-410 — Runtime Stage Planner & Presentation Loop](../../docs/backlog/active/RT-410-runtime-stage-planner.md)
- **Roadmap Link:** [Phase 4 — GPU Execution & Tooling Readiness](../../docs/ROADMAP.md#L64)
- **Workflow Phase:** Phase 2 complete → entering Phase 3 (Execution).【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 2. Problem Summary
- Current behaviour: Runtime loop exposes `presentation.dispatch`, but automated coverage for callback vs. backend activation remains thin, keeping RT-410’s test checkbox open.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/modules/runtime/README.md†L5-L113】
- Desired behaviour: Deterministic execution reporting whenever presentation paths are configured, with tests validating diagnostics output for both callback-only and backend-enabled scenarios.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】
- Constraints / invariants: Respect ADR-0008 stage planning contract, avoid regressions to GPU enablement tasks (T-0120/T-0119).【F:docs/ROADMAP.md†L64-L102】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】
- Quality budgets / telemetry notes: Telemetry must remain deterministic for PM-510 weekly demo snapshots; ensure execution report ordering stable.【F:docs/ROADMAP.md†L75-L102】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [docs/modules/runtime/README.md](../../docs/modules/runtime/README.md) | Summarises outstanding RT-410 work and presentation dispatch usage.【F:docs/modules/runtime/README.md†L5-L113】 | 5 |
| ADR / Spec | [docs/specs/ADR-0008-runtime-main-loop-and-tooling.md](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Defines declarative loop + presentation backend obligations.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】 | 6 |
| Code excerpts | [engine/runtime/src/api.cpp](../../engine/runtime/src/api.cpp) | Implements loop plan application, execution reporting, and presentation dispatch handler. | 7 |
| Telemetry / Benchmarks | [README.md](../../README.md) | Lists required validation commands ensuring telemetry stability checks. 【F:README.md†L120-L142】 | 1 |
| Prior PRs / Discussions | N/A | No linked discussion threads identified during context sweep. | — |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Runtime flagged "At Risk" until RT-410 closes; maintain standard build/test workflow.【F:README.md†L13-L142】 | Knowledge Librarian | None |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Confirms ladder order and template usage for agents.【F:docs/NAVIGATION.md†L5-L108】 | Knowledge Librarian | None |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Prioritises RT-410 alongside GPU tasks; milestone due 2026-03-05.【F:docs/ROADMAP.md†L64-L102】 | Knowledge Librarian | Monitor schedule |
| 4 | [RT-410 backlog](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | Testing checklist still unchecked; emphasises presentation adapters/tests.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】 | Knowledge Librarian | Address via implementation |
| 5 | [Runtime README](../../docs/modules/runtime/README.md) | Documents presentation dispatch hook and outstanding synchronisation hooks.【F:docs/modules/runtime/README.md†L5-L113】 | Knowledge Librarian | Update if behaviour changes |
| 6 | [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Architectural guardrails for loop plan + presentation separation.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】 | Knowledge Librarian | Ensure compliance |
| 7 | [engine/runtime/src/api.cpp](../../engine/runtime/src/api.cpp) | Pending review of execution reporting toggle for presentation stage. | Specialist Engineer | Inspect before coding |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied: `cmake --preset linux-gcc-debug`, `cmake --build --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, `pytest python/tests scripts/tests`, `python scripts/validate_docs.py`.【F:README.md†L120-L142】
- Additional presets / datasets: None required; runtime tests rely on default mock backend fixtures.
- Benchmark targets & expected deltas: N/A — instrumentation-only change.
- Tooling updates required: Update PM-510 notes if telemetry output format shifts.【F:docs/ROADMAP.md†L75-L102】

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Does existing runtime test harness support toggling presentation callback/backends independently? | Specialist Engineer | 2026-02-26 | Inspect `engine/runtime/tests/test_module.cpp` before implementation. |
| Any downstream tooling parsing execution reports that may need fixture updates? | Knowledge Librarian | 2026-02-26 | Coordinate with Docs/DevRel if serialization changes occur. |

## 7. Attachments
- Diagrams: None.
- Data sets: None.
- Additional notes: Keep backlog RT-410 status updated after tests land; consider notifying PM-510 cadence owners if execution report output expands.【F:docs/ROADMAP.md†L75-L102】

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
