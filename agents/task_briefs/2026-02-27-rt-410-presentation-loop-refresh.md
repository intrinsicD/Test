# Task Brief Template

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 Presentation Loop Plan Refresh Hooks
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../docs/ROADMAP.md) · [RT-410](../../docs/backlog/active/RT-410-runtime-stage-planner.md)
- **Primary Goal:** Ensure `RuntimeHost` rebuilds its default loop plan when presentation callbacks or backends are attached after initialization so the `presentation.dispatch` stage records diagnostics as mandated by RT-410’s synchronisation deliverables.【F:docs/ROADMAP.md†L64-L90】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】【F:engine/runtime/src/api.cpp†L1913-L2168】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-02-27-rt-410-presentation-loop-refresh.md`), context package (`agents/context_packages/2026-02-27-rt-410-presentation-loop-refresh.md`)

## 2. Scope & Boundaries
- In scope: Runtime loop plan rebuild triggers tied to presentation callback/backend toggles, associated tests, and runtime documentation updates covering the new hook.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/modules/runtime/README.md†L5-L113】
- Out of scope: Implementing GPU command encoder/provider functionality tracked under T-0119/T-0120 or editor integration sequenced behind RT-410.【F:docs/ROADMAP.md†L64-L82】【F:docs/backlog/active/T-0119-command-encoder-integration.md†L1-L38】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L1-L37】
- Architectural considerations / ADRs: Honour ADR-0008 requirements for configurable loop stages and presentation backend contracts when modifying runtime APIs.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L118】

## 3. Success Criteria
- Functional: Registering or removing presentation callbacks/backends after initialization updates execution reports/diagnostics to include or exclude `presentation.dispatch` deterministically.【F:engine/runtime/src/api.cpp†L1913-L2168】
- Documentation: Update the runtime module README to describe runtime presentation backend/callback hot-swapping behaviour.【F:docs/modules/runtime/README.md†L5-L120】
- Validation: Execute the standard linux-gcc-debug configure/build/test flow plus Python test suites and docs validator.【F:README.md†L120-L144】
- Quality gates & benchmarks: Existing runtime tests remain green; new coverage demonstrates loop-plan rebuild behaviour without altering performance budgets.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Establishes module risk levels (runtime “At Risk”) and required validation workflow.【F:README.md†L13-L144】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Confirms precedence chain and template usage for agents.【F:docs/NAVIGATION.md†L5-L116】 | Product Manager |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Phase 4 highlights RT-410/T-0119/T-0120 as priority-1 initiatives requiring presentation readiness.【F:docs/ROADMAP.md†L64-L102】 | Product Manager |
| 4 | [RT-410 backlog](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | Details DoD for presentation loop integration and telemetry coverage.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】 | Product Manager |
| 5 | [Runtime module README](../../docs/modules/runtime/README.md) | Documents presentation dispatch expectations and outstanding work.【F:docs/modules/runtime/README.md†L5-L120】 | Product Manager |
| 6 | [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Specifies declarative loop/planner + presentation backend architecture that changes must respect.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L118】 | Product Manager |
| 7 | [Runtime loop implementation](../../engine/runtime/src/api.cpp) | Verify current loop-plan build logic and callback handling; confirm rebuild hooks required.【F:engine/runtime/src/api.cpp†L1913-L2187】 | Product Manager |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assigned this session | 1–5 | Coordinate phases, approve exits | Planned |
| Knowledge Librarian | Assigned this session | 2 & 5 | Context package, archive hand-off | Planned |
| Specialist Engineer(s) | Runtime contributor | 3 | Implementation, test updates | Planned |
| Docs/DevRel | Runtime docs reviewer | 2, 4, 5 | Documentation updates, terminology review | Planned |
| QA/Test Specialist | Runtime QA | 4 | Validation suite | Planned |
| Performance Engineer | N/A (no perf scope) | 4 | Benchmarks | N/A |
| Safety Reviewer | N/A (not security impacting) | 4 | Security & safety | N/A |
| Reviewer | Runtime reviewer | 4 | Code review | Planned |
| Release Manager | Release coordinator | 5 | Release prep | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed; backlog alignment confirmed. | Task brief + context package drafted. | This brief + linked context package. |
| 2 – Context Assembly | Knowledge Librarian captures artefacts/questions. | Implementation checklist agreed. | Context package Sections 2–6. |
| 3 – Execution & Collaboration | Implementation plan approved. | Runtime/tests/docs patches authored. | Git commits, inline notes. |
| 4 – Quality Gates | Build + test commands executed. | Logs show linux-gcc-debug + Python + docs validator passing. | Command transcripts. |
| 5 – Release & Documentation Sync | Docs/tests updated. | Brief/backlog updated, artefacts archived. | Updated brief/context, PR notes. |

## 7. Timeline & Milestones
- Kickoff: 2026-02-27
- Implementation window: 2026-02-27 (single session)
- Quality gate window: Same-day post-implementation
- Release target: 2026-02-27
- Post-release monitoring: Observe runtime diagnostics regression suite in next scheduled run.

## 8. Known Risks & Dependencies
- Risks: Loop plan rebuild could introduce regressions in runtime execution ordering or diagnostics reporting.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- Dependencies: GPU milestone tasks (T-0119/T-0120) rely on presentation readiness; ensure behaviour remains compatible.【F:docs/ROADMAP.md†L64-L102】【F:docs/backlog/active/T-0119-command-encoder-integration.md†L1-L38】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L1-L37】
- Mitigations / contingency: Keep rebuild scoped to default plan regeneration with tests validating execution reports, enabling quick rollback if diagnostics differ unexpectedly.【F:engine/runtime/src/api.cpp†L1913-L2168】

## 9. Communication Plan
- Async updates cadence: Post findings alongside PM-510 weekly integration demo notes to keep stakeholders aligned.【F:docs/ROADMAP.md†L75-L102】
- Live sync triggers: Schedule ad-hoc sync if rebuild impacts presentation telemetry consumers.
- Escalation path: Escalate blockers to Agent Orchestrator coordinating Phase 4 milestones.【F:docs/ROADMAP.md†L64-L102】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-02-27 | Product Manager | Session scoped to RT-410 presentation loop rebuild triggers. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.
