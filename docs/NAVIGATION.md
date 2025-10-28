# Documentation Navigation for AI Agents

## 🎯 Start Here Workflow

**First-time or general questions?**
1. Read [`../README.md`](../README.md) - workspace overview, build steps, module health
2. Read [`HYBRID_WORKFLOW.md`](HYBRID_WORKFLOW.md) - unified agentic workflow (best of both approaches)
3. Read [`../agents/AGENTS-INDEX.md`](../agents/AGENTS-INDEX.md) - role directory and quick start
4. Return here for specialized documentation

**Working on a specific task?**
1. Check [`ROADMAP.md`](ROADMAP.md) - is your work active, backlog, or blocked?
2. Find your module in [`modules/<name>/README.md`](modules/) - understand the subsystem
3. Read related specs in [`specs/`](specs/) - understand constraints
4. Follow [`prompts/IMPLEMENTATION-PLAYBOOK.md`](prompts/IMPLEMENTATION-PLAYBOOK.md)

**Need architectural context?**
1. [`ARCHITECTURE.md`](ARCHITECTURE.md) - module boundaries, data flow, invariants
2. [`specs/ADR-*.md`](specs/) - binding decisions

## 📁 Directory Guide

| Directory | Purpose | When to Use |
|-----------|---------|-------------|
| `specs/` | ADRs, RFPs - binding architectural decisions | Before designing new features |
| `design/` | Deep dives, guides, strategies | Understanding complex subsystems |
| `modules/` | Per-module README + roadmap | Working in specific subsystem |
| `tasks/` | Active sprint work, acceptance criteria | Implementing assigned work |
| `prompts/` | Reusable agent instruction templates | Standardizing AI workflows |
| `archive/` | Historical artifacts (prints, reviews, completed tasks) | Historical research only |

## 🔀 Hybrid Workflow Documents

| Document | Purpose | When to Use |
|----------|---------|-------------|
| [`../agents/AGENTS.md`](../agents/AGENTS.md) | Multi-agent operating manual | Coordinating hand-offs and context packs |
| [`HYBRID_WORKFLOW.md`](HYBRID_WORKFLOW.md) | Complete unified workflow guide | Understanding the full workflow |
| [`HYBRID_WORKFLOW_SUMMARY.md`](HYBRID_WORKFLOW_SUMMARY.md) | Quick reference and FAQ | Quick lookups, onboarding |
| [`HYBRID_WORKFLOW_DIAGRAM.md`](HYBRID_WORKFLOW_DIAGRAM.md) | Visual workflow diagram | Understanding flow and phases |
| [`CODEX_PROMPTING_GUIDE.md`](CODEX_PROMPTING_GUIDE.md) | **How to prompt AI agents** | **Starting any AI-assisted work** |
| [`WORKFLOW_COMPARISON.md`](WORKFLOW_COMPARISON.md) | Old vs new analysis | Understanding design decisions |
| [`AGENTIC_WORKFLOW_ENHANCEMENT.md`](AGENTIC_WORKFLOW_ENHANCEMENT.md) | Implementation log | Historical context |

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

CI also runs this check automatically on pushes and PRs; see `.github/workflows/docs-validate.yml`.

**Maintenance:**
- Update this file when adding new directories or major documents
- Keep module tables in sync with actual module state
- Archive completed work to `archive/` after 30 days of inactivity

**Precedence for Conflicts:**
`../AGENTS.md` → `NAVIGATION.md` (this file) → `ARCHITECTURE.md` → `design/` or `specs/` → module READMEs → code comments

When a lower-precedence document conflicts with a higher one, update the higher-precedence document first, then cascade changes downstream.

---

**Last updated:** 2026-01-08 (Added application readiness assessment)
