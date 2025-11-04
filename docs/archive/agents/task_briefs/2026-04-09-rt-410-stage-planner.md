# Task Brief — RT-410 Stage Planner Presentation Integration

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 Stage Planner Presentation Integration
- **Roadmap / Backlog Reference:** [`docs/ROADMAP.md`](../../../ROADMAP.md), [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../../backlog/active/RT-410-runtime-stage-planner.md)
- **Primary Goal:** Deliver the remaining presentation loop plumbing mandated by ADR-0008 so `RuntimeHost` can drive presentation backends in lockstep with GPU enablement milestones.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L38】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L115】
- **Linked Workflow Artefacts:** Task brief (this document), context package ([`2026-04-09-rt-410-stage-planner.md`](../context_packages/2026-04-09-rt-410-stage-planner.md)), quality report (pending)

## 2. Scope & Boundaries
- In scope:
  - Runtime stage planner integration work that wires presentation adapters, synchronisation hooks, and configuration surfaces needed for GPU demos.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L38】
  - Documentation updates for the runtime module once functionality lands.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- Out of scope:
  - Rendering command encoder and GPU resource provider deliverables tracked under T-0119/T-0120.【F:docs/ROADMAP.md†L67-L83】
  - Editor tooling enablement covered by TL-310.【F:docs/ROADMAP.md†L70-L83】
- Architectural considerations / ADRs:
  - [`ADR-0008`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) governs stage planner design, presentation backend contracts, and tooling integration requirements.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】

## 3. Success Criteria
- Functional:
  - Runtime stage planner exposes presentation adapter selection aligned with ADR-0008, enabling mock/GLFW presenters and synchronisation hooks referenced in RT-410 definition of done.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L115】
- Documentation:
  - Update runtime module README and root roadmap snapshot to reflect presentation support when implemented.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:README.md†L26-L30】
- Validation:
  - Extend harness/runtime coverage for presentation activation under mock and GLFW backends per RT-410 acceptance criteria.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- Quality gates & benchmarks:
  - Run standard build/test command block defined in AGENTS workflow to keep automation reproducible.【F:README.md†L112-L144】【F:AGENTS.md†L93-L115】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Runtime module flagged "At Risk" pending RT-410 presentation work; roadmap alignment required.【F:README.md†L26-L29】【F:README.md†L98-L107】 | Product Manager |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Confirms precedence, module docs, ADR references required for runtime work.【F:docs/NAVIGATION.md†L11-L20】【F:docs/NAVIGATION.md†L21-L43】 | Knowledge Librarian |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 assigns RT-410 priority 1 with stage planner milestone scheduled for 2026-03-05.【F:docs/ROADMAP.md†L64-L83】 | Agent Orchestrator |
| 4 | [`docs/backlog/active/RT-410-runtime-stage-planner.md`](../../../backlog/active/RT-410-runtime-stage-planner.md) | Defines goal, DoD, dependencies; clarifies presentation adapters and tests outstanding.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L48】 | Specialist Engineer |
| 5 | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Notes RuntimeHost exposes loop plan/presentation hooks but lacks GPU-backed presenters until RT-410 ships.【F:docs/modules/runtime/README.md†L5-L121】 | Specialist Engineer |
| 6 | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Mandates declarative loop, presentation backends, panel registry; guides implementation scope.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L130】 | Specialist Engineer |
| 7 | (TBD) Historical reviews | No additional reviews consulted yet; revisit if design ambiguity arises. | Knowledge Librarian |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | AI Agent | 1–5 | Coordinate phases, approve exits | Active |
| Knowledge Librarian | AI Agent | 2 & 5 | Context package, archive hand-off | Active |
| Specialist Engineer(s) | AI Agent | 3 | Implementation, test updates | Active |
| Docs/DevRel | AI Agent | 2, 4, 5 | Documentation updates, terminology review | Pending |
| QA/Test Specialist | AI Agent | 4 | Validation suite | Pending |
| Performance Engineer | (TBD) | 4 | Benchmarks | Pending |
| Safety Reviewer | (TBD) | 4 | Security & safety | Pending |
| Reviewer | (TBD) | 4 | Code review | Pending |
| Release Manager | AI Agent | 5 | Release prep | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Roadmap/backlog reviewed; task brief drafted. | Summary/Scope approved internally. | This brief section 1–3. |
| 2 – Context Assembly | Context ladder traversed; context package drafted. | Context package reviewed by specialist engineer role. | [`2026-04-09-rt-410-stage-planner.md`](../context_packages/2026-04-09-rt-410-stage-planner.md). |
| 3 – Execution & Collaboration | Implementation plan recorded in task brief. | Feature/bugfix implemented; code ready for validation. | Commit history, decision log. |
| 4 – Quality Gates | Implementation complete; tests planned. | Standard build/tests pass; documentation updated. | Quality report (pending). |
| 5 – Release & Documentation Sync | Quality report approved. | Docs synced; PR prepared. | PR summary, roadmap/doc updates. |

## 7. Timeline & Milestones
- Kickoff: 2026-04-09 (current session)
- Implementation window: 2026-04-09 → 2026-04-09
- Quality gate window: 2026-04-09
- Release target: Immediate upon validation
- Post-release monitoring: Monitor runtime presentation telemetry via existing harness runs

## 8. Known Risks & Dependencies
- Risks:
  - Presentation adapter integration may depend on GPU backend readiness; risk of blocked testing in absence of GPU resources.【F:docs/ROADMAP.md†L64-L83】
  - Synchronisation hooks could affect existing diagnostics; regression risk if not validated with harness.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】
- Dependencies:
  - T-0119/T-0120 GPU milestones for backend submission compatibility.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L39-L45】
- Mitigations / contingency:
  - Focus initial work on mock presenter integration and configuration surfaces to unblock runtime/tooling progress even before GPU submission completes.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】

## 9. Communication Plan
- Async updates cadence: Daily notes in task brief decision log when progress occurs.
- Live sync triggers: Trigger live sync if presentation adapter API conflicts with GPU milestones or ADR guidance.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L45-L115】
- Escalation path: Escalate blockers to Agent Orchestrator per AGENTS workflow.【F:AGENTS.md†L116-L159】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-04-09 | AI Agent | Session kickoff; task brief + context package initiated. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](.) to keep audit trails coherent.
