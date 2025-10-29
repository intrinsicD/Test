# Agent Workflow Portal

## Purpose and Scope
This document is the **single entry point** for every AI or human agent contributing to the Test Engine workspace. It replaces the previous hybrid workflow bundle (`docs/HYBRID_WORKFLOW*.md`, `docs/AGENTIC_WORKFLOW_ENHANCEMENT.md`, agent indexes, and scattered guardrails) with a consolidated operating model. Start here, then follow the links to the dedicated role guide, task templates, and contribution standards.

## Legacy Workflow Assessment
| Observed Issue | Impact | Resolution in This Revision |
| --- | --- | --- |
| Multiple overlapping entry documents (HYBRID_WORKFLOW trilogy, enhancement log, agent indices) | Onboarding required cross-referencing four+ files before work could begin. | Collapsed into this single document with explicit navigation pointers. |
| Role instructions duplicated across twenty separate files. | Context drift and conflicting expectations between role descriptions. | Introduced a unified [`agents/ROLES.md`](agents/ROLES.md) that maps responsibilities, approvals, and quality gates. |
| Documentation alignment was implicit. | Contributors missed module READMEs or ADRs during execution. | Embedded documentation checkpoints into every phase below and templated expectations in [`agents/TEMPLATES/`](agents/TEMPLATES). |
| Build/test commands scattered between README, guardrails, and templates. | Rework during hand-offs and inconsistent validation notes. | Added a centralized build workflow (§ Central Build Workflow) and mirrored it inside all task templates. |

## Phase Overview
The coordination cycle is split into five explicit phases. The **Agent Orchestrator** owns transitions between phases.

1. **Intake & Scoping**  
   - Product Manager prepares a task brief using [`agents/TEMPLATES/TASK_BRIEF_TEMPLATE.md`](agents/TEMPLATES/TASK_BRIEF_TEMPLATE.md).  
   - Task is registered in `docs/backlog/active/` with roadmap identifier references.  
   - Orchestrator validates scope, required roles, and readiness checklist.
2. **Context Assembly**  
   - Knowledge Librarian curates documentation excerpts and cross-links to ADRs, module READMEs, and prior PRs.  
   - Deliverable: [`agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md`](agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md).  
   - Docs/DevRel role ensures all links resolve; unresolved questions are tracked in the brief.
3. **Execution & Collaboration**  
   - Specialist engineers implement changes while logging communication in the task brief.  
   - Coordination rules live in § Coordination Model.  
   - Contributors follow standards in [`CONTRIBUTION.md`](CONTRIBUTION.md) and document deviations immediately.
4. **Quality Gates**  
   - Each gate has an accountable role (see `agents/ROLES.md`).  
   - Evidence is captured in [`agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md`](agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md) and linked back to the task brief.  
   - Gates run concurrently once implementation stabilises; blockers escalate to the Orchestrator.
5. **Release & Documentation Sync**  
   - Release Manager finalises changelog entries, version bumps, and artifact publication.  
   - Docs/DevRel updates module READMEs, roadmap items, and `docs/NAVIGATION.md` as required.  
   - Final confirmation is recorded in the task brief before merge.

## Coordination Model
- **Communication Ledger:** Every hand-off must update the task brief with timestamped notes. Short summaries + links to evidence keep the workflow auditable.
- **Sync Rhythm:** Daily async updates via the task brief; urgent blockers escalate to the Orchestrator who convenes relevant roles.
- **Conflict Resolution:** The Chief Architect resolves architectural disputes. The Docs/DevRel representative ensures documentation decisions are captured.
- **Escalation Paths:**
  1. Missing context → Knowledge Librarian.  
  2. Architectural ambiguity → Chief Architect, referencing `docs/specs/ADR-*.md`.  
  3. Tooling/build failures → Build Engineer.  
  4. Quality gate disagreements → Orchestrator for arbitration.

## Documentation Integration Checklist
For every task, confirm:
1. **Roadmap Alignment:** Reference the owning item in `docs/ROADMAP.md` and the corresponding backlog file.  
2. **Module Documentation:** Update the affected module README using `docs/README_TEMPLATE.md`.  
3. **Architecture Records:** Reflect design-impacting changes in the relevant ADR or create a new one under `docs/specs/`.  
4. **Navigation Update:** If new docs are introduced, add entries to `docs/NAVIGATION.md`.  
5. **Contribution Standards:** Cite the sections in `CONTRIBUTION.md` that apply to the work (naming, testing, coding style).

## Quality Gates Snapshot
| Gate | Accountable Role | Required Evidence |
| --- | --- | --- |
| Testing | QA/Test Specialist | CTest + pytest logs, captured in quality report template. |
| Performance | Performance Engineer | Benchmark diffs ≤2% regression, telemetry snapshots. |
| Security & Safety | Safety Reviewer | Sanitizer runs + dependency diff notes. |
| Documentation | Docs/DevRel | Links to updated READMEs, ADRs, NAVIGATION entries. |
| Review | Reviewer | Completed review checklist referencing `CONTRIBUTION.md`. |
| Release | Release Manager | Tagged release notes, artifact locations, deployment validation.

Each gate must sign off in the quality report template. The Orchestrator ensures no gate is skipped before merge.

## Central Build Workflow
Use these commands across all roles to guarantee reproducible validation. Copy/paste this block into status updates and task briefs.

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

Add additional presets only when mandated by the task scope. Document any deviation inside the quality report template.

## Artefact Overview
- **Roles and responsibilities:** [`agents/ROLES.md`](agents/ROLES.md)
- **Task coordination templates:** [`agents/TEMPLATES/`](agents/TEMPLATES)
- **Contribution standards:** [`CONTRIBUTION.md`](CONTRIBUTION.md)
- **Documentation index:** [`docs/NAVIGATION.md`](docs/NAVIGATION.md)

Keep this document authoritative; when the workflow evolves, update it alongside the linked artefacts in the same commit.
