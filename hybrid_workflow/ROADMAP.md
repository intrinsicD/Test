# ROADMAP

**Current Focus:** Restore GPU execution, runtime presentation readiness, and tooling parity for the AI-004 prototyping workflow.

## Bundle A — GPU Execution (Priority 1)

**Goal:** Enable real GPU rendering with backend command submission and shader pipelines.

- [x] **T-0120** — GPU resource provider → `hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md`
- [x] **T-0119** — Command encoder integration → `hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md`
- [x] **RG-450** — Modular render pipeline planner → `hybrid_workflow/backlog/archive/RG-450-modular-render-pipeline.md`

**Success Criteria:**
- OpenGL/Vulkan execute frame-graph workloads with deterministic telemetry traces.
- Shader pipelines compile and execute real GPU commands.
- Backend smoke tests validate resource creation and command encoding.

---

## Bundle B — Presentation & Tooling (Priority 2)

**Goal:** Runtime presentation adapters and editor foundations for integrated workflows.

- [x] **RT-410** — Runtime stage planner → `hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md`
- [ ] **TL-310** — Editor foundations → `hybrid_workflow/backlog/TL-310-editor-foundations.md`
- [ ] **PM-510** — Weekly integration demos → `hybrid_workflow/backlog/PM-510-weekly-integration-demos.md`

**Success Criteria:**
- Runtime presentation adapters drive editor/tooling previews without manual wiring.
- Editor builds are re-enabled with baseline smoke coverage.
- Weekly demos capture GPU → runtime → tooling integration progress.

_Current status:_ RG-450 has shipped with descriptor-driven planner execution and hot-reload coverage; RT-410 is complete and archived; TL-310 has entered planning under the hybrid workflow while waiting for presentation adapters to unblock tooling activation; PM-510 continues to coordinate weekly integration demos.

---

## Bundle C — Documentation & Infrastructure (Priority 3)

**Goal:** Keep documentation synchronized and improve automation/tooling.

- [x] **DC-050** — Workflow migration to hybrid model → `hybrid_workflow/backlog/archive/DC-050-workflow-migration.md`
- [x] **TL-320** — Task status dashboard automation → `hybrid_workflow/backlog/archive/TL-320-task-dashboard.md`
- [x] **TL-330** — Task status CLI blocked filter → `hybrid_workflow/backlog/archive/TL-330-task-status-blocked-filter.md`
- [x] **TL-331** — Hybrid status reporter JSON export → `hybrid_workflow/backlog/archive/TL-331-hybrid-status-json.md`
- [x] **TL-332** — Task status CLI multiline metadata parsing → `hybrid_workflow/backlog/archive/TL-332-task-status-multiline-metadata.md`
- [x] **TL-341** — Next-action summary for hybrid status reporter → `hybrid_workflow/backlog/archive/TL-341-next-action-summary.md`
- [x] **TL-342** — Hybrid status reporter owner filter → `hybrid_workflow/backlog/archive/TL-342-hybrid-status-owner-filter.md`
- [x] **TL-343** — Task status CLI owner filter → `hybrid_workflow/backlog/archive/TL-343-task-status-owner-filter.md`
- [x] **TL-344** — Next-actions guidance for empty ready queue → `hybrid_workflow/backlog/archive/TL-344-next-actions-guidance.md`
- [x] **TL-345** — Hybrid status reporter relates_to filter → `hybrid_workflow/backlog/archive/TL-345-hybrid-status-relates-to-filter.md`
- [ ] **TL-346** — Next-actions filter support → `hybrid_workflow/backlog/TL-346-next-actions-filter-support.md`

**Success Criteria:**
- All active tasks migrated to hybrid workflow format.
- Automated dashboard reports task status from metadata. ✅ (2025-11-05)
- Documentation cross-links validated in CI.

---

## Bundle D — Kickoff Coordination

**Priority:** P0 (Process/Coordination — runs parallel to technical bundles)

**Goal:** Align backlog, roadmap, and sprint artefacts for the AI-004 kickoff review.

_Note: While Bundle D tasks have P0 priority for process coordination, they run in parallel with Bundles A-C technical work and don't block GPU/runtime/tooling execution._

- [ ] **AI-004** — Kickoff brief readiness → `hybrid_workflow/backlog/AI-004-kickoff-brief.md`
- [ ] **SPRINT-11** — Sprint 11 alignment → `hybrid_workflow/backlog/SPRINT-11-alignment.md`

**Success Criteria:**
- Kickoff packet consolidates agenda, risks, and demo evidence with accountable owners.
- Sprint 11 ledger feeds directly into kickoff packet updates and roadmap milestones.
- Documentation validation recorded after each update to kickoff artefacts.

---

## Risks & Mitigations

| Priority | Risk | Mitigation | Owner |
|----------|------|------------|-------|
| 1 | GPU resource provider/command encoder slip | Joint milestone with shared design reviews; weekly demos in PM-510 | Rendering Lead |
| 1 | Stage planner delays blocking presentation | Completed 2026-03-30; monitor TL-310 enablement and presentation telemetry | Runtime Lead |
| 2 | Editor re-enablement blocked by runtime hooks | Sequence TL-310 after RT-410 adapters merge | Tools Lead |
| 3 | Workflow migration overhead | Gradual migration; new tasks use hybrid, old tasks migrated opportunistically | Agent Orchestrator |

---

## Task Status Summary

Tasks are tracked in `hybrid_workflow/backlog/` with structured metadata. Query status programmatically:

```bash
# Quick next actions (ready → new fallback)
python -m scripts.workflow.report_hybrid_status --next-actions

# List ready tasks
grep -r "^status: ready" hybrid_workflow/backlog/*.md

# Show high-priority tasks
grep -r "^priority: P1" hybrid_workflow/backlog/*.md

# Find blocked tasks
grep -r "^blocked_on:" hybrid_workflow/backlog/*.md

# Generate HTML + JSON dashboard
python -m scripts.workflow.dashboard --output-dir build/hybrid-dashboard
```

---

## Maintenance

- **Weekly:** Review task status and update bundle checkboxes.
- **Monthly:** Reassess priorities and risks based on progress.
- **Per Task:** Update this roadmap when tasks complete (mark checkbox, note completion date).
- **Bundle Complete:** Document learnings and archive bundle notes.

---

## Archive

Completed tasks move to `hybrid_workflow/backlog/archive/` with their original metadata preserved for audit trails.

**Completed Bundles:** (none yet)

---

_Source of truth: Task metadata in `hybrid_workflow/backlog/*.md`_

