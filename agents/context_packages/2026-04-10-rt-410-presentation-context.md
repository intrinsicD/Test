# Context Package — RT-410 Mock Presentation Context Validation

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-04-10-rt-410-presentation-context.md`](../task_briefs/2026-04-10-rt-410-presentation-context.md)【F:docs/ROADMAP.md†L64-L95】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】
- **Backlog Entry:** [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md)【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md)【F:docs/ROADMAP.md†L64-L95】
- **Workflow Phase:** Phase 2 – Context Assembly (handoff to Specialist Engineer for implementation)

## 2. Problem Summary
- Current behaviour: Runtime presentation stage invokes mock backends but lacks direct unit coverage asserting that the supplied `RuntimePresentationContext` exposes the `submit_render_graph` hook, even though the stage wires it when a backend is present.【F:engine/runtime/src/api.cpp†L2158-L2178】
- Desired behaviour: Add deterministic coverage (using the mock backend) verifying that RT-410’s presentation loop guarantees a populated context so tooling can rely on automated tests before GPU backends are ready.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L118】
- Constraints / invariants: Must respect ADR-0008 presentation contracts, avoid exercising incomplete GPU providers noted in rendering README, and keep runtime diagnostics stable.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L118】【F:docs/modules/rendering/README.md†L5-L13】【F:docs/modules/runtime/README.md†L5-L176】
- Quality budgets / telemetry notes: No new telemetry; existing runtime diagnostics already capture frame-graph metadata. Standard build/test block required for acceptance.【F:README.md†L112-L144】【F:docs/modules/runtime/README.md†L117-L176】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Backlog | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | Defines DoD including automated presentation loop tests. | 4 |
| Module README | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Documents presentation stage semantics, diagnostics expectations. | 5 |
| Module README | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Highlights coordination needs with rendering backends. | 7 |
| ADR / Spec | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Governs stage planner and presentation backend contract. | 6 |
| Code excerpt | [`engine/runtime/src/api.cpp`](../../engine/runtime/src/api.cpp) | Presentation stage wiring sets `submit_render_graph` for backends. | — |
| Code excerpt | [`engine/rendering/include/engine/rendering/backend/mock/presentation_backend.hpp`](../../engine/rendering/include/engine/rendering/backend/mock/presentation_backend.hpp) | Mock backend captures invocation metadata and forwards context to callbacks. | — |
| Existing tests | [`engine/runtime/tests/test_module.cpp`](../../engine/runtime/tests/test_module.cpp) | Provides asset validator scaffolding and render submission tests to mirror. | — |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Confirms runtime module is “At Risk” pending RT-410, and reiterates canonical validation commands. | Knowledge Librarian | None |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Establishes documentation precedence; ensure module docs updated only if behaviour changes. | Knowledge Librarian | Monitor for doc impacts |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | RT-410 priority 1 milestone emphasises presentation readiness. | Agent Orchestrator | Track milestone notes |
| 4 | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | DoD requires automated presentation-loop tests covering mock backend. | Specialist Engineer | Implement unit coverage |
| 5 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Presentation stage toggles and diagnostics details inform assertions. | Specialist Engineer | Ensure tests match documented behaviour |
| 6 | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Specifies separation of submission and presentation along with scripting hooks—tests must verify runtime honours contract. | Specialist Engineer | Validate context surfaces callback |
| 7 | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Rendering backlog still blocked; reinforce reliance on mock backend for tests. | Knowledge Librarian | Avoid GPU dependencies |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None; mock backend test is CPU-only.
- Benchmark targets & expected deltas: No benchmark deltas anticipated; watch runtime diagnostics serialization for regressions.
- Tooling updates required: None beyond documenting results in quality report.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Can mock backend callback capture `RuntimePresentationContext` without extra asset fixtures? | Specialist Engineer | During implementation | Use existing asset validator helpers from `test_module.cpp` to initialise runtime state before tick.【F:engine/runtime/tests/test_module.cpp†L969-L1133】 |
| Do we need documentation updates? | Docs/DevRel | Post-validation | Only update module docs if behaviour changes; expected to remain informational. |

## 7. Attachments
- Diagrams: Not applicable.
- Data sets: Not required.
- Additional notes: Ensure new test isolates presentation stage behaviour to prevent flakiness when GPU backends later integrate.

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
