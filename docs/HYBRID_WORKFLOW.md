# Hybrid Agentic Workflow

**Date**: 2025-10-24  
**Status**: ✅ Active  
**Purpose**: Unified workflow combining the best of specialized multi-agent roles with practical development guidance

---

## 🎯 Executive Summary

This hybrid workflow merges:
- **Old Workflow Strengths**: Single entry point, practical build guidance, clear task management, strong documentation integration
- **New Workflow Strengths**: Specialized roles, explicit coordination, quality gates, comprehensive templates, formalized processes

**Result**: A scalable, specialized multi-agent system with practical development workflow and clear entry points.

---

## 🚀 Quick Start (Choose Your Path)

### For AI Agents
1. **Load your role**: Start with [`agents/AGENTS-INDEX.md`](../agents/AGENTS-INDEX.md)
2. **Read context**: Review the 5 essential documents listed
3. **Get context pack**: Request from Agent Orchestrator using template
4. **Execute**: Follow your role-specific guidance
5. **Deliver**: Use templates and validate against Definition of Done

### For Human Contributors
1. **Orient**: Read [`README.md`](../README.md) for workspace overview
2. **Navigate**: Use [`NAVIGATION.md`](NAVIGATION.md) to find relevant docs
3. **Build**: Follow standard build workflow (cmake presets)
4. **Contribute**: Follow [`CODING_STYLE.md`](../CODING_STYLE.md)
5. **Review**: Check agent workflow for automation opportunities

---

## 📋 The Hybrid Approach

### Core Principles

| Principle | From | Implementation |
|-----------|------|----------------|
| **Single Entry Point** | Old | [`agents/AGENTS-INDEX.md`](../agents/AGENTS-INDEX.md) |
| **Role Specialization** | New | 15 specialized agent roles |
| **Practical Guidance** | Old | Build commands, coding standards in Common Guardrails |
| **Explicit Coordination** | New | Agent Orchestrator role |
| **Task Clarity** | Old | Module-based IDs, docs/tasks/ structure |
| **Quality Gates** | New | Security, Performance, QA, Reviewer roles |
| **Documentation Integration** | Old | NAVIGATION.md, module READMEs required |
| **Templates** | New | Comprehensive templates for consistency |

---

## 🏗️ Workflow Structure

### Phase 1: Discovery & Planning

**Entry Point**: Product Manager or human request

1. **Product Manager** ([`10-Product-Manager.md`](../agents/10-Product-Manager.md))
   - Reviews [`docs/ROADMAP.md`](ROADMAP.md) for alignment
   - Creates task in `docs/tasks/<MODULE>-<NUMBER>.md`
   - Uses module-based ID convention (RE-541, GE-221, etc.)
   - Defines acceptance criteria using template
   - Links to parent initiative/epic

2. **Agent Orchestrator** ([`11-Agent-Orchestrator.md`](../agents/11-Agent-Orchestrator.md))
   - Receives task assignment
   - Builds Context Pack using template
   - Routes to appropriate specialist(s)
   - Monitors progress and coordinates hand-offs

### Phase 2: Design & Architecture

**When Needed**: Architectural changes, new modules, API design

3. **Chief Architect** ([`20-Chief-Architect.md`](../agents/20-Chief-Architect.md))
   - Reviews architecture alignment
   - Creates ADR using [`TEMPLATES/ADR_TEMPLATE.md`](../agents/TEMPLATES/ADR_TEMPLATE.md)
   - Defines interfaces and contracts
   - Updates [`docs/ARCHITECTURE.md`](ARCHITECTURE.md)

4. **Research Scientist** ([`13-Research-Scientist.md`](../agents/13-Research-Scientist.md))
   - Evaluates algorithms and techniques
   - Creates prototypes and benchmarks
   - Writes RFPs for major technical decisions

### Phase 3: Implementation

**Required Reading Before Coding**:
1. [`README.md`](../README.md) - Module health, build workflow
2. [`docs/NAVIGATION.md`](NAVIGATION.md) - Documentation structure
3. [`CODING_STYLE.md`](../CODING_STYLE.md) - Standards
4. [`docs/ROADMAP.md`](ROADMAP.md) - Current priorities
5. [`agents/00-COMMON-GUARDRAILS.md`](../agents/00-COMMON-GUARDRAILS.md) - Universal rules

**Module-Specific Engineers**:
- **Tech Lead** ([`30-Tech-Lead.md`](../agents/30-Tech-Lead.md)) - Cross-module coordination
- **Rendering Engineer** ([`40-Rendering-Engineer.md`](../agents/40-Rendering-Engineer.md)) - Graphics pipeline
- **Geometry/Math Engineer** ([`50-Geometry-Math-Engineer.md`](../agents/50-Geometry-Math-Engineer.md)) - Spatial algorithms
- **Physics Engineer** ([`60-Physics-Engineer.md`](../agents/60-Physics-Engineer.md)) - Simulation

**Standard Workflow**:
```bash
# 1. Configure
cmake --preset <debug|release|sanitize>

# 2. Build
cmake --build --preset <debug|release|sanitize>

# 3. Test
ctest --preset <debug|release|sanitize> --output-on-failure
pytest python/tests scripts/tests

# 4. Validate docs
python scripts/validate_docs.py
```

### Phase 4: Quality Assurance

**Parallel Quality Gates**:

5. **QA/Test Engineer** ([`90-QA-Test-Engineer.md`](../agents/90-QA-Test-Engineer.md))
   - Validates test coverage ≥ 85%
   - Ensures deterministic tests
   - Reviews fuzz testing where applicable

6. **Performance Engineer** ([`80-Performance-Engineer.md`](../agents/80-Performance-Engineer.md))
   - Runs benchmarks
   - Checks for regressions
   - Validates telemetry integration

7. **Security/Safety Gate** ([`15-Security-Safety-Gate.md`](../agents/15-Security-Safety-Gate.md))
   - Security review
   - Dependency audit
   - Safety checks (sanitizers)

8. **Docs/DevRel** ([`95-Docs-DevRel.md`](../agents/95-Docs-DevRel.md))
   - Updates API documentation
   - Validates examples compile
   - Ensures module README current

### Phase 5: Review & Release

9. **Reviewer** ([`99-Reviewer.md`](../agents/99-Reviewer.md))
   - Validates Definition of Done
   - Checks all quality gates passed
   - Reviews using [`TEMPLATES/PR_TEMPLATE.md`](../agents/TEMPLATES/PR_TEMPLATE.md)

10. **Release Manager** ([`98-Release-Manager.md`](../agents/98-Release-Manager.md))
    - Manages versioning
    - Prepares changelogs
    - Coordinates releases

---

## 📂 Task Management (Hybrid Approach)

### Three-Tier Structure

| Type | Location | ID Format | Purpose |
|------|----------|-----------|---------|
| **Active Tasks** | `docs/tasks/` | Module-based (RE-541) | Sprint work, implementation |
| **Roadmap Items** | `docs/ROADMAP.md` | Initiative-based (DC-004, AI-002) | Strategic planning |
| **Community Issues** | GitHub Issues | Issue numbers | External contributions, bugs |

### Module Prefix Convention

| Prefix | Module | Example |
|--------|--------|---------|
| AN | Animation | AN-230 |
| AS | Assets | AS-150 |
| CO | Compute | CO-170 |
| CR | Core | CR-135 |
| GE | Geometry | GE-221 |
| IO | IO | IO-240 |
| MA | Math | MA-130 |
| PH | Physics | PH-430 |
| PL | Platform | PL-215 |
| RE | Rendering | RE-541 |
| RT | Runtime | RT-006 |
| SC | Scene | SC-230 |
| DC | Cross-cutting/Docs | DC-004 |
| AI | Architecture Initiative | AI-002 |

### Task Workflow

```
[Request] → [Product Manager creates task in docs/tasks/]
          → [Orchestrator assigns to specialist(s)]
          → [Implementation with quality gates]
          → [Reviewer validates]
          → [Task archived or closed]
```

---

## 🎭 Role Directory

### Strategic & Coordination
- **Product Manager** - Feature prioritization, backlog grooming
- **Agent Orchestrator** - Multi-agent coordination, context packs
- **Chief Architect** - Architectural decisions, ADRs

### Knowledge & Research
- **Knowledge Librarian** - Documentation organization, patterns
- **Research Scientist** - Algorithm research, prototypes

### Engineering (By Module)
- **Tech Lead** - Cross-module design, scaffolding
- **Rendering Engineer** - Graphics pipeline, shaders
- **Geometry/Math Engineer** - Spatial structures, algorithms
- **Physics Engineer** - Collision, dynamics, constraints

### Infrastructure
- **Tools/Build/CI Engineer** - CMake, CI, developer tooling
- **Performance Engineer** - Benchmarks, profiling
- **QA/Test Engineer** - Testing strategy, coverage

### Documentation & Community
- **Docs/DevRel** - API docs, tutorials, examples
- **Community Maintainer** - Issue triage, community support

### Governance
- **Auto-Improver** - Refactoring, tech debt
- **Security/Safety Gate** - Security reviews, audits
- **Reviewer** - PR reviews, DoD enforcement
- **Release Manager** - Versioning, releases

---

## 📚 Documentation Integration

### Always Reference

1. **Module READMEs**: `docs/modules/<module>/README.md` or `engine/<module>/README.md`
2. **Module Roadmaps**: Check for `docs/modules/<module>/ROADMAP.md`
3. **ADRs**: Review `docs/specs/ADR-*.md` for binding decisions
4. **Telemetry Schema**: `docs/design/TELEMETRY_SCHEMA.md`
5. **Error Handling**: `docs/design/ERROR_HANDLING_MIGRATION.md`

### Update in Same PR

- Module README if behavior changes
- API documentation
- Examples that demonstrate new features
- Task status in task file
- Module health table in [`README.md`](../README.md) if milestone reached

---

## ✅ Definition of Done (Unified)

Every PR must meet these criteria:

### Code Quality
- ✅ Builds cleanly on CI (Clang-22, MSVC)
- ✅ No compiler warnings (warnings-as-errors enabled)
- ✅ Sanitizers green (undefined behavior, memory leaks)
- ✅ Follows [`CODING_STYLE.md`](../CODING_STYLE.md)

### Testing
- ✅ Unit tests added/updated
- ✅ Test coverage ≥ 85% on touched lines
- ✅ All tests pass and are deterministic
- ✅ Integration tests if crossing module boundaries

### Documentation
- ✅ API docs updated (inline comments)
- ✅ Module README updated if behavior changed
- ✅ Examples compile and demonstrate feature
- ✅ Task file updated with status
- ✅ ADR created if architectural decision made

### Performance
- ✅ Benchmarks run (no regressions > 5%)
- ✅ Telemetry integrated where applicable
- ✅ Tracy zones around hot paths

### Review
- ✅ Code reviewed by Reviewer role
- ✅ All quality gates passed
- ✅ No outstanding review comments
- ✅ PR template completed

---

## 🎯 Success Metrics

### Workflow Effectiveness
- **Time to first contribution**: < 30 minutes from clone to test PR
- **Agent autonomy**: ≥ 80% of tasks completed without human intervention
- **Documentation freshness**: 100% of PRs update docs
- **Hand-off success**: Zero lost context between role transitions

### Quality Metrics
- **DoD compliance**: 100% of merged PRs meet Definition of Done
- **Performance regressions**: < 5% of PRs need perf fixes
- **Test coverage**: ≥ 85% on all touched code
- **Documentation accuracy**: Zero broken links (validated by CI)

### Velocity Metrics
- **PR cycle time**: < 48 hours from submission to merge
- **Rework rate**: < 15% of PRs need major revisions
- **Automation rate**: ≥ 70% of tasks handled by AI agents

---

## 🔄 Continuous Improvement

### Feedback Loops

1. **Auto-Improver** ([`14-Auto-Improver.md`](../agents/14-Auto-Improver.md))
   - Monitors workflow metrics
   - Proposes process improvements
   - Refactors documentation based on usage

2. **Community Maintainer** ([`16-Community-Maintainer.md`](../agents/16-Community-Maintainer.md))
   - Gathers contributor feedback
   - Identifies friction points
   - Improves onboarding

### Retrospectives

After each milestone:
- Review success metrics
- Identify bottlenecks
- Update workflow documentation
- Share learnings in team retrospective doc

---

## 🛠️ Templates Available

All templates in [`agents/TEMPLATES/`](../agents/TEMPLATES/):

- **ADR_TEMPLATE.md** - Architecture Decision Records
- **CONTEXT_PACK.md** - AI agent context transfer
- **ISSUE_TEMPLATE.md** - GitHub issues and feature requests
- **PR_TEMPLATE.md** - Pull request format
- **TASK_CARD.md** - Detailed task specifications

---

## 📖 Additional Resources

### Workflow Guides
- [`WORKFLOW_COMPARISON.md`](WORKFLOW_COMPARISON.md) - Old vs New comparison
- [`AGENTIC_WORKFLOW_ENHANCEMENT.md`](AGENTIC_WORKFLOW_ENHANCEMENT.md) - Enhancement details
- [`agents/17-Example-Session.md`](../agents/17-Example-Session.md) - Example workflow session

### Architecture
- [`ARCHITECTURE.md`](ARCHITECTURE.md) - System architecture
- [`ROADMAP.md`](ROADMAP.md) - Strategic roadmap
- [`GLOSSARY.md`](GLOSSARY.md) - Terms and definitions

### Development
- [`CODING_STYLE.md`](../CODING_STYLE.md) - C++20 and Python standards
- [`CMakePresets.json`](../CMakePresets.json) - Build configurations
- [`README.md`](../README.md) - Workspace snapshot

---

## 🎓 Learning Path

### Week 1: Orientation
- Day 1-2: Read essential documents (README, NAVIGATION, CODING_STYLE)
- Day 3-4: Explore module structure and READMEs
- Day 5: Build project, run tests, validate docs

### Week 2: First Contribution
- Day 1-2: Pick a "good first issue" or small task
- Day 3-4: Implement following workflow
- Day 5: Submit PR, iterate on feedback

### Week 3: Specialization
- Day 1-2: Deep dive into specific module
- Day 3-4: Study related ADRs and design docs
- Day 5: Propose improvement or new feature

### Ongoing
- Monitor assigned module's health
- Participate in code reviews
- Contribute to documentation
- Share knowledge with community

---

## 💡 Key Takeaways

1. **Start with AGENTS-INDEX.md** - Single entry point for orientation
2. **Follow your role** - Specialized guidance for your task type
3. **Use templates** - Consistency across all artifacts
4. **Validate quality** - Multiple gates ensure excellence
5. **Document everything** - In same PR as code changes
6. **Measure success** - Metrics guide continuous improvement
7. **Collaborate explicitly** - Orchestrator coordinates hand-offs
8. **Improve continuously** - Workflow evolves based on feedback

---

**This hybrid workflow provides the best of both worlds**: specialized agent roles with practical development guidance, enabling both human and AI contributors to work effectively together.

