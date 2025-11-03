# Backlog Item PM-520 — Backlog and Roadmap Hygiene Remediation

- **Status**: Complete
- **Priority**: 2
- **Owner**: Docs/DevRel
- **Module(s)**: Documentation, Product Operations
- **Goal**: Archive completed backlog entries lingering in `docs/backlog/active/`, align `docs/ROADMAP.md` with the curated active set, and update navigation so contributors have an authoritative source of truth.【F:docs/ROADMAP.md†L96-L118】【F:docs/backlog/archive/DC-040-ai-004-configuration-schema.md†L1-L42】【F:docs/backlog/archive/DC-041-ai-004-kickoff-readiness.md†L1-L31】

## Summary
The March 2026 hygiene audit identified that the active backlog directory still contains items already marked "Complete"—including DC-040, DC-041, RT-320, TL-210, RT-321, AS-330, CC-310, CC-311, and PL-240—while the roadmap snapshot omits PL-240 despite its completion status.【F:docs/backlog/archive/DC-040-ai-004-configuration-schema.md†L1-L42】【F:docs/backlog/archive/DC-041-ai-004-kickoff-readiness.md†L1-L31】【F:docs/backlog/archive/RT-320-runtime-prototyping-harness.md†L1-L62】【F:docs/backlog/archive/TL-210-experiment-sandbox-ui.md†L1-L57】【F:docs/backlog/archive/RT-321-prototyping-case-studies.md†L1-L57】【F:docs/backlog/archive/AS-330-reference-dataset-packages.md†L1-L58】【F:docs/backlog/archive/CC-310-comparative-benchmark-automation.md†L1-L48】【F:docs/backlog/archive/CC-311-benchmark-visualisation.md†L1-L47】【F:docs/backlog/archive/PL-240-platform-filesystem-watcher-guidance.md†L1-L27】【F:docs/ROADMAP.md†L96-L118】 This task executes the remediation plan so roadmap, backlog, and navigation all reflect the actual work queue.【F:agents/context_packages/2026-03-01-backlog-roadmap-audit.md†L1-L78】

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Approve archival plan and coordinate review | TBD |
| Product Manager | Confirm roadmap deltas and messaging | TBD |
| Knowledge Librarian | Supply audit evidence and verify archival targets | Assigned |
| Specialist Engineer(s) | Spot regressions in subsystem TODOs after archival | Rendering & Runtime Leads |
| Docs/DevRel | Execute file moves, roadmap edits, and navigation sync | Docs Team |
| QA/Test Specialist | Run documentation validator and smoke tests post-move | QA Lead |
| Performance Engineer | Ensure benchmark references remain intact | Performance Lead |
| Safety Reviewer | Confirm no compliance artefacts lost during archival | Security Reviewer |
| Reviewer | Provide final approval on documentation PR | Assigned Reviewer |
| Release Manager | Publish changelog entry for backlog maintenance | Release Manager |

## Definition of Done
- [x] Completed backlog entries (DC-040, DC-041, RT-320, TL-210, RT-321, AS-330, CC-310, CC-311, PL-240) are moved to `docs/backlog/archive/` with preserved metadata.【F:docs/backlog/archive/DC-040-ai-004-configuration-schema.md†L1-L42】【F:docs/backlog/archive/README.md†L1-L8】
- [x] `docs/ROADMAP.md` active snapshot and tables reference only open work and document the archival outcome for PL-240.【F:docs/ROADMAP.md†L64-L118】
- [x] `docs/NAVIGATION.md` and any module README links referencing moved backlog items are updated accordingly.【F:docs/NAVIGATION.md†L1-L113】
- [x] `python scripts/validate_docs.py` and roadmap/backlog linting succeed after the updates.【F:README.md†L122-L142】

## Dependencies
- Approval from subsystem leads on timing of archival to avoid disrupting in-progress documentation refreshes.【F:docs/backlog/active/T-0119-command-encoder-integration.md†L1-L38】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L1-L38】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】
- Coordination with PM-510 demo cadence to communicate backlog changes in weekly summaries.【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L29】

## Related Artefacts
- Task brief: [`agents/task_briefs/2026-03-01-backlog-roadmap-audit.md`](../../../agents/task_briefs/2026-03-01-backlog-roadmap-audit.md)
- Context package: [`agents/context_packages/2026-03-01-backlog-roadmap-audit.md`](../../../agents/context_packages/2026-03-01-backlog-roadmap-audit.md)
- AGENTS workflow: [`AGENTS.md`](../../../AGENTS.md)

## Notes
- Run `python scripts/update_agents_tree.py` after archival to refresh generated guidance indexes if directory layouts shift.【F:README.md†L122-L142】
- Ensure archived files retain their historical citations for future audits; do not rewrite completion notes beyond moving location.【F:docs/backlog/archive/README.md†L1-L64】
- 2026-03-05 — Archived DC-040, DC-041, RT-320, TL-210, RT-321, AS-330, CC-310, CC-311, and PL-240; roadmap, navigation, and root README now reference `docs/backlog/archive/`.【F:docs/ROADMAP.md†L64-L118】【F:docs/NAVIGATION.md†L1-L113】【F:README.md†L98-L107】
