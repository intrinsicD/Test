# Task Brief — RT-410 Mock Presentation Context Validation

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 Mock Presentation Context Validation
- **Roadmap / Backlog Reference:** [`docs/ROADMAP.md`](../../../ROADMAP.md), [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)【F:docs/ROADMAP.md†L64-L95】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】
- **Primary Goal:** Verify that the runtime presentation stage feeds mock backends a populated `RuntimePresentationContext`, advancing RT-410’s automated test coverage requirement without waiting for GPU backends.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L36】【F:engine/runtime/src/api.cpp†L2158-L2186】
- **Linked Workflow Artefacts:** Task brief (this document); context package ([`2026-04-10-RT_410_PRESENTATION_CONTEXT.md`](../context_packages/2026-04-10-RT_410_PRESENTATION_CONTEXT.md)); quality report (pending)

## 2. Scope & Boundaries
- In scope:
  - Add runtime unit coverage that exercises the `presentation.dispatch` stage with a mock backend and asserts the emitted context exposes `submit_render_graph` and host references.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L36】【F:engine/runtime/src/api.cpp†L2158-L2178】【F:engine/rendering/include/engine/rendering/backend/mock/presentation_backend.hpp†L8-L36】
  - Document findings in the context artefacts and quality report per workflow expectations.【F:docs/NAVIGATION.md†L11-L43】【F:README.md†L112-L144】
- Out of scope:
  - Implementing GPU-backed presentation backends or command encoder/resource provider functionality tracked under T-0119/T-0120.【F:docs/modules/rendering/README.md†L5-L13】【F:docs/ROADMAP.md†L64-L90】
- Architectural considerations / ADRs:
  - Honour ADR-0008’s presentation contract and stage planner guidance when designing tests and assertions.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L116】

## 3. Success Criteria
- Functional:
  - Runtime tests confirm that when a `MockPresentationBackend` is registered, the presentation stage triggers exactly once per tick and supplies a non-null `submit_render_graph` callback referencing the active host.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L36】【F:engine/runtime/src/api.cpp†L2158-L2178】【F:engine/rendering/include/engine/rendering/backend/mock/presentation_backend.hpp†L8-L36】
- Documentation:
  - Task brief, context package, and quality report capture scope, context ladder trace, and validation evidence for auditability.【F:README.md†L5-L30】【F:docs/NAVIGATION.md†L11-L43】
- Validation:
  - Standard build/test command block passes after the new coverage lands.【F:README.md†L112-L144】【F:AGENTS.md†L93-L115】
- Quality gates & benchmarks:
  - No performance benchmarks required; ensure runtime diagnostics remain stable after the additional test coverage.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:docs/modules/runtime/README.md†L117-L176】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Rendering blocked; runtime at risk pending RT-410 delivery; records canonical validation commands.【F:README.md†L15-L144】 | Product Manager |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Confirms precedence ordering and requirement to update module docs when behaviour changes.【F:docs/NAVIGATION.md†L5-L114】 | Knowledge Librarian |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 lists RT-410 as priority 1 with milestone targeting presentation readiness.【F:docs/ROADMAP.md†L64-L95】 | Agent Orchestrator |
| 4 | [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | Definition of done mandates automated tests for presentation loop across mock backends.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】 | Specialist Engineer |
| 5 | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Documents presentation stage activation semantics and diagnostics responsibilities.【F:docs/modules/runtime/README.md†L5-L176】 | Specialist Engineer |
| 6 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Specifies declarative loop planner and presentation backend abstraction that tests must respect.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L118】 | Specialist Engineer |
| 7 | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Highlights coordination expectations between rendering and runtime teams for presentation support.【F:docs/modules/rendering/README.md†L5-L13】 | Knowledge Librarian |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | AI Agent | 1–5 | Coordinate phases, approve exits | Active |
| Knowledge Librarian | AI Agent | 2 & 5 | Context package, archive hand-off | Active |
| Specialist Engineer(s) | AI Agent | 3 | Implementation, test updates | Active |
| Docs/DevRel | AI Agent | 2, 4, 5 | Documentation updates, terminology review | Pending |
| QA/Test Specialist | AI Agent | 4 | Validation suite | Pending |
| Performance Engineer | (TBD) | 4 | Benchmarks | Not required |
| Safety Reviewer | (TBD) | 4 | Security & safety | Not applicable |
| Reviewer | (TBD) | 4 | Code review | Pending |
| Release Manager | AI Agent | 5 | Release prep | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Roadmap/backlog reviewed; task brief drafted. | Summary and scope approved internally. | Sections 1–3. |
| 2 – Context Assembly | Context ladder traversed; context package drafted. | Context package reviewed by specialist engineer. | [`2026-04-10-RT_410_PRESENTATION_CONTEXT.md`](../context_packages/2026-04-10-RT_410_PRESENTATION_CONTEXT.md). |
| 3 – Execution & Collaboration | Implementation plan logged. | Tests implemented and code ready for validation. | Commit history, decision log. |
| 4 – Quality Gates | Implementation complete; validation planned. | Standard build/tests recorded in quality report. | Quality report (pending). |
| 5 – Release & Documentation Sync | Quality report approved. | PR prepared with synced docs/tests. | PR summary + doc updates. |

## 7. Timeline & Milestones
- Kickoff: 2026-04-10 (current session)
- Implementation window: 2026-04-10
- Quality gate window: 2026-04-10
- Release target: Immediate post-validation
- Post-release monitoring: Watch runtime diagnostics telemetry for regressions in presentation stage metrics

## 8. Known Risks & Dependencies
- Risks:
  - Mock backend tests may require asset handle scaffolding similar to existing runtime tests; mistakes could destabilize shared fixtures.【F:engine/runtime/tests/test_module.cpp†L969-L1133】
  - Assertions must avoid depending on GPU backends still blocked by T-0120/T-0119.【F:docs/modules/rendering/README.md†L5-L13】
- Dependencies:
  - Runtime presentation stage implementation in `engine/runtime/src/api.cpp` and mock backend support in rendering module.【F:engine/runtime/src/api.cpp†L2158-L2186】【F:engine/rendering/include/engine/rendering/backend/mock/presentation_backend.hpp†L8-L36】
- Mitigations / contingency:
  - Reuse existing mock backend infrastructure and asset validators from runtime tests to keep coverage deterministic; fall back to presentation callback path if backend wiring blocks compilation.【F:engine/runtime/tests/test_module.cpp†L969-L1133】【F:engine/rendering/include/engine/rendering/backend/mock/presentation_backend.hpp†L8-L36】

## 9. Communication Plan
- Async updates cadence: Note progress in the decision log after each workflow phase change.
- Live sync triggers: Escalate to Agent Orchestrator if mock backend coverage surfaces gaps contradicting ADR-0008 or requires runtime API changes.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L118】
- Escalation path: Follow AGENTS blueprint—Knowledge Librarian for missing context, Agent Orchestrator for scope conflicts.【F:AGENTS.md†L116-L159】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-04-10 | AI Agent | Kickoff: task brief + context package initiated for RT-410 mock presentation validation. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](.) to keep audit trails coherent.
