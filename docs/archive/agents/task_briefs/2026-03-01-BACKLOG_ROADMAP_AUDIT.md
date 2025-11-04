# Task Brief — Backlog and Roadmap Hygiene Audit

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** Backlog and Roadmap Hygiene Audit
- **Roadmap / Backlog Reference:** [`docs/ROADMAP.md`](../../../ROADMAP.md) and active backlog entries under [`docs/backlog/active/`](../../../backlog/active)
- **Primary Goal:** Assess alignment between the roadmap, active backlog items, and shipped implementation state for rendering, runtime, and tooling subsystems while flagging archival or scope corrections.【F:README.md†L5-L105】【F:docs/ROADMAP.md†L64-L118】【F:docs/modules/rendering/README.md†L1-L15】【F:docs/modules/runtime/README.md†L1-L36】【F:docs/modules/tools/README.md†L1-L14】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-03-01-BACKLOG_ROADMAP_AUDIT.md`), context package (`agents/context_packages/2026-03-01-BACKLOG_ROADMAP_AUDIT.md`), remediation backlog item ([`docs/backlog/active/PM_520_BACKLOG_HYGIENE_REMEDIATION.md`](../../../backlog/active/PM_520_BACKLOG_HYGIENE_REMEDIATION.md))

## 2. Scope & Boundaries
- In scope:
  - Review roadmap priority tables and active backlog files for status drift or archival candidates.【F:docs/ROADMAP.md†L19-L118】【F:docs/backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md†L1-L42】【F:docs/backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md†L1-L27】
  - Cross-check subsystem READMEs and representative source files to confirm whether "In Progress" items remain incomplete.【F:docs/modules/rendering/README.md†L1-L15】【F:docs/modules/runtime/README.md†L1-L36】【F:docs/modules/tools/README.md†L1-L14】【F:engine/rendering/src/backend/opengl/command_stream.cpp†L13-L55】【F:engine/runtime/src/api.cpp†L2160-L2228】
  - Capture discrepancies and recommended actions in the audit report and communication plan.【F:docs/backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md†L1-L38】【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L1-L38】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】【F:docs/backlog/active/TL_310_EDITOR_FOUNDATIONS.md†L1-L38】【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L1-L29】
- Out of scope:
  - Implementing backlog fixes or code changes outside the audit recommendations.
  - Modifying historical archive entries unless the audit uncovers blocking inconsistencies.
- Architectural considerations / ADRs:
  - GPU enablement and runtime loop work must respect ADR-0003 and ADR-0008 contracts while we evaluate progress against those decisions.【F:docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md†L1-L24】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L1-L118】

## 3. Success Criteria
- Functional: Document whether each active backlog item is current, archived, or missing, and identify misalignments between backlog status and shipped code.【F:docs/backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md†L1-L38】【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L1-L38】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】
- Documentation: Recommend roadmap/backlog updates, including archival moves for fully complete items still listed as active and roadmap coverage gaps (e.g., PL-240).【F:docs/ROADMAP.md†L96-L118】【F:docs/backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md†L1-L27】
- Validation: Corroborate subsystem status with code references that demonstrate remaining gaps (e.g., OpenGL command stream stub, runtime presentation hooks).【F:engine/rendering/src/backend/opengl/command_stream.cpp†L13-L55】【F:engine/runtime/src/api.cpp†L2160-L2228】
- Quality gates & benchmarks: Capture existing canonical build/test commands to reuse if remediation work is scheduled.【F:README.md†L122-L142】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Confirms context ladder process, module health snapshot, and build/test workflow.【F:README.md†L5-L142】 | Product Manager |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Documents directory structure, backlog/roadmap entry points, and precedence rules.【F:docs/NAVIGATION.md†L1-L113】 | Knowledge Librarian |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase breakdown shows Phase 4 tasks active while Phase 1–3 entries marked complete; roadmap snapshot still lists archived-ready entries.【F:docs/ROADMAP.md†L64-L118】 | Product Manager |
| 4 | [`docs/backlog/active/`](../../../backlog/active) | Active directory still contains numerous "Complete" items (DC-040, DC-041, RT-320, etc.) plus PL-240 that is absent from roadmap tables; remediation task tracked under PM-520.【F:docs/backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md†L1-L42】【F:docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md†L1-L31】【F:docs/backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md†L1-L62】【F:docs/backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md†L1-L27】【F:docs/backlog/active/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L86】 | Knowledge Librarian |
| 5 | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Rendering remains blocked pending T-0119/T-0120; verifies backlog statuses against subsystem reality.【F:docs/modules/rendering/README.md†L1-L15】 | Specialist Engineer |
| 6 | [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](../../../specs/ADR_0003_RUNTIME_FRAME_GRAPH.md) & [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Establishes binding requirements for GPU/resource work and runtime stage planner integration.【F:docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md†L1-L24】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L1-L118】 | Chief Architect |
| 7 | [`engine/rendering/src/backend/opengl/command_stream.cpp`](../../../../engine/rendering/src/backend/opengl/command_stream.cpp) & [`engine/runtime/src/api.cpp`](../../../../engine/runtime/src/api.cpp) | Code inspection shows command stream default stub and runtime presentation stage rebuild hooks—evidence of partial implementation aligning with backlog "In Progress" status.【F:engine/rendering/src/backend/opengl/command_stream.cpp†L13-L58】【F:engine/runtime/src/api.cpp†L2160-L2228】 | Specialist Engineer |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | TBD | 1–5 | Coordinate phases, approve exits | Pending |
| Knowledge Librarian | Assigned (audit lead) | 2 & 5 | Compile context package, archive findings | Active |
| Specialist Engineer(s) | Rendering & Runtime leads | 3 | Validate subsystem state vs. backlog | Pending |
| Docs/DevRel | Docs team | 2, 4, 5 | Update roadmap/backlog/module docs per audit; own PM-520 execution. | Pending |
| QA/Test Specialist | QA lead | 4 | Verify automation once backlog hygiene applied | Pending |
| Performance Engineer | Performance lead | 4 | Confirm benchmark coverage remains intact | Pending |
| Safety Reviewer | Security reviewer | 4 | Ensure archival changes respect compliance | Pending |
| Reviewer | Assigned reviewer | 4 | Approve documentation and backlog updates | Pending |
| Release Manager | Release manager | 5 | Coordinate publication of updated artefacts | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Task charter logged; roadmap/backlog files identified.【F:docs/ROADMAP.md†L19-L118】 | Stakeholders acknowledge audit scope. | Task brief |
| 2 – Context Assembly | Context ladder traversed; module/ADR notes captured.【F:docs/modules/rendering/README.md†L1-L15】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L1-L118】 | Context package reviewed by engineering leads. | Context package |
| 3 – Execution & Collaboration | Engineers confirm backlog/code comparisons for each item.【F:engine/rendering/src/backend/opengl/command_stream.cpp†L13-L58】【F:engine/runtime/src/api.cpp†L2160-L2228】 | Draft audit findings circulated for comments. | Draft report |
| 4 – Quality Gates | QA validates documentation consistency, automation unaffected.【F:README.md†L122-L142】 | Sign-off on backlog/roadmap updates and archival actions. | QA sign-off notes |
| 5 – Release & Documentation Sync | Final audit merged; navigation updated if files move.【F:docs/NAVIGATION.md†L21-L33】 | Roadmap/backlog reflect agreed state; artefacts archived. | PR & changelog |

## 7. Timeline & Milestones
- Kickoff: TBD (align with next PM-510 integration demo cadence).【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L1-L29】
- Implementation window: TBD (target before next roadmap refresh).
- Quality gate window: TBD (coordinate with QA once backlog updates staged).
- Release target: TBD (publish with weekly demo summary).
- Post-release monitoring: Review backlog hygiene during subsequent PM-510 demo retro.【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L23-L29】

## 8. Known Risks & Dependencies
- Risks:
  - Continued backlog drift confuses priority focus and violates roadmap maintenance checklist.【F:docs/ROADMAP.md†L111-L118】
  - Archival actions might desynchronise navigation links if not coordinated.【F:docs/NAVIGATION.md†L96-L107】
- Dependencies:
  - Rendering and runtime leads to confirm status for T-0119, T-0120, RT-410.【F:docs/backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md†L1-L38】【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L1-L38】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】
  - Docs/DevRel to execute archival moves and update navigation via PM-520.【F:docs/NAVIGATION.md†L21-L33】【F:docs/backlog/active/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L86】
- Mitigations / contingency:
  - Stage archival via PR with navigation updates and docs validator to prevent broken links.【F:README.md†L122-L142】【F:docs/NAVIGATION.md†L99-L107】

## 9. Communication Plan
- Async updates cadence: Weekly summary aligned with PM-510 integration demo notes.【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L23-L29】
- Live sync triggers: Convene focused review if GPU milestone slips or roadmap updates required ahead of demos.【F:docs/ROADMAP.md†L75-L83】
- Escalation path: Escalate unresolved conflicts to Agent Orchestrator per AGENTS workflow hierarchy.【F:AGENTS.md†L61-L103】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-01 | Knowledge Librarian | Logged remediation backlog item PM-520 to track archival and roadmap sync work. | Open |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](.) to keep audit trails coherent.
