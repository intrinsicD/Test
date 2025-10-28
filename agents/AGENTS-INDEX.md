# Agentic Workflow Index

**Welcome to the 3D Engine Multi-Agent Development Workflow**

This is your **single entry point** for AI-driven collaboration on the engine project. Whether you're a human contributor or an AI agent, start here.

---

**📖 Full Workflow Guide:** See [docs/HYBRID_WORKFLOW.md](../docs/HYBRID_WORKFLOW.md) for the complete hybrid workflow that merges specialized roles with practical development guidance.

## 🎯 Quick Start (30 seconds)

1. **Read this index** (you are here)
2. **Review the workspace**: [README.md](../README.md) - module health, build presets, current status
3. **Understand navigation**: [docs/NAVIGATION.md](../docs/NAVIGATION.md) - documentation structure
4. **Skim the operations manual**: [AGENTS.md](AGENTS.md) - how roles coordinate and hand-offs work
5. **Pick your role** below based on your task

---

## 📋 Essential Reading (Always Start Here)

Before working on any task, review these documents in order:

1. **[README.md](../README.md)** - Workspace snapshot, module health table, build workflow
2. **[docs/NAVIGATION.md](../docs/NAVIGATION.md)** - Documentation map, task routing, common workflows
3. **[CODING_STYLE.md](../CODING_STYLE.md)** - C++20 and Python coding standards
4. **[docs/ROADMAP.md](../docs/ROADMAP.md)** - Architecture improvement plan, active initiatives
5. **[00-COMMON-GUARDRAILS.md](00-COMMON-GUARDRAILS.md)** - Rules that apply to ALL roles

---

## 🏗️ Build & Test Workflow

### Configure
```bash
cmake --preset <debug|release|sanitize>
```

### Build
```bash
cmake --build --preset <debug|release|sanitize>
```

### Test
```bash
ctest --preset <debug|release|sanitize> --output-on-failure
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

**See [README.md](../README.md) section 3** for complete build instructions and preset details.

---

## 👥 Choose Your Role

### 🎭 Strategic Roles

| Role | File | When to Use |
|------|------|-------------|
| **Product Manager** | [10-Product-Manager.md](10-Product-Manager.md) | Prioritizing features, creating roadmap items, defining user value |
| **Agent Orchestrator** | [11-Agent-Orchestrator.md](11-Agent-Orchestrator.md) | Coordinating multi-agent tasks, building context packs, routing work |
| **Chief Architect** | [20-Chief-Architect.md](20-Chief-Architect.md) | Making architectural decisions, writing ADRs, defining interfaces |

### 🔬 Knowledge & Research

| Role | File | When to Use |
|------|------|-------------|
| **Knowledge Librarian** | [12-Knowledge-Librarian.md](12-Knowledge-Librarian.md) | Organizing docs, maintaining patterns, creating examples |
| **Research Scientist** | [13-Research-Scientist.md](13-Research-Scientist.md) | Evaluating new algorithms, prototyping techniques, writing RFCs |

### 💻 Engineering Roles (By Module)

| Role | File | Scope | When to Use |
|------|------|-------|-------------|
| **Tech Lead** | [30-Tech-Lead.md](30-Tech-Lead.md) | Cross-module | Leading module development, design docs, scaffolding |
| **Rendering Engineer** | [40-Rendering-Engineer.md](40-Rendering-Engineer.md) | `engine::rendering` | Graphics pipelines, shaders, frame graphs |
| **Geometry/Math Engineer** | [50-Geometry-Math-Engineer.md](50-Geometry-Math-Engineer.md) | `engine::geometry`, `engine::math` | Spatial structures, algorithms, math primitives |
| **Physics Engineer** | [60-Physics-Engineer.md](60-Physics-Engineer.md) | `engine::physics` | Collision, dynamics, constraints |

### 🛠️ Infrastructure & Quality

| Role | File | When to Use |
|------|------|-------------|
| **Tools/Build/CI Engineer** | [70-Tools-Build-CI-Engineer.md](70-Tools-Build-CI-Engineer.md) | CMake, CI pipelines, developer tooling |
| **Performance Engineer** | [80-Performance-Engineer.md](80-Performance-Engineer.md) | Benchmarks, profiling, regression detection |
| **QA/Test Engineer** | [90-QA-Test-Engineer.md](90-QA-Test-Engineer.md) | Testing strategy, coverage, fuzz testing |

### 📚 Documentation & Community

| Role | File | When to Use |
|------|------|-------------|
| **Docs/DevRel** | [95-Docs-DevRel.md](95-Docs-DevRel.md) | API docs, tutorials, examples, migration guides |
| **Community Maintainer** | [16-Community-Maintainer.md](16-Community-Maintainer.md) | Issue triage, contributor support, community health |

### 🔒 Governance & Quality Gates

| Role | File | When to Use |
|------|------|-------------|
| **Auto-Improver** | [14-Auto-Improver.md](14-Auto-Improver.md) | Refactoring, code quality improvements, tech debt |
| **Security/Safety Gate** | [15-Security-Safety-Gate.md](15-Security-Safety-Gate.md) | Security reviews, dependency audits, safety checks |
| **Reviewer** | [99-Reviewer.md](99-Reviewer.md) | PR reviews, enforcing Definition of Done |
| **Release Manager** | [98-Release-Manager.md](98-Release-Manager.md) | Versioning, releases, changelogs, artifacts |

---

## 🚀 Quick Workflow Guide

### For AI Agents

1. **Load the appropriate role file** as your system prompt
2. **Request a CONTEXT PACK** from the Orchestrator (use [TEMPLATES/CONTEXT_PACK.md](TEMPLATES/CONTEXT_PACK.md))
3. **Follow the Codex execution loop**: Read → Plan → Patch → Test → Report
4. **Use templates** for issues, ADRs, PRs, and task cards

### For Human Contributors

1. **Review [AGENTS-QUICKSTART.md](AGENTS-QUICKSTART.md)** for the end-to-end workflow
2. **Check the module health table** in [README.md](../README.md)
3. **Find or create a task** in [docs/backlog/active/](../docs/backlog/active/) or GitHub Issues
4. **Follow the Definition of Done** in [00-COMMON-GUARDRAILS.md](00-COMMON-GUARDRAILS.md)

---

## 📝 Templates

Use these templates for consistent work products:

- **[ISSUE_TEMPLATE.md](TEMPLATES/ISSUE_TEMPLATE.md)** - Creating GitHub issues or task files
- **[ADR_TEMPLATE.md](TEMPLATES/ADR_TEMPLATE.md)** - Architecture Decision Records
- **[CONTEXT_PACK.md](TEMPLATES/CONTEXT_PACK.md)** - Context for AI agents
- **[TASK_CARD.md](TEMPLATES/TASK_CARD.md)** - Detailed task specifications
- **[PR_TEMPLATE.md](TEMPLATES/PR_TEMPLATE.md)** - Pull request format

---

## 📊 Task Management

### Where Tasks Live

**Internal Tasks** (for active development):
- Location: `docs/backlog/active/`
- Format: `<MODULE>-<NUMBER>.md` (e.g., `RE-541`, `GE-221`)
- Tracking: Module health table in [README.md](../README.md)

**External/Community Tasks**:
- Location: GitHub Issues
- Labels: `bug`, `feature`, `enhancement`, `good first issue`
- Board: GitHub project board

**Roadmap/Epics**:
- Location: [docs/ROADMAP.md](../docs/ROADMAP.md)
- Format: Initiative IDs (e.g., `DC-001`, `AI-002`)

### Task Lifecycle

```
Roadmap → Product Manager → Issue/Task File → Orchestrator → Context Pack → 
Role Agent → Implementation → Reviewer → Merge → Release Manager
```

See [AGENTS-QUICKSTART.md](AGENTS-QUICKSTART.md) for the complete lifecycle.

---

## 🎯 Definition of Done (All Roles)

Every PR must satisfy:

- ✅ **Builds** cleanly on CI (Clang-22/libc++, MSVC 19.3x)
- ✅ **Tests** pass: unit, integration, sanitizers (ASan, UBSan)
- ✅ **Coverage** ≥ 85% on touched lines
- ✅ **Performance** regression ≤ 2% vs. baseline
- ✅ **Documentation** updated: API docs, examples, READMEs
- ✅ **Coding style** follows [CODING_STYLE.md](../CODING_STYLE.md)
- ✅ **No warnings** with `-Werror`
- ✅ **Logging & tracing** instrumented (spdlog, Tracy)

---

## 🏛️ Repository Structure

```
.
├── README.md                 # Workspace snapshot, module health, build workflow
├── AGENTS.md                 # (Legacy) Single-file workflow guidance
├── CODING_STYLE.md           # C++20 and Python standards
├── CMakeLists.txt            # Root build configuration
├── CMakePresets.json         # Build presets (debug, release, sanitize)
├── agents/                   # Multi-agent workflow definitions
│   ├── AGENTS-INDEX.md       # ← You are here
│   ├── AGENTS-QUICKSTART.md  # Quick workflow guide
│   ├── AGENTS.md             # Workflow operating manual
│   ├── 00-COMMON-GUARDRAILS.md
│   ├── 10-Product-Manager.md
│   ├── 11-Agent-Orchestrator.md
│   ├── ...                   # Other role files
│   └── TEMPLATES/            # Issue, ADR, PR, Context Pack templates
├── docs/                     # Design docs, specs, tasks, roadmap
│   ├── NAVIGATION.md         # Documentation map (start here!)
│   ├── ROADMAP.md            # Architecture improvement plan
│   ├── ARCHITECTURE.md       # System overview
│   ├── modules/              # Per-module design docs
│   ├── tasks/                # Active task files
│   ├── specs/                # ADRs and RFPs
│   └── ...
├── engine/                   # C++ modules
│   ├── animation/
│   ├── rendering/
│   ├── geometry/
│   └── ...
├── python/                   # Python bindings and tools
├── scripts/                  # Build, CI, validation automation
└── third_party/              # Vendored dependencies
```

**Full hierarchy**: See [README.md](../README.md) or run `tree -L 2` in the workspace root.

---

## 🔄 Workflow Variants

### Starting a New Feature
1. Product Manager: Create issue from [TEMPLATES/ISSUE_TEMPLATE.md](TEMPLATES/ISSUE_TEMPLATE.md)
2. Chief Architect: Write ADR from [TEMPLATES/ADR_TEMPLATE.md](TEMPLATES/ADR_TEMPLATE.md)
3. Orchestrator: Build context pack, assign to Tech Lead/Engineer
4. Engineer: Implement following role guidance
5. Reviewer + Performance + QA: Validate
6. Merge and create follow-ups

### Fixing a Bug
1. QA Engineer: Reproduce, write failing test
2. Orchestrator: Create context pack with test + relevant code
3. Engineer: Fix implementation
4. Reviewer: Verify fix and test coverage
5. Merge

### Refactoring/Tech Debt
1. Auto-Improver: Identify smell, propose refactor
2. Architect: Approve approach
3. Engineer: Implement with performance parity
4. Performance Engineer: Verify no regression
5. Merge

---

## 📖 Additional Resources

- **[AGENTS.md](AGENTS.md)** - Operating manual covering coordination loops, hand-offs, and shared tooling
- **[AGENTS-QUICKSTART.md](AGENTS-QUICKSTART.md)** - Step-by-step workflow guide
- **[README.md](README.md)** - Brief overview of the agent system
- **[17-Example-Session.md](17-Example-Session.md)** - Example task execution

---

## 🆘 Getting Help

**Unclear where to start?**
1. Read [AGENTS-QUICKSTART.md](AGENTS-QUICKSTART.md)
2. Check [docs/NAVIGATION.md](../docs/NAVIGATION.md) for documentation routing
3. Ask the Agent Orchestrator to build you a context pack

**Missing information?**
1. Check [README.md](../README.md) workspace snapshot
2. Review relevant module README in `docs/modules/<name>/`
3. Search for ADRs in `docs/specs/`

**Task unclear?**
1. Review acceptance criteria in the task file or issue
2. Request a context pack from the Orchestrator
3. Consult the Tech Lead for the relevant module

---

**Last Updated**: 2025-10-24

**Maintained by**: Agent Orchestrator, Knowledge Librarian

