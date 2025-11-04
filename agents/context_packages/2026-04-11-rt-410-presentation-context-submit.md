# Context Package — RT-410 Presentation Context Submission Coverage

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-04-11-rt-410-presentation-context-submit.md`](../task_briefs/2026-04-11-rt-410-presentation-context-submit.md)【F:docs/ROADMAP.md†L64-L95】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- **Backlog Entry:** [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md)【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md)【F:docs/ROADMAP.md†L64-L95】
- **Workflow Phase:** Phase 2 — Context Assembly (handoff to Specialist Engineer)

## 2. Problem Summary
- Current behaviour: Presentation stage invokes backends and sets `submit_render_graph`, yet runtime tests only assert callback presence without exercising the submission hook.【F:engine/runtime/src/api.cpp†L2158-L2186】【F:engine/runtime/tests/test_module.cpp†L2521-L2689】
- Desired behaviour: Add a mock-backend test that captures the context, calls `submit_render_graph`, and proves the runtime render pipeline runs with recording fixtures, satisfying RT-410’s automated coverage requirement prior to GPU backend completion.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L118】
- Constraints / invariants: Respect ADR-0008 contracts, avoid GPU provider assumptions noted in rendering README, keep diagnostics stable, and leverage existing asset validators to prevent flaky fixtures.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】【F:docs/modules/rendering/README.md†L1-L31】【F:engine/runtime/tests/test_module.cpp†L276-L360】
- Quality budgets / telemetry notes: No new telemetry; standard build/test/doc command block remains mandatory.【F:README.md†L120-L142】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Backlog | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | DoD demands automated presentation tests. | 4 |
| Module README | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Details presentation stage semantics and outstanding work. | 5 |
| ADR / Spec | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Defines presentation backend contract and loop planner separation. | 6 |
| Module README | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Notes GPU provider gap; emphasises reliance on mock coverage. | 7 |
| Code excerpt | [`engine/runtime/src/api.cpp`](../../engine/runtime/src/api.cpp) | Presentation stage wiring provides `submit_render_graph`. | — |
| Code excerpt | [`engine/runtime/tests/test_module.cpp`](../../engine/runtime/tests/test_module.cpp) | Existing presentation tests and recording fixtures. | — |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Runtime marked “At Risk”; canonical validation commands recorded.【F:README.md†L26-L29】【F:README.md†L120-L142】 | Knowledge Librarian | None |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Confirms precedence and template requirements for task artefacts.【F:docs/NAVIGATION.md†L5-L114】 | Knowledge Librarian | None |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 emphasises RT-410 alongside GPU milestones.【F:docs/ROADMAP.md†L64-L95】 | Agent Orchestrator | Monitor milestone notes |
| 4 | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | DoD requires automated presentation-loop tests; dependencies recorded.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L48】 | Specialist Engineer | Implement coverage |
| 5 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Documents presentation stage and diagnostics expectations.【F:docs/modules/runtime/README.md†L5-L176】 | Specialist Engineer | Align assertions |
| 6 | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Binding decision for declarative loop and presentation separation.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】 | Specialist Engineer | Ensure compliance |
| 7 | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Rendering blocked; rely on mock backends for validation.【F:docs/modules/rendering/README.md†L1-L31】 | Knowledge Librarian | Avoid GPU dependencies |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None; mock backend test runs CPU-only.
- Benchmark targets & expected deltas: No performance benchmarks expected; diagnostics serialization should remain stable.
- Tooling updates required: None beyond recording results in the quality report.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Will calling `submit_render_graph` require GPU dependencies? | Specialist Engineer | During implementation | Use recording GPU provider + Vulkan scheduler stubs already employed by integration tests.【F:engine/rendering/resources/recording_gpu_resource_provider.hpp†L17-L60】【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L42-L150】 |
| Do we need additional documentation updates? | Docs/DevRel | Post-validation | Behaviour unchanged; document in quality report only. |

## 7. Attachments
- Diagrams: None.
- Data sets: Reuse existing runtime fixture handles; no new assets required.【F:engine/runtime/tests/test_module.cpp†L947-L980】
- Additional notes: Ensure new test copies handle-binding pattern to satisfy validators and avoid spurious failures.【F:engine/runtime/tests/test_module.cpp†L947-L980】

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
