# Task Brief Template

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 Presentation Stage Telemetry Validation
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../../ROADMAP.md) · [RT-410](../../../backlog/active/RT-410-runtime-stage-planner.md)
- **Primary Goal:** Strengthen the runtime stage planner by validating the `presentation.dispatch` stage’s diagnostics and readiness so RT-410 can exit its testing criterion for automated loop coverage.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/modules/runtime/README.md†L5-L113】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-02-26-rt-410-presentation-telemetry.md`), context package (`agents/context_packages/2026-02-26-rt-410-presentation-telemetry.md`)

## 2. Scope & Boundaries
- In scope: Runtime loop tests that exercise presentation-stage instrumentation under callback and backend permutations, plus minor runtime documentation updates if telemetry contracts move.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】
- Out of scope: Implementing new GPU resource providers or command encoder features tracked under T-0120/T-0119; editor integration work from TL-310.【F:docs/ROADMAP.md†L64-L102】【F:docs/modules/rendering/README.md†L5-L13】
- Architectural considerations / ADRs: Honour ADR-0008 loop configurability and presentation backend contracts when extending diagnostics hooks.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】

## 3. Success Criteria
- Functional: Runtime `presentation.dispatch` stage records execution order and dependency graph entries when callbacks or backends are configured.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- Documentation: Update runtime module README if telemetry semantics change during implementation.【F:docs/modules/runtime/README.md†L5-L113】
- Validation: Execute the standard build + test + docs validation pipeline for the linux-gcc-debug preset and Python suites.【F:README.md†L120-L142】
- Quality gates & benchmarks: Ensure telemetry metrics remain deterministic; no regression in existing runtime tests.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../../../README.md) | Confirms runtime is "At Risk" pending RT-410 completion and lists test workflow expectations.【F:README.md†L13-L142】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../../NAVIGATION.md) | Reiterates ladder order and template usage for task/context capture.【F:docs/NAVIGATION.md†L5-L108】 | Product Manager |
| 3 | [docs/ROADMAP.md](../../../ROADMAP.md) | Highlights Phase 4 priority-1 items (T-0120/T-0119/RT-410) and milestone timeline.【F:docs/ROADMAP.md†L64-L102】 | Product Manager |
| 4 | [RT-410 backlog](../../../backlog/active/RT-410-runtime-stage-planner.md) | Details DoD for stage planner, presentation loop, and testing gaps.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】 | Product Manager |
| 5 | [Runtime module README](../../../modules/runtime/README.md) | Describes outstanding work and current `presentation.dispatch` behaviour.【F:docs/modules/runtime/README.md†L5-L113】 | Product Manager |
| 6 | [ADR-0008](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Documents architectural requirements for loop planning and presentation hooks.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L116】 | Product Manager |
| 7 | [Runtime loop implementation](../../../../engine/runtime/src/api.cpp) | Inspect execution report handling for presentation-stage toggles (implementation owner pending deep dive). | Product Manager |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assigned this session | 1–5 | Align backlog priorities, approve gate exits. | Planned |
| Knowledge Librarian | Assigned this session | 2 & 5 | Maintain context package, archive artefacts. | Planned |
| Specialist Engineer(s) | Runtime contributor | 3 | Implement runtime/tests updates. | Planned |
| Docs/DevRel | Runtime docs reviewer | 2, 4, 5 | Review README/test doc changes. | Planned |
| QA/Test Specialist | Runtime QA | 4 | Review new test coverage and logs. | Planned |
| Performance Engineer | N/A (no perf scope) | 4 | — | N/A |
| Safety Reviewer | N/A (not security impacting) | 4 | — | N/A |
| Reviewer | Runtime reviewer | 4 | Code review + validation of telemetry semantics. | Planned |
| Release Manager | Release coordinator | 5 | Ensure change notes captured in roadmap/task log. | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Roadmap/backlog reviewed; scope agreed. | Task brief + context package drafted. | This brief + context package. |
| 2 – Context Assembly | Context ladder traversed, open questions logged. | Code investigation checklist ready. | Context package §4–6. |
| 3 – Execution & Collaboration | Implementation plan approved. | Runtime/tests patches completed, docs updated. | Git commits, code review notes. |
| 4 – Quality Gates | All tests/validators executed. | CI-equivalent commands green; QA sign-off. | Command log, test outputs. |
| 5 – Release & Documentation Sync | Tests/docs done. | Backlog/task brief updated with outcomes; artefacts archived. | Updated brief/context + PR. |

## 7. Timeline & Milestones
- Kickoff: 2026-02-26
- Implementation window: 2026-02-26 (single session)
- Quality gate window: Immediately post-implementation (same day)
- Release target: 2026-02-26 (merge-ready changes)
- Post-release monitoring: Observe runtime telemetry dashboards during next regression run.

## 8. Known Risks & Dependencies
- Risks: Presentation telemetry changes could regress execution reports relied upon by diagnostics.【F:docs/modules/runtime/README.md†L5-L113】
- Dependencies: Runtime presentation loop depends on GPU backend progress (T-0120/T-0119).【F:docs/ROADMAP.md†L64-L102】
- Mitigations / contingency: Keep changes limited to diagnostics toggles and add regression tests to surface differences quickly.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 9. Communication Plan
- Async updates cadence: Post progress in weekly integration notes (align with PM-510 cadence).【F:docs/ROADMAP.md†L75-L102】
- Live sync triggers: Schedule ad-hoc review if presentation telemetry semantics change.
- Escalation path: Notify Agent Orchestrator for any blockers tied to GPU enablement milestone.【F:docs/ROADMAP.md†L64-L102】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-02-26 | Product Manager | Session scoped to RT-410 presentation telemetry validation. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../../../../agents/TEMPLATES) to keep audit trails coherent.
