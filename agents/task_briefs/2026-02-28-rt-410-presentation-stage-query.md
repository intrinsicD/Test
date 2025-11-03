# Task Brief Template

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 Presentation Stage Activity Query Exposure
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../docs/ROADMAP.md) · [RT-410](../../docs/backlog/active/RT-410-runtime-stage-planner.md)
- **Primary Goal:** Provide a runtime API surface so tooling and scripting can detect when the `presentation.dispatch` stage is active, unblocking RT-410’s synchronisation hook deliverables and milestone cadence.【F:docs/ROADMAP.md†L64-L99】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-02-28-rt-410-presentation-stage-query.md`), context package (`agents/context_packages/2026-02-28-rt-410-presentation-stage-query.md`)

## 2. Scope & Boundaries
- In scope: Add a public `RuntimeHost` accessor (and global helper) that reports whether presentation handlers are active, plus accompanying tests and documentation updates within the runtime module.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/modules/runtime/README.md†L5-L124】
- Out of scope: GPU resource provider/command encoder implementation tracked under T-0120/T-0119, editor enablement, or presentation backend feature work beyond activity introspection.【F:README.md†L15-L105】【F:docs/modules/rendering/README.md†L5-L36】
- Architectural considerations / ADRs: Follow ADR-0008 requirements around exposing synchronisation hooks and runtime configurability while keeping the stage planner declarative.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L129】

## 3. Success Criteria
- Functional: Tooling can query `RuntimeHost` (or the global runtime facade) to determine if presentation callbacks/backends are currently engaged, enabling conditional wiring without inspecting internal diagnostics.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/modules/runtime/README.md†L52-L124】
- Documentation: Update the runtime module README to mention the new query and its relationship to presentation stage telemetry.【F:docs/modules/runtime/README.md†L5-L124】
- Validation: Run the standard linux-gcc-debug configure/build/test pipeline and Python/doc validators per repository workflow expectations.【F:README.md†L120-L144】
- Quality gates & benchmarks: Existing runtime loop/presentation tests remain green; new coverage verifies the accessor under callback/backend permutations.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Confirms runtime module is "At Risk" pending RT-410 work and captures canonical build/test commands.【F:README.md†L15-L144】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Reiterates ladder order, templates, and documentation precedence.【F:docs/NAVIGATION.md†L5-L116】 | Product Manager |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Phase 4 highlights RT-410/T-0120/T-0119 as priority-1 deliverables.【F:docs/ROADMAP.md†L64-L99】 | Product Manager |
| 4 | [RT-410 backlog](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | Definition of done calls for synchronisation hooks/tooling integration to support presentation flow.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】 | Product Manager |
| 5 | [Runtime module README](../../docs/modules/runtime/README.md) | Details presentation stage behaviour, hot-swapping, and outstanding work for synchronisation APIs.【F:docs/modules/runtime/README.md†L5-L124】 | Product Manager |
| 6 | [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Mandates instrumentation and scripting hooks for the declarative loop/presentation system.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L129】 | Product Manager |
| 7 | [Runtime implementation](../../engine/runtime/src/api.cpp) | Verify presentation stage toggles, diagnostics updates, and plan rebuild pathways backing the new accessor. | Product Manager |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assigned this session | 1–5 | Align scope with roadmap priorities, approve gate exits. | Planned |
| Knowledge Librarian | Assigned this session | 2 & 5 | Maintain context package, archive artefacts. | Planned |
| Specialist Engineer(s) | Runtime contributor | 3 | Implement runtime API/tests/docs. | Planned |
| Docs/DevRel | Runtime docs reviewer | 2, 4, 5 | Review README updates, terminology alignment. | Planned |
| QA/Test Specialist | Runtime QA | 4 | Validate new tests + regression suite logs. | Planned |
| Performance Engineer | N/A (no perf impact) | 4 | — | N/A |
| Safety Reviewer | N/A (not security impacting) | 4 | — | N/A |
| Reviewer | Runtime reviewer | 4 | Code review, ensure API semantics fit RT-410 plan. | Planned |
| Release Manager | Release coordinator | 5 | Ensure backlog/roadmap notes reflect new API availability. | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed, backlog alignment confirmed. | Task brief + context package drafted. | This brief + context package. |
| 2 – Context Assembly | Module/ADR references reviewed; implementation questions logged. | Implementation plan recorded in brief/context. | Context package §§2–6. |
| 3 – Execution & Collaboration | Implementation plan approved by reviewer/agent lead. | Runtime API/tests/docs updated with citations. | Git commits, code review notes. |
| 4 – Quality Gates | Build/test/doc validators executed. | Command logs captured, QA sign-off. | Test outputs attached to quality report. |
| 5 – Release & Documentation Sync | QA approvals complete. | Backlog/task brief updated; artefacts archived. | Updated brief/context + PR summary. |

## 7. Timeline & Milestones
- Kickoff: 2026-02-28
- Implementation window: 2026-02-28 (current session)
- Quality gate window: Immediate post-implementation
- Release target: 2026-02-28 (merge-ready change)
- Post-release monitoring: Observe runtime presentation diagnostics in next regression run.

## 8. Known Risks & Dependencies
- Risks: API exposure might diverge from future ADR-0008 scripting bridge design; needs minimal surface to avoid churn.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L62-L129】
- Dependencies: Presentation stage semantics rely on runtime presentation callbacks/backends and RT-410 coordination with rendering.【F:docs/modules/runtime/README.md†L52-L124】【F:docs/ROADMAP.md†L64-L99】
- Mitigations / contingency: Limit change to passive query; add tests ensuring parity with existing diagnostics toggles.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 9. Communication Plan
- Async updates cadence: Record progress in RT-410 integration updates and PM-510 demo notes when API lands.【F:docs/ROADMAP.md†L75-L102】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】
- Live sync triggers: Notify runtime lead if accessor semantics need ADR amendments.
- Escalation path: Raise blockers with Agent Orchestrator to preserve Phase 4 timeline.【F:docs/ROADMAP.md†L64-L109】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-02-28 | Product Manager | Scoped RT-410 increment to expose presentation stage activity query. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.
