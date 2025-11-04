# ROADMAP

**Current Focus:** Restore GPU execution, runtime presentation readiness, and tooling parity for the AI-004 prototyping workflow.

**Period:** Q1 2025 → Q2 2025

---

## Bundle A — GPU Execution (Priority 1)

**Goal:** Enable real GPU rendering with backend command submission and shader pipelines.

- [ ] **T-0120** — GPU resource provider → `hybrid_workflow/backlog/T-0120-gpu-resource-provider.md`
- [ ] **T-0119** — Command encoder integration → `hybrid_workflow/backlog/T-0119-command-encoder-integration.md`

**Success Criteria:**
- OpenGL/Vulkan execute frame-graph workloads with deterministic telemetry traces.
- Shader pipelines compile and execute real GPU commands.
- Backend smoke tests validate resource creation and command encoding.

---

## Bundle B — Presentation & Tooling (Priority 2)

**Goal:** Runtime presentation adapters and editor foundations for integrated workflows.

- [ ] **RT-410** — Runtime stage planner → `hybrid_workflow/backlog/RT-410-runtime-stage-planner.md`
- [ ] **TL-310** — Editor foundations → `hybrid_workflow/backlog/TL-310-editor-foundations.md`
- [ ] **PM-510** — Weekly integration demos → `hybrid_workflow/backlog/PM-510-weekly-integration-demos.md`

**Success Criteria:**
- Runtime presentation adapters drive editor/tooling previews without manual wiring.
- Editor builds are re-enabled with baseline smoke coverage.
- Weekly demos capture GPU → runtime → tooling integration progress.

---

## Bundle C — Documentation & Infrastructure (Priority 3)

**Goal:** Keep documentation synchronized and improve automation/tooling.

- [ ] **DC-050** — Workflow migration to hybrid model → `hybrid_workflow/backlog/DC-050-workflow-migration.md`
- [ ] **TL-320** — Task status dashboard automation → `hybrid_workflow/backlog/TL-320-task-dashboard.md`

**Success Criteria:**
- All active tasks migrated to hybrid workflow format.
- Automated dashboard reports task status from metadata.
- Documentation cross-links validated in CI.

---

## Risks & Mitigations

| Priority | Risk | Mitigation | Owner |
|----------|------|------------|-------|
| 1 | GPU resource provider/command encoder slip | Joint milestone with shared design reviews; weekly demos in PM-510 | Rendering Lead |
| 1 | Stage planner delays blocking presentation | Start RT-410 parallel to GPU work; preview in weekly demos | Runtime Lead |
| 2 | Editor re-enablement blocked by runtime hooks | Sequence TL-310 after RT-410 adapters merge | Tools Lead |
| 3 | Workflow migration overhead | Gradual migration; new tasks use hybrid, old tasks migrated opportunistically | Agent Orchestrator |

---

## Task Status Summary

Tasks are tracked in `hybrid_workflow/backlog/` with structured metadata. Query status programmatically:

```bash
# List ready tasks
grep -r "^status: ready" hybrid_workflow/backlog/*.md

# Show high-priority tasks
grep -r "^priority: P1" hybrid_workflow/backlog/*.md

# Find blocked tasks
grep -r "^blocked_on:" hybrid_workflow/backlog/*.md
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

_Last updated: 2025-11-04 · Source of truth: Task metadata in `hybrid_workflow/backlog/*.md`_

