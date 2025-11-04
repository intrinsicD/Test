# Task Brief — PM-520 Backlog and Roadmap Hygiene Remediation

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** PM-520 Backlog and Roadmap Hygiene Remediation
- **Roadmap / Backlog Reference:** [`docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md`](../../../backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md); [`docs/ROADMAP.md`](../../../ROADMAP.md) Phase 4 priorities.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L34】【F:docs/ROADMAP.md†L64-L103】
- **Primary Goal:** Archive completed backlog entries, align roadmap tables with the curated active set, and sync navigation pointers so contributors operate from an authoritative task list.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L34】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-03-05-PM_520_BACKLOG_HYGIENE.md`), context package (`agents/context_packages/2026-03-05-PM_520_BACKLOG_HYGIENE.md`), quality report (pending).

## 2. Scope & Boundaries
- In scope:
  - Move completed backlog entries (DC-040, DC-041, RT-320, TL-210, RT-321, AS-330, CC-310, CC-311, PL-240) into `docs/backlog/archive/` while preserving metadata.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L26-L30】
  - Update roadmap and README snapshots to reference archived locations only for open work, and record PL-240’s archival outcome.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L27-L30】【F:docs/ROADMAP.md†L64-L118】
  - Refresh `docs/NAVIGATION.md` (and any downstream links) to reflect the new file paths per navigation precedence rules.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L27-L30】【F:docs/NAVIGATION.md†L1-L113】
- Out of scope:
  - Altering in-progress GPU/runtime/tooling backlog items (T-0120, T-0119, RT-410, TL-310) beyond updating cross-links.【F:docs/ROADMAP.md†L64-L103】
  - Modifying benchmark automation code or telemetry budgets managed under CC-310/CC-311 except for link maintenance.【F:docs/backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md†L1-L48】【F:docs/backlog/archive/CC_311_BENCHMARK_VISUALISATION.md†L1-L48】
- Architectural considerations / ADRs:
  - Respect ADR-0003 and ADR-0008 when updating references to GPU/runtime work; documentation must continue to reflect their design commitments.【F:docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md†L1-L24】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L115】
  - Follow AGENTS deliverable matrix requiring synchronized roadmap/backlog/doc updates per change set.【F:AGENTS.md†L32-L117】

## 3. Success Criteria
- Functional: Completed backlog entries reside under `docs/backlog/archive/` with intact content and updated cross-links; active directory lists only open items.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L26-L30】
- Documentation: Roadmap tables, root README backlog overview, and NAVIGATION pointers reference archived paths and mention PL-240 archival status.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L27-L30】【F:README.md†L98-L107】【F:docs/ROADMAP.md†L64-L103】
- Validation: `python scripts/validate_docs.py` passes after moves; cite command output in quality report.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L30-L31】【F:README.md†L122-L142】
- Quality gates & benchmarks: Standard cmake/ctest/pytest/validate_docs block executed to ensure no regressions in automation triggers.【F:AGENTS.md†L86-L99】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Confirms expectation to sync roadmap/backlog updates and provides canonical command block for validation.【F:README.md†L5-L142】 | Product Manager |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Establishes directory precedence and need to update navigation when moving backlog files.【F:docs/NAVIGATION.md†L1-L113】 | Knowledge Librarian |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 snapshot lists PM-520 as planned while earlier completed items remain referenced as active.【F:docs/ROADMAP.md†L64-L118】 | Product Manager |
| 4 | [`docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md`](../../../backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md) | Definition of done mandates archival + navigation updates; dependencies include GPU/runtime milestones.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L35】 | Docs/DevRel |
| 5 | Module READMEs (rendering/runtime/tools) | Confirm current module health statements reference open work; ensure post-archival links remain valid.【F:docs/modules/rendering/README.md†L1-L14】【F:docs/modules/runtime/README.md†L1-L36】【F:docs/modules/tools/README.md†L1-L14】 | Specialist Engineer |
| 6 | ADRs (`ADR-0003`, `ADR-0008`) | Maintain consistency with GPU/resource and runtime presentation decisions while updating doc references.【F:docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md†L1-L24】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L31-L115】 | Chief Architect |
| 7 | Historical audit (`agents/context_packages/2026-03-01-BACKLOG_ROADMAP_AUDIT.md`) | Provides prior remediation findings informing this effort’s scope.【F:agents/context_packages/2026-03-01-BACKLOG_ROADMAP_AUDIT.md†L1-L90】 | Knowledge Librarian |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assistant (acting) | 1–5 | Approve scope, ensure roadmap/backlog sync adheres to workflow. | Active |
| Knowledge Librarian | Assistant | 2 & 5 | Compile context package, archive artefacts post-merge. | Active |
| Specialist Engineer(s) | Assistant (Docs specialist) | 3 | Execute file moves, update references/tests. | Active |
| Docs/DevRel | Assistant | 2, 4, 5 | Review doc tone/style, ensure NAVIGATION alignment. | Active |
| QA/Test Specialist | Assistant | 4 | Run validation commands, record outputs. | Pending |
| Performance Engineer | Assistant | 4 | Monitor benchmark references for regressions (documentation only). | Pending |
| Safety Reviewer | Assistant | 4 | Confirm archival retains licensing/compliance notes. | Pending |
| Reviewer | Assistant | 4 | Self-review per CONTRIBUTION.md. | Pending |
| Release Manager | Assistant | 5 | Summarise archival outcome in changelog/brief. | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Roadmap/backlog discrepancies identified via PM-520 DOD. | Task brief approved with scope + risks logged. | This document.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L35】 |
| 2 – Context Assembly | Context ladder traversed; key artefacts cited. | Context package published with open questions documented. | `agents/context_packages/2026-03-05-PM_520_BACKLOG_HYGIENE.md`. |
| 3 – Execution & Collaboration | Plan approved; file move script prepared. | Backlog files moved, references updated, plan adjustments logged. | Git commits + task brief decision log. |
| 4 – Quality Gates | Implementation ready for validation. | Standard build/test commands executed with passing results, docs validator clean. | Quality report (pending). |
| 5 – Release & Documentation Sync | Quality report approved. | Navigation/roadmap/root README updated, backlog status synced, PR published. | Final PR + changelog summary. |

## 7. Timeline & Milestones
- Kickoff: 2026-03-05 (post-audit remediation start).【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L34】
- Implementation window: 2026-03-05 single-session update.
- Quality gate window: Immediately after implementation (same session).
- Release target: 2026-03-05 PR merge following review.
- Post-release monitoring: Validate no broken links or navigation regressions in subsequent documentation validator runs.【F:README.md†L122-L142】

## 8. Known Risks & Dependencies
- Risks:
  - Broken internal links or stale references after file moves could misdirect contributors.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L27-L30】
  - Archival timing might conflict with GPU/runtime documentation updates currently in progress.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L32-L34】
- Dependencies:
  - Coordination with rendering/runtime leads handling T-0120/T-0119/RT-410 for cross-link accuracy.【F:docs/backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md†L1-L37】【F:docs/backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md†L1-L35】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L36】
  - Weekly integration demo comms via PM-510 to announce archival changes.【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L1-L29】
- Mitigations / contingency:
  - Run `python scripts/validate_docs.py` and targeted `rg` checks to confirm links; revert path updates if validation fails.【F:README.md†L122-L142】
  - Document archival outcomes in roadmap risk notes to keep teams informed.【F:docs/ROADMAP.md†L85-L118】

## 9. Communication Plan
- Async updates cadence: Document progress in task brief decision log after each phase transition (same-day updates).【F:AGENTS.md†L61-L117】
- Live sync triggers: None anticipated; escalate via PM-510 demo notes if blockers arise.【F:docs/backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md†L1-L29】
- Escalation path: Notify Agent Orchestrator for workflow conflicts; escalate architectural issues to Chief Architect per AGENTS guardrails.【F:AGENTS.md†L117-L139】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-05 | Assistant | Task brief drafted; scope approved for PM-520 remediation. | Approved |
| 2026-03-05 | Assistant | Archived backlog entries, updated roadmap/navigation/root README, and marked PM-520 Definition of Done complete. | Complete |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../../../../agents/TEMPLATES) to keep audit trails coherent.
