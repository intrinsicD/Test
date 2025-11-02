# Documentation Navigation for AI Agents

## 🎯 Start Here Workflow

**First-time or general questions?**
1. Read [`../README.md`](../README.md) – workspace overview, build steps, module health.
2. Load [`../AGENTS.md`](../AGENTS.md) – follow the Workflow Blueprint (Sections 0.1–0.7) to populate the context ladder and deliverable matrix for your task.
3. Review [`../agents/ROLES.md`](../agents/ROLES.md) and the templates in [`../agents/TEMPLATES/`](../agents/TEMPLATES) for task-specific execution.
4. Return here for specialised documentation and directory references.

**Working on a specific task?**
1. Check [`ROADMAP.md`](ROADMAP.md) – confirm the milestone and priority band.
2. Open the relevant backlog entry under [`backlog/active/`](backlog/active/) – review its Role Roster, definition of done, dependencies, and artefacts.
3. Read the module notes in [`modules/<name>/README.md`](modules/) – understand subsystem invariants.
4. Consult related specs in [`specs/`](specs/) – respect architectural decisions.

**Need architectural context?**
1. [`ARCHITECTURE.md`](ARCHITECTURE.md) – system boundaries, data flow, invariants.
2. [`specs/ADR-*.md`](specs/) – binding decisions.

## 📁 Directory Guide

| Directory | Purpose | When to Use |
|-----------|---------|-------------|
| `backlog/` | Active + archived backlog items with priority 1–5 scale | Planning, status tracking |
| `design/` | Deep dives, guides, strategies | Understanding complex subsystems |
| `architecture/` | Generated diagrams (module dependency graph, wiring snapshots) | Visualising system structure |
| `modules/` | Per-module README + roadmap | Working in specific subsystem |
| `prompts/` | Reusable agent instruction templates | Standardising AI workflows |
| `templates/` | Documentation templates (research papers, READMEs) | Standardising new documentation |
| `specs/` | ADRs, RFPs – binding architectural decisions | Before designing new features |
| `examples/` | Usage samples and tutorials ([README](examples/README.md)) | Learning API usage |
| `archive/` | Historical artefacts (completed tasks, reviews) | Research and provenance |

## 🔀 Workflow References

| Document | Purpose | When to Use |
|----------|---------|-------------|
| [`../AGENTS.md`](../AGENTS.md) | Workflow Blueprint (Sections 0.1–0.7), phase checklists, quality instrumentation | Beginning any contribution |
| [`../AGENTS.md#agent-directory-workflow`](../AGENTS.md#agent-directory-workflow) | Directory overview, maintenance guardrails, workflow-change process | Managing artefacts inside `agents/` |
| [`../agents/ROLES.md`](../agents/ROLES.md) | Specialised role responsibilities and approvals | Assigning or fulfilling roles |
| [`../agents/TEMPLATES/`](../agents/TEMPLATES) | Task brief, context package, and quality report templates | Documenting and auditing work |

## 🔧 Common Tasks

### Implementing a Feature
→ [`prompts/IMPLEMENTATION-PLAYBOOK.md`](prompts/IMPLEMENTATION-PLAYBOOK.md)

### Reviewing Code
→ [`prompts/REVIEW-CHECKLIST.md`](prompts/REVIEW-CHECKLIST.md)

### Refactoring
→ [`prompts/REFACTOR-PLAYBOOK.md`](prompts/REFACTOR-PLAYBOOK.md)

### Architecture Audit
→ [`prompts/ARCHITECTURE-AUDIT.md`](prompts/ARCHITECTURE-AUDIT.md)

### AI-004 Prototyping Workflow
→ [`design/AI-004-prototyping-playbook.md`](design/AI-004-prototyping-playbook.md)

## 📊 Key References

- **Coding Standards:** [`../CONTRIBUTION.md`](../CONTRIBUTION.md)
- **Telemetry:** [`design/TELEMETRY_SCHEMA.md`](design/TELEMETRY_SCHEMA.md), [`design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](design/TELEMETRY_INSTRUMENTATION_GUIDE.md)
- **Error Handling:** [`design/ERROR_HANDLING_MIGRATION.md`](design/ERROR_HANDLING_MIGRATION.md)
- **Resource Management:** [`design/RESOURCE_MANAGEMENT.md`](design/RESOURCE_MANAGEMENT.md)
- **Accessibility:** [`design/TL-210-accessibility-checklist.md`](design/TL-210-accessibility-checklist.md)
- **Case Study Baselines:** [`design/RT-321-case-studies.md`](design/RT-321-case-studies.md)
- **Benchmark Automation:** [`design/CC-310-benchmark-playbook.md`](design/CC-310-benchmark-playbook.md)
- **Dependency Graph:** [`architecture/README.md`](architecture/README.md)
- **Research Template:** [`templates/RESEARCH_PAPER_TEMPLATE.md`](templates/RESEARCH_PAPER_TEMPLATE.md)
- **Backlog Template:** [`backlog/README.md`](backlog/README.md)
- **Architecture Reviews:** [`reviews/2025-10-26-architecture-audit.md`](reviews/2025-10-26-architecture-audit.md), [`reviews/2025-12-05-roadmap-direction-review.md`](reviews/2025-12-05-roadmap-direction-review.md), [`reviews/2026-01-08-application-readiness-assessment.md`](reviews/2026-01-08-application-readiness-assessment.md), [`reviews/2026-02-10-comprehensive-architecture-evaluation.md`](reviews/2026-02-10-comprehensive-architecture-evaluation.md)

## 🗂️ Module Overview

All 13 engine modules are documented under [`modules/`](modules/):

| Module | Purpose |
|--------|---------|
| [Animation](modules/animation/README.md) | Animation system, state machines, blending |
| [Assets](modules/assets/README.md) | Asset management, hot reload, generational handles |
| [Compute](modules/compute/README.md) | GPU compute, kernel dispatch, CUDA interop |
| [Core](modules/core/README.md) | ECS registry, subsystem discovery, lifecycle |
| [Geometry](modules/geometry/README.md) | Mesh/point-cloud processing, spatial structures |
| [IO](modules/io/README.md) | Import/export, file format detection, fuzzing |
| [Math](modules/math/README.md) | Vector/matrix/quaternion primitives |
| [Physics](modules/physics/README.md) | Rigid body simulation, collision detection |
| [Platform](modules/platform/README.md) | Windowing, input, filesystem abstraction |
| [Rendering](modules/rendering/README.md) | Frame graph, backend abstraction (Vulkan/DX12/GL) |
| [Runtime](modules/runtime/README.md) | Main loop orchestration, subsystem integration |
| [Scene](modules/scene/README.md) | Entity management, hierarchy, serialization |
| [Tools](modules/tools/README.md) | Editor, profiler, diagnostics viewer |

---

## 📋 Documentation Health

**Validation:**
Run `python scripts/validate_docs.py` after editing to catch broken links.

**Maintenance:**
- Update this file when adding new directories or major documents.
- Keep module tables in sync with actual module state.
- Archive completed work to `archive/` after 30 days of inactivity.
- Reference [`../AGENTS.md#workflow-change-proposals`](../AGENTS.md#workflow-change-proposals) when adjusting the agent workflow so documentation updates ship atomically.

**Precedence for Conflicts:**
> **Note:** `../AGENTS.md` (the workflow portal at the root) now also contains the Agent Directory Workflow guidance referenced above. It remains the top-level authority in the precedence chain below.
`../AGENTS.md` → `NAVIGATION.md` (this file) → `ARCHITECTURE.md` → `design/` or `specs/` → module READMEs → code comments.

When a lower-precedence document conflicts with a higher one, update the higher-precedence document first, then cascade changes downstream.

---

**Last updated:** 2026-02-10 (Agent directory guidance merged into root portal)
