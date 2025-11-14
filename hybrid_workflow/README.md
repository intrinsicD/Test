# Hybrid Workflow

> **Mission:** Ship high-priority tasks safely, fast, and to spec with minimal overhead and maximum automation.

This directory contains the **hybrid workflow system** — a streamlined task management approach that combines:

- **Simplicity** from lightweight, metadata-driven task tracking
- **Quality** from structured gates and role coordination
- **Automation** from parseable task frontmatter and tooling
- **Flexibility** from optional artifacts for complex work

---

## Quick Start

1. **Read the workflow:** [`AGENTS.md`](./AGENTS.md) — 7-step task lifecycle
2. **Understand task detection:** [`WORKFLOW_DETECTION.md`](./WORKFLOW_DETECTION.md) — how the workflow selects what to work on
3. **Check coding standards:** [`CONTRIBUTING.md`](./CONTRIBUTING.md) — naming, formatting, testing
4. **Load the coding micro-agent spec:** [`CODING_MICRO_AGENT_SPEC.md`](./CODING_MICRO_AGENT_SPEC.md) — MAKER-style guardrails for incremental changes
5. **Review available tools:** [`TOOLS_REFERENCE.md`](./TOOLS_REFERENCE.md) — engine tools quick reference
6. **Plan migrations:** [`MIGRATION.md`](./MIGRATION.md) — deterministic process for porting legacy tasks
7. **Pick a task:** [`ROADMAP.md`](./ROADMAP.md) — bundles and priorities
8. **Use the template:** [`backlog/000-template.md`](./backlog/000-template.md) — task file structure

### Sample Dashboard Snapshot

- View the latest generated dashboard artifacts under [`assets/hybrid_workflow_dashboard/`](../assets/hybrid_workflow_dashboard/).
- Regenerate the snapshot locally with:

  ```bash
  python -m scripts.workflow.dashboard --output-dir "$(pwd)/assets/hybrid_workflow_dashboard"
  ```

  Adjust the output directory path if your checkout lives elsewhere.

---

## Key Concepts

### Task Lifecycle

```
new → ready → in_progress → review → done → archived
```

All tasks follow this state machine. Status is tracked in frontmatter and drives automation.

### Metadata-Driven Tasks

Every task uses YAML frontmatter for structured metadata:

```yaml
---
id: NNN
title: Short imperative title
status: ready
priority: P1
area: rendering
size: M
owner: agent
gates: [tests, perf]
relates_to: [bundle:A]
blocked_on: []
links: []
---
```

This enables:
- ✅ Automated status dashboards
- ✅ Programmatic queries (grep, scripts)
- ✅ ROADMAP synchronization
- ✅ Dependency tracking

### Quality Gates

Tasks declare required gates in metadata: `gates: [tests, perf, docs, safety, release]`

Each gate has an owner role and specific criteria. Simple tasks record evidence inline; complex tasks use separate Quality Reports.

### Flexible Complexity

- **Simple tasks:** Use task file only, inline evidence
- **Medium tasks:** Add optional Task Brief for coordination
- **Complex tasks:** Full artifact suite (Task Brief, Context Package, Quality Report)

---

## Directory Structure

```
hybrid_workflow/
├── AGENTS.md              # Workflow guide (7 steps)
├── CONTRIBUTING.md        # Coding standards and conventions
├── ROADMAP.md             # Bundle-based priorities
├── README.md              # This file
└── backlog/
    ├── 000-template.md    # Task file template
    ├── NNN-task-name.md   # Active tasks
    └── archive/           # Completed tasks
```

---

## Goals

1. **Reduce overhead** — Single task file for most work, not 3-4 separate documents
2. **Enable automation** — Structured metadata for status tracking, queries, dashboards
3. **Maintain quality** — Explicit gates with role ownership and evidence requirements
4. **Preserve flexibility** — Optional artifacts for complex coordination needs
5. **Support scalability** — Works for solo contributors and teams

---

## Features

### From the Streamlined Workflow
- ✅ Clear 7-step process
- ✅ Visual lifecycle: `new → ready → in_progress → review → done → archived`
- ✅ Metadata-driven task files (automation-friendly)
- ✅ Bundle-based roadmap organization
- ✅ Concise templates (<50 lines per task)

### From the Comprehensive Workflow
- ✅ Context Ladder (systematic reference loading)
- ✅ Role coordination for complex tasks
- ✅ Quality gate framework with sign-offs
- ✅ Escalation paths and conflict resolution
- ✅ Integration with existing documentation system

---
## Available Tools

The engine provides reusable tools to accelerate task implementation and evidence collection. See [`TOOLS_REFERENCE.md`](./TOOLS_REFERENCE.md) for quick examples.

### Performance Tools
- **Profiler** (`engine/tools/profiling/profiler.hpp`) — `PROFILE_SCOPE` macro for timing hot paths
- **Benchmark Runners** — Automated headless and comparative benchmarking
- Generate profiler reports for `perf` gate evidence

### Diagnostic Tools
- **ImGui Helpers** (`engine/tools/imgui_helpers.hpp`) — Runtime diagnostics visualization
- **Panel Registry** — Reusable UI panels for editor/tooling (see TL-310)
- Render diagnostics, validation reports, and profiler windows

### Prototyping Tools
- **Experiment Sandbox** (`engine/tools/sandbox/`) — Interactive AI-004 prototyping UI
- **Configuration Loader** — JSON config parsing with validation
- Dataset browser, benchmark triggers, telemetry visualization

**Quick Reference:** [`TOOLS_REFERENCE.md`](./TOOLS_REFERENCE.md)  
**Full Documentation:** [`../docs/modules/tools/README.md`](../docs/modules/tools/README.md)  
**Integration Guide:** [`CONTRIBUTING.md`](./CONTRIBUTING.md) §Diagnostic Tools & Performance

---


## Workflow Integration

The hybrid workflow integrates with existing project infrastructure:

- **Documentation:** Links to `docs/NAVIGATION.md`, `docs/ROADMAP.md`, module READMEs
- **Roles:** Uses `agents/ROLES.md` for complex task coordination
- **Templates:** Leverages `agents/TEMPLATES/` for Task Briefs, Context Packages, Quality Reports
- **Validation:** Runs `python scripts/validate_docs.py` for cross-link checking
- **Build System:** Uses canonical CMake presets and test commands

---

## Migration Path

### For New Tasks
- Create task file using `backlog/000-template.md`
- Follow 7-step workflow in `AGENTS.md`
- Reference bundles in `ROADMAP.md`

### For Existing Tasks
- Migrate opportunistically (when updating or revisiting)
- Convert to metadata format with frontmatter
- Preserve context and history
- Move completed tasks to `backlog/archive/`

### For Complex Work
- Start with task file
- Add Task Brief if multi-role coordination needed
- Create Context Package if research-heavy
- Generate Quality Report if formal gate sign-offs required

---

## Automation Examples

### Status Summary Script
```bash
# Human-readable table (default)
python -m scripts.workflow.report_hybrid_status

# Machine-readable summary
python -m scripts.workflow.report_hybrid_status --format json

# Filter by roadmap bundle metadata
python -m scripts.workflow.report_hybrid_status --relates-to bundle:C
python -m scripts.workflow.report_hybrid_status --relates-to bundle:A bundle:D
```

### Task CLI Automation
```bash
# Machine-readable backlog snapshot
python hybrid_workflow/task_status.py --format json

# Summary counts for dashboards
python hybrid_workflow/task_status.py --format json --summary

# Detailed metadata for a specific task
python hybrid_workflow/task_status.py --format json --detail TL-310
```

### Generate Task Dashboard
```bash
python -m scripts.workflow.dashboard --output-dir build/hybrid-dashboard
```

This command writes `index.html` and `tasks.json` containing the current task
snapshot. Publish the HTML artifact to share progress during roadmap or PM-510
demos.

### Query Ready Tasks
```bash
grep -l "^status: ready" hybrid_workflow/backlog/*.md
```

### List High-Priority Work
```bash
grep -l "^priority: P1" hybrid_workflow/backlog/*.md | xargs head -n 15
```

### Find Blocked Tasks
```bash
grep -B 5 "^blocked_on: \[" hybrid_workflow/backlog/*.md
```

### Generate Status Report
```bash
for file in hybrid_workflow/backlog/*.md; do
  echo "=== $(basename $file) ==="
  grep "^status:" $file
  grep "^priority:" $file
  grep "^owner:" $file
  echo
done
```

---

## Tooling Roadmap

Future automation planned:

- [ ] **Task dashboard web UI** — Visual status board from metadata
- [ ] **Automated ROADMAP sync** — Update bundle checkboxes from task status
- [ ] **Slack/notification integration** — Alert on status changes
- [ ] **Dependency graph visualization** — Show `blocked_on` relationships
- [ ] **Metrics collection** — Time in each state, completion velocity

See [`backlog/archive/TL-320-task-dashboard.md`](./backlog/archive/TL-320-task-dashboard.md) for details.

---

## Comparison with Previous Workflows

| Aspect | Current (Root) | Proposed (Workflow) | **Hybrid** |
|--------|----------------|---------------------|------------|
| **Complexity** | High (200+ lines) | Low (60 lines) | **Medium (100 lines)** |
| **Task Format** | Free-form MD | Frontmatter + MD | **Frontmatter + MD** |
| **Overhead** | 3-4 files/task | 1 file/task | **1 file/task, optional extras** |
| **Automation** | Manual | High potential | **High (metadata-driven)** |
| **Role Coordination** | Always | Never | **Optional (as needed)** |
| **Quality Gates** | Implicit | Explicit metadata | **Explicit metadata + roles** |
| **Context Loading** | Formal ladder | Informal | **Formal ladder** |
| **Flexibility** | Low | Medium | **High (scales with complexity)** |

---

## Best Practices

1. **Start simple** — Use task file only, add artifacts if complexity grows
2. **Update metadata** — Change status as work progresses
3. **Record evidence** — Capture test outputs, benchmarks, validation results inline
4. **Link liberally** — Reference ADRs, PRs, docs in `links:` field
5. **Use bundles** — Group related tasks with `relates_to:` tags
6. **Block explicitly** — Document dependencies in `blocked_on:` field
7. **Archive completed** — Move done tasks to preserve history

---

## Support

- **Workflow questions:** See [`AGENTS.md`](./AGENTS.md)
- **Code standards:** See [`CONTRIBUTING.md`](./CONTRIBUTING.md)
- **Task template:** See [`backlog/000-template.md`](./backlog/000-template.md)
- **Role coordination:** See [`../agents/ROLES.md`](../agents/ROLES.md)
- **Artifacts:** See [`../agents/TEMPLATES/`](../agents/TEMPLATES/)

---

_Workflow version: 1.0 (2025-11-04)_

