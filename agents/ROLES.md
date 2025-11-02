# Agent Roles and Responsibilities

This guide replaces the legacy stack of numbered role files. All contributors inherit the common ground rules from [`CONTRIBUTION.md`](../CONTRIBUTION.md) and orchestrate work using the Workflow Blueprint in [`AGENTS.md`](../AGENTS.md).

## Core Roles
| Role | Primary Owner | Workflow Phases | Key Responsibilities | Mandatory Deliverables | Communication Path |
| --- | --- | --- | --- | --- | --- |
| Agent Orchestrator | 11 | All phases (lead transitions) | Schedule phases, assemble role roster, maintain task ledger, resolve blockers, approve phase exits. | Updated task brief with roster + timeline, escalation log, workflow-change backlog entries. | Direct line to Product Manager, Quality Leads, Release Manager. |
| Product Manager | 10 | Phase 1 | Define scope, acceptance criteria, roadmap alignment, approve intake exit. | [`TASK_BRIEF_TEMPLATE`](TEMPLATES/TASK_BRIEF_TEMPLATE.md) filled and linked to backlog with risk ledger. | Daily sync with Orchestrator; weekly status to stakeholders. |
| Knowledge Librarian | 12 | Phases 2 & 5 | Curate context packs, ensure documentation references resolve, archive decisions for reuse. | [`CONTEXT_PACKAGE_TEMPLATE`](TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md) plus annotated sources and open-question register. | Shares updates via task brief comments; coordinates with Docs/DevRel. |
| Specialist Engineer | Domain dependent (geometry, rendering, runtime, etc.) | Phase 3 | Implement features/fixes, capture rationale, align with architecture invariants, prep validation hand-offs. | Code changes + inline documentation + test updates referencing `CONTRIBUTION.md`; execution notes in task brief. | Collaborates with QA, Performance, and Docs roles inside task brief. |
| Docs/DevRel | 95 | Phases 2, 4, 5 | Keep documentation synchronized, update NAVIGATION and module READMEs, audit terminology, approve documentation gate. | Documentation change log appended to task brief, merged doc updates, navigation diffs. | Works with Knowledge Librarian; posts updates in documentation channel. |
| QA/Test Specialist | 90 | Phase 4 | Execute validation suite, record evidence, maintain regression artifacts, own testing gate. | [`QUALITY_REPORT_TEMPLATE`](TEMPLATES/QUALITY_REPORT_TEMPLATE.md) with logs + verdicts and command ledger. | Flags failures to Orchestrator and Specialist Engineer immediately. |
| Performance Engineer | 80 | Phase 4 | Benchmark impacted areas, review telemetry, sign off on regressions, enforce budgets. | Benchmark summary embedded in quality report template, comparison charts when applicable. | Coordinates with Specialist Engineer for tuning or mitigation plans. |
| Safety Reviewer | 15 | Phase 4 | Run sanitizers, dependency audits, security review, record risk mitigations. | Sanitizer logs, dependency diffs, risk notes attached to quality report. | Escalates critical issues to Orchestrator and Release Manager. |
| Reviewer | 99 | Phase 4 | Perform code review, ensure standards compliance, triage follow-ups, enforce template completeness. | Review checklist referencing `CONTRIBUTION.md`; decision recorded in task brief and quality report. | Collaborates with Specialist Engineer and Docs/DevRel for clarifications. |
| Release Manager | 98 | Phase 5 | Package artifacts, tag releases, update changelog, verify deployment steps, confirm archive updates. | Release section in quality report + changelog PR + release readiness note in task brief. | Communicates rollout timing to stakeholders and Orchestrator. |

## Collaboration Rules
- **Role Substitution:** When a specialist engineer covers multiple domains, note the combined responsibilities explicitly in the task brief and ensure quality gates are still independently validated.
- **Documentation Custody:** Docs/DevRel owns final documentation approval. If conflicts arise with Knowledge Librarian curation, the Orchestrator arbitrates.
- **Quality Gate Completion:** Each accountable role must sign the quality report. Missing evidence blocks merge.
- **Continuous Improvement:** Suggestions for workflow refinements are filed as backlog items tagged `workflow` and linked from the task brief for visibility.

Keep this file synchronized with organizational changes. If a new role emerges, update the table here and adjust templates accordingly in the same change.
