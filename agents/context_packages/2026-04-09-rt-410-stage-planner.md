# Context Package — RT-410 Stage Planner Presentation Integration

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`2026-04-09-rt-410-stage-planner.md`](../task_briefs/2026-04-09-rt-410-stage-planner.md)
- **Backlog Entry:** [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md)
- **Roadmap Link:** [`docs/ROADMAP.md#phase-4--gpu-execution--tooling-readiness-priorities-1–2`](../../docs/ROADMAP.md#phase-4--gpu-execution--tooling-readiness-priorities-1–2)
- **Workflow Phase:** Phase 2 — Context Assembly (handoff to Specialist Engineer)

## 2. Problem Summary
- Current behaviour: `RuntimeHost` compiles a declarative `RuntimeLoopPlan` and exposes presentation callbacks, yet GPU-aware presenters and synchronisation APIs remain outstanding, leaving presentation support incomplete.【F:docs/modules/runtime/README.md†L5-L170】
- Desired behaviour: Implement ADR-0008 requirements so runtime stage planner selects presentation backends (mock + GLFW), wires synchronisation hooks, and enables tooling reuse while supporting GPU submission milestones.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L115】
- Constraints / invariants: Must honour ADR-0008 contracts (declarative stages, presentation backend separation, ImGui panel registry) and coordinate with GPU tasks T-0119/T-0120 plus platform capability guidelines from prior kickoff work.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L39-L48】
- Quality budgets / telemetry notes: Maintain per-stage telemetry and harness regression coverage while integrating presenters; reuse PM-510 demo cadence for telemetry snapshots.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】【F:docs/ROADMAP.md†L76-L95】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Outlines RuntimeHost loop plan, presentation hooks, outstanding work for RT-410.【F:docs/modules/runtime/README.md†L5-L176】 | 5 |
| ADR / Spec | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Defines declarative loop, presentation backend abstraction, tooling registry obligations.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】 | 6 |
| Code excerpts | [`engine/runtime/src/api.cpp`](../../engine/runtime/src/api.cpp) | Existing presentation callback/back-end plumbing; verify activation toggles (reference lines cited in module README).【F:docs/modules/runtime/README.md†L160-L174】 | 6 |
| Telemetry / Benchmarks | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | PM-510 cadence captures telemetry expectations for GPU/runtime milestones.【F:docs/ROADMAP.md†L76-L95】 | 3 |
| Prior PRs / Discussions | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | Notes recent exports (`set_loop_plan`, presentation stage active) and outstanding deliverables.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L48-L54】 | 4 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Runtime flagged "At Risk" because presentation adapters missing; Phase 4 backlog emphasised.【F:README.md†L26-L29】【F:README.md†L98-L107】 | Knowledge Librarian | None |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Workflow requires consulting module READMEs and ADRs before code changes.【F:docs/NAVIGATION.md†L11-L43】 | Knowledge Librarian | None |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | RT-410 priority 1 milestone scheduled; dependencies with GPU tasks recorded.【F:docs/ROADMAP.md†L64-L83】 | Knowledge Librarian | Track schedule slip risk |
| 4 | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | DoD enumerates presentation backends, synchronisation hooks, doc/test updates; dependencies on T-0119/T-0120.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L48】 | Knowledge Librarian | Clarify GPU availability during testing |
| 5 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Documents current RuntimeHost capabilities, outstanding presentation work, and API surfaces to extend.【F:docs/modules/runtime/README.md†L5-L176】 | Specialist Engineer | Audit API coverage |
| 6 | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Binding decision for stage planner, presentation backend separation, panel registry integration.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】 | Specialist Engineer | Validate implementation plan adherence |
| 7 | (TBD) Reviews | No reviews consulted yet; reference docs/reviews if design ambiguity arises. | Knowledge Librarian | Pending |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None anticipated; focus on CPU mock presenters until GPU backends ready.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】
- Benchmark targets & expected deltas: Monitor runtime telemetry for presentation stages; no new budgets defined yet.
- Tooling updates required: Ensure documentation validator and harness tests include presentation activation scenarios per DoD.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Are GPU backends available in CI to exercise GLFW presenter, or should validation rely on mock presenter only? | Specialist Engineer | Before Phase 3 kickoff | Pending |
| Do tooling teams require panel registry updates in this increment, or can presentation wiring land first? | Agent Orchestrator | Before implementation | Pending |

## 7. Attachments
- Diagrams: None
- Data sets: Reuse existing harness case studies (geometry baseline) once presentation adapters integrated.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】
- Additional notes: Consider capturing screenshots once presenter surfaces exist to feed PM-510 demos.【F:docs/ROADMAP.md†L76-L95】

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
