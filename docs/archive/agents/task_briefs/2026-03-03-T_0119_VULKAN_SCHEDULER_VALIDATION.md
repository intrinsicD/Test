# Task Brief Template

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** T-0119 Vulkan Scheduler Submission Validation
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../../ROADMAP.md) · [T-0119](../../../backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md)
- **Primary Goal:** Harden the Vulkan scheduler path so command encoder recordings surface as submission payloads with synchronisation metadata, delivering the scheduler wiring demanded by the T-0119 definition of done.【F:docs/ROADMAP.md†L64-L112】【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L31-L37】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-03-03-T_0119_VULKAN_SCHEDULER_VALIDATION.md`), context package (`agents/context_packages/2026-03-03-T_0119_VULKAN_SCHEDULER_VALIDATION.md`)

## 2. Scope & Boundaries
- In scope: Improve Vulkan queue selection heuristics for frame-graph passes, verify scheduler submissions capture recorded commands, barriers, waits, signals, and fence data, and add unit coverage mirroring the OpenGL precedent.【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L1-L81】【F:engine/rendering/include/engine/rendering/backend/opengl/gpu_scheduler.hpp†L1-L237】
- Out of scope: Real GPU execution, shader/material binding, or presentation backends tracked by T-0120/RT-410 remain deferred.【F:docs/modules/rendering/README.md†L1-L13】【F:docs/ROADMAP.md†L64-L112】
- Architectural considerations / ADRs: Honour the frame-graph contract and runtime/presentation split from ADR-0003 and ADR-0008 while adjusting scheduler behaviour.【F:docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md†L1-L24】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L119】

## 3. Success Criteria
- Functional: Vulkan scheduler submissions expose recorded draw/dispatch commands plus synchronisation metadata so downstream presentation hooks can consume them consistently.【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L31-L37】【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L23-L76】
- Documentation: No new documentation planned; existing rendering README already highlights the remaining GPU execution blockers.【F:docs/modules/rendering/README.md†L1-L13】
- Validation: Execute the canonical configure/build/test/doc-validation workflow after implementation per repository guidance.【F:README.md†L120-L144】
- Quality gates & benchmarks: Extend rendering unit coverage; performance benchmarks remain out of scope until real GPU execution lands.【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L31-L37】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../../../README.md) | Rendering module flagged blocked pending T-0119/T-0120 and lists canonical validation commands.【F:README.md†L15-L144】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../../NAVIGATION.md) | Confirms backlog/ADR precedence when touching rendering assets.【F:docs/NAVIGATION.md†L5-L113】 | Knowledge Librarian |
| 3 | [docs/ROADMAP.md](../../../ROADMAP.md) | Phase 4 milestone pairs T-0119 with GPU resource provider and runtime presentation work.【F:docs/ROADMAP.md†L64-L112】 | Product Manager |
| 4 | [T-0119 backlog](../../../backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md) | DoD demands encoder APIs wired into schedulers with telemetry and tests.【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L31-L37】 | Product Manager |
| 5 | [Rendering module README](../../../modules/rendering/README.md) | Highlights missing command encoder/provider work as top blockers.【F:docs/modules/rendering/README.md†L1-L13】 | Specialist Engineer |
| 6 | [ADR-0003](../../../specs/ADR_0003_RUNTIME_FRAME_GRAPH.md) & [ADR-0008](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Frame-graph interface and runtime loop contracts constrain scheduler integration choices.【F:docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md†L1-L24】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L119】 | Chief Architect |
| 7 | [Rendering progress update](../../../modules/rendering/PROGRESS_2025_10_27.md) | Reinforces T-0120/T-0119 as critical path and cites scheduler readiness needs.【F:docs/modules/rendering/PROGRESS_2025_10_27.md†L40-L102】 | Knowledge Librarian |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Session lead | 1–5 | Coordinate phases, approve exits | In Progress |
| Knowledge Librarian | Session lead | 2 & 5 | Assemble context package, archive artefacts | In Progress |
| Specialist Engineer(s) | Rendering contributor | 3 | Implement scheduler heuristics/tests | In Progress |
| Docs/DevRel | Rendering docs reviewer | 2, 4, 5 | Confirm documentation alignment | Planned |
| QA/Test Specialist | Rendering QA | 4 | Execute build/test/doc validators | Planned |
| Performance Engineer | N/A this increment | 4 | — | N/A |
| Safety Reviewer | Rendering reviewer | 4 | Review for unsafe API changes | Planned |
| Reviewer | Rendering maintainer | 4 | Code review of scheduler/test updates | Planned |
| Release Manager | Release coordinator | 5 | Ensure roadmap/backlog sync recorded | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed; backlog alignment captured. | Task brief approved. | This brief. |
| 2 – Context Assembly | Module/ADR review complete. | Context package populated with artefacts/risks. | Context package §§2–6. |
| 3 – Execution & Collaboration | Implementation plan confirmed. | Scheduler/tests ready for validation. | Git history + inline notes. |
| 4 – Quality Gates | Build/test/doc commands executed. | Logs captured for QA report. | Quality report outputs. |
| 5 – Release & Documentation Sync | QA sign-off complete. | Backlog/roadmap/doc updates recorded if needed. | Linked documentation updates. |

## 7. Timeline & Milestones
- Kickoff: 2026-03-03
- Implementation window: 2026-03-03 (current session)
- Quality gate window: Immediately post-implementation
- Release target: 2026-03-03
- Post-release monitoring: Verify scheduler payloads during PM-510 GPU demos once execution paths go live.【F:docs/ROADMAP.md†L78-L112】【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L1-L41】

## 8. Known Risks & Dependencies
- Risks: Queue heuristics might misclassify passes, starving compute/transfer queues or breaking telemetry expectations.【F:docs/modules/rendering/README.md†L1-L13】
- Dependencies: Relies on Vulkan GPU resource provider stubs (T-0120) for command buffer access and synchronization handles.【F:engine/rendering/include/engine/rendering/backend/vulkan/resource_provider.hpp†L1-L120】【F:docs/backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md†L31-L37】
- Mitigations / contingency: Mirror OpenGL scheduler behaviours/tests and preserve existing provider interfaces to ease later GPU bring-up.【F:engine/rendering/include/engine/rendering/backend/opengl/gpu_scheduler.hpp†L1-L237】

## 9. Communication Plan
- Async updates cadence: Post incremental results in T-0119 notes and weekly PM-510 demo artefacts while scheduler work lands.【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L9-L37】【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L1-L41】
- Live sync triggers: Escalate to Agent Orchestrator if scheduler changes require ADR amendments or reveal provider contract gaps.
- Escalation path: Agent Orchestrator → Rendering Lead per roadmap governance.【F:docs/ROADMAP.md†L64-L112】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-03 | Product Manager | Scoped follow-up on T-0119 to validate Vulkan scheduler payloads alongside command encoder recordings. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../../../../agents/TEMPLATES) to keep audit trails coherent.
