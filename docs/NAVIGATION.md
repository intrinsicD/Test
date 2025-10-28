# Documentation Navigation for AI Agents

## 🎯 Start Here Workflow

**First-time or general questions?**
1. Read [`../README.md`](../README.md) – workspace overview, build steps, module health.
2. Read [`HYBRID_WORKFLOW.md`](HYBRID_WORKFLOW.md) – canonical hybrid workflow for humans + AI agents.
3. Read [`../agents/AGENTS-INDEX.md`](../agents/AGENTS-INDEX.md) – pick your role and load its guidance.
4. Return here for specialised documentation and directory references.

**Working on a specific task?**
1. Check [`ROADMAP.md`](ROADMAP.md) – confirm the milestone and priority band.
2. Open the relevant backlog entry under [`backlog/active/`](backlog/active/) – it defines ownership, definition of done, dependencies, and artefacts.
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
| `modules/` | Per-module README + roadmap | Working in specific subsystem |
| `prompts/` | Reusable agent instruction templates | Standardising AI workflows |
| `specs/` | ADRs, RFPs – binding architectural decisions | Before designing new features |
| `examples/` | Usage samples and tutorials | Learning API usage |
| `archive/` | Historical artefacts (completed tasks, reviews) | Research and provenance |

## 🔀 Workflow References

| Document | Purpose | When to Use |
|----------|---------|-------------|
| [`../agents/AGENTS.md`](../agents/AGENTS.md) | Operating manual for multi-agent coordination | Coordinating hand-offs and context packs |
| [`HYBRID_WORKFLOW.md`](HYBRID_WORKFLOW.md) | Full hybrid workflow (single authoritative doc) | Understanding the collaboration model |
| [`HYBRID_WORKFLOW_SUMMARY.md`](HYBRID_WORKFLOW_SUMMARY.md) | Quick reference & FAQ | Fast refresh on responsibilities |
| [`WORKFLOW_COMPARISON.md`](WORKFLOW_COMPARISON.md) | Old vs new workflow analysis | Historical reasoning |
| [`CODEX_PROMPTING_GUIDE.md`](CODEX_PROMPTING_GUIDE.md) | Prompting best practices | Starting AI-assisted work |

## 🔧 Common Tasks

### Implementing a Feature
→ [`prompts/IMPLEMENTATION-PLAYBOOK.md`](prompts/IMPLEMENTATION-PLAYBOOK.md)

### Reviewing Code
→ [`prompts/REVIEW-CHECKLIST.md`](prompts/REVIEW-CHECKLIST.md)

### Refactoring
→ [`prompts/REFACTOR-PLAYBOOK.md`](prompts/REFACTOR-PLAYBOOK.md)

### Architecture Audit
→ [`prompts/ARCHITECTURE-AUDIT.md`](prompts/ARCHITECTURE-AUDIT.md)

## 📊 Key References

- **Coding Standards:** [`../CODING_STYLE.md`](../CODING_STYLE.md)
- **Telemetry:** [`design/TELEMETRY_SCHEMA.md`](design/TELEMETRY_SCHEMA.md), [`design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](design/TELEMETRY_INSTRUMENTATION_GUIDE.md)
- **Error Handling:** [`design/ERROR_HANDLING_MIGRATION.md`](design/ERROR_HANDLING_MIGRATION.md)
- **Resource Management:** [`design/RESOURCE_MANAGEMENT.md`](design/RESOURCE_MANAGEMENT.md)
- **Backlog Template:** [`backlog/README.md`](backlog/README.md)
- **Architecture Reviews:** [`reviews/2025-10-26-architecture-audit.md`](reviews/2025-10-26-architecture-audit.md), [`reviews/2025-12-05-roadmap-direction-review.md`](reviews/2025-12-05-roadmap-direction-review.md), [`reviews/2026-01-08-application-readiness-assessment.md`](reviews/2026-01-08-application-readiness-assessment.md)

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

**Precedence for Conflicts:**
`../AGENTS.md` → `NAVIGATION.md` (this file) → `ARCHITECTURE.md` → `design/` or `specs/` → module READMEs → code comments.

When a lower-precedence document conflicts with a higher one, update the higher-precedence document first, then cascade changes downstream.

---

**Last updated:** 2026-01-08 (Restructured backlog + workflow references)
