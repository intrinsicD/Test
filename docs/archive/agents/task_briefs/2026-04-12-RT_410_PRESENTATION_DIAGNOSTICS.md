# Task Brief — RT-410 Presentation Diagnostics Submission Coverage

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 presentation diagnostics submission coverage
- **Roadmap / Backlog Reference:** [`docs/ROADMAP.md`](../../../ROADMAP.md), [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)【F:docs/ROADMAP.md†L64-L95】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】
- **Primary Goal:** Verify that invoking `RuntimePresentationContext::submit_render_graph` via the presentation stage populates runtime diagnostics with frame-graph serialization, resource events, and GPU usage snapshots required by RT-410’s instrumentation promises.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:engine/runtime/src/api.cpp†L2836-L2856】
- **Linked Workflow Artefacts:** (Task brief, context package, quality report URLs)

## 2. Scope & Boundaries
- In scope: Extend runtime presentation tests to assert diagnostics fields populated after backend submission and document findings in the quality artefacts.【F:engine/runtime/tests/test_module.cpp†L2782-L2839】【F:engine/runtime/src/api.cpp†L2836-L2856】
- Out of scope: Implementing real GPU presenters, modifying frame-graph execution, or altering diagnostics schemas beyond new assertions.【F:docs/modules/rendering/README.md†L5-L13】【F:docs/modules/runtime/README.md†L5-L120】
- Architectural considerations / ADRs: Must adhere to ADR-0008 presentation separation and maintain deterministic loop telemetry.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L118】

## 3. Success Criteria
- Functional: Presentation-stage tests confirm diagnostics include serialized frame graph, resource events, and non-default GPU usage after `submit_render_graph` executes.【F:engine/runtime/src/api.cpp†L2836-L2856】
- Documentation: Update context artefacts; no module README changes expected unless diagnostics behaviour shifts.【F:docs/modules/runtime/README.md†L5-L256】
- Validation: Standard build, test, and docs validation commands executed with passing results.【F:README.md†L120-L142】
- Quality gates & benchmarks: Maintain existing telemetry baselines; no new benchmarks introduced.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Runtime remains “At Risk” pending RT-410; canonical validation commands recorded.【F:README.md†L15-L29】【F:README.md†L120-L142】 | Product Manager |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Confirms precedence rules and template usage for agents artefacts.【F:docs/NAVIGATION.md†L5-L43】 | Product Manager |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 prioritises RT-410 alongside GPU milestones; milestone timeline expects presentation coverage.【F:docs/ROADMAP.md†L64-L83】 | Agent Orchestrator |
| 4 | [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD mandates presentation diagnostics/tests; synchronisation hooks outstanding.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L48】 | Specialist Engineer |
| 5 | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Documents presentation stage semantics and diagnostics responsibilities.【F:docs/modules/runtime/README.md†L5-L256】 | Specialist Engineer |
| 6 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Binding contract for stage planner and presentation separation.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L118】 | Specialist Engineer |
| 7 | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Rendering blocked; rely on mock/recording pipelines for validation.【F:docs/modules/rendering/README.md†L5-L13】 | Knowledge Librarian |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | — | 1–5 | Coordinate phases, approve exits | Pending |
| Knowledge Librarian | — | 2 & 5 | Context package, archive hand-off | Pending |
| Specialist Engineer(s) | — | 3 | Extend runtime tests and adjust diagnostics assertions | Pending |
| Docs/DevRel | — | 2, 4, 5 | Review documentation impact, ensure citations | Pending |
| QA/Test Specialist | — | 4 | Execute validation suite, confirm deterministic outputs | Pending |
| Performance Engineer | — | 4 | Monitor telemetry for regressions | Pending |
| Safety Reviewer | — | 4 | Confirm no new unsafe hooks introduced | Pending |
| Reviewer | — | 4 | Code review for runtime/tests changes | Pending |
| Release Manager | — | 5 | Summarise change, prep release notes | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | RT-410 increment identified; task brief drafted. | Brief approved by Agent Orchestrator. | This document. |
| 2 – Context Assembly | Knowledge Librarian assembles ladder per templates. | Context package reviewed, open questions assigned. | `agents/context_packages/2026-04-12-RT_410_PRESENTATION_DIAGNOSTICS.md` |
| 3 – Execution & Collaboration | Implementation plan agreed; tests updated. | Code changes and tests ready for validation. | Commit diff, inline notes. |
| 4 – Quality Gates | Implementation complete. | Canonical validation block passes; QA/Performance sign-off. | Quality report (TBD). |
| 5 – Release & Documentation Sync | Quality gates cleared. | PR merged with docs/backlog sync as needed. | PR description, updated artefacts. |

## 7. Timeline & Milestones
- Kickoff: 2026-04-12
- Implementation window: 2026-04-12 → 2026-04-13
- Quality gate window: 2026-04-13
- Release target: 2026-04-13
- Post-release monitoring: Observe PM-510 telemetry snapshots for presentation diagnostics stability.【F:docs/ROADMAP.md†L76-L83】

## 8. Known Risks & Dependencies
- Risks: Diagnostics expectations might shift; failing to reset recording fixtures could flake tests.【F:engine/runtime/tests/test_module.cpp†L2782-L2839】
- Dependencies: Recording providers and mock backends remain the only safe path while GPU backends blocked.【F:docs/modules/rendering/README.md†L5-L13】
- Mitigations / contingency: Use existing recording resource providers and ensure deterministic asset handles in tests.【F:engine/runtime/tests/test_module.cpp†L2786-L2835】

## 9. Communication Plan
- Async updates cadence: Daily notes in task brief decision log until release.
- Live sync triggers: Escalate to Agent Orchestrator if diagnostics expectations change or build fails.
- Escalation path: Rendering/runtime leads for RT-410 scope alignment.【F:docs/ROADMAP.md†L64-L95】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](.) to keep audit trails coherent.
