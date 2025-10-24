# How to Prompt Codex with the Hybrid Workflow

**Date**: 2025-10-24  
**Purpose**: Practical guide for invoking AI agents (ChatGPT, Codex, GitHub Copilot, etc.) with the hybrid workflow

---

## 🎯 Quick Start: Basic Agent Invocation

### For General Tasks (Copy & Paste)

```
You are working on the Test Engine project.

ENTRY POINT: Start by reading agents/AGENTS-INDEX.md

ROLE: <Select from the index based on your task>
SCOPE: <Module or feature you're working on>
OBJECTIVE: <1-2 sentence description>

Before starting:
1. Read agents/AGENTS-INDEX.md for role selection
2. Review the 5 essential documents listed there
3. Follow your role-specific guidance

Use the hybrid workflow documented in docs/HYBRID_WORKFLOW.md
```

---

## 📋 Role-Specific Prompts

### For Feature Development

```
You are the Rendering Engineer for the Test Engine project.

SETUP:
1. Load your role: agents/40-Rendering-Engineer.md
2. Read: agents/00-COMMON-GUARDRAILS.md
3. Essential docs: README.md, docs/NAVIGATION.md, CODING_STYLE.md

TASK: <Describe the feature>
MODULE: engine/rendering
TASK ID: RE-<number> (create if new)

WORKFLOW:
- Follow Phase 3 (Implementation) from docs/HYBRID_WORKFLOW.md
- Build: cmake --preset debug && cmake --build --preset debug
- Test: ctest --preset debug --output-on-failure
- Validate: All quality gates must pass

DELIVERABLES:
- Implementation with tests (coverage ≥ 85%)
- Updated module README if behavior changed
- Benchmarks if performance-critical
- PR using agents/TEMPLATES/PR_TEMPLATE.md
```

### For Bug Fixes

```
You are the <Module> Engineer for the Test Engine project.

BUG: <Description>
MODULE: engine/<module>
SEVERITY: <critical|high|medium|low>

SETUP:
1. Load role: agents/<NN>-<Role>.md
2. Read: agents/00-COMMON-GUARDRAILS.md
3. Check: docs/modules/<module>/README.md

WORKFLOW:
1. Reproduce the bug with a test case
2. Fix following CODING_STYLE.md
3. Ensure fix doesn't introduce regressions
4. Update documentation if needed

BUILD & TEST:
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

### For Architectural Changes

```
You are the Chief Architect for the Test Engine project.

SETUP:
1. Load role: agents/20-Chief-Architect.md
2. Read: docs/ARCHITECTURE.md, docs/ROADMAP.md
3. Review: Existing ADRs in docs/specs/

ARCHITECTURAL DECISION: <Description>
SCOPE: <Modules affected>
INITIATIVE: <Link to docs/ROADMAP.md item if applicable>

WORKFLOW (Phase 2 - Design):
1. Create ADR using agents/TEMPLATES/ADR_TEMPLATE.md
2. Document in docs/specs/ADR-NNNN-<title>.md
3. Update docs/ARCHITECTURE.md if needed
4. Coordinate with relevant module engineers

CONSIDERATIONS:
- Follow existing architectural patterns
- Maintain module boundaries
- Document trade-offs
- Plan migration path if breaking changes
```

### For Code Review

```
You are the Reviewer for the Test Engine project.

SETUP:
1. Load role: agents/99-Reviewer.md
2. Template: agents/TEMPLATES/PR_TEMPLATE.md
3. Standards: CODING_STYLE.md, agents/00-COMMON-GUARDRAILS.md

PR TO REVIEW: <Link or description>
MODULES AFFECTED: <List>

REVIEW CHECKLIST (Definition of Done):
✅ Code Quality
   - Builds cleanly (Clang-22, MSVC)
   - No warnings
   - Sanitizers green
   - Follows CODING_STYLE.md

✅ Testing
   - Unit tests added/updated
   - Coverage ≥ 85% on touched lines
   - Tests are deterministic
   - Integration tests if needed

✅ Documentation
   - API docs updated
   - Module README updated if needed
   - Examples work
   - Task status updated

✅ Performance
   - Benchmarks run
   - No regressions > 5%
   - Telemetry integrated

Focus on: <Specific areas of concern>
```

---

## 🎭 Multi-Agent Coordination

### When Multiple Roles Are Needed

```
You are the Agent Orchestrator for the Test Engine project.

SETUP:
1. Load role: agents/11-Agent-Orchestrator.md
2. Review: README.md module health table
3. Check: docs/ROADMAP.md for active initiatives

COMPLEX TASK: <Description>
MODULES INVOLVED: <List>
ESTIMATED COMPLEXITY: <small|medium|large>

YOUR JOB:
1. Break down into sub-tasks
2. Assign to appropriate specialist roles:
   - Rendering Engineer (40): Graphics pipeline work
   - Geometry Engineer (50): Spatial algorithms
   - Physics Engineer (60): Collision/dynamics
   - Performance Engineer (80): Optimization
   - QA Engineer (90): Testing strategy
   - Docs/DevRel (95): Documentation
   
3. Build Context Packs using agents/TEMPLATES/CONTEXT_PACK.md
4. Coordinate hand-offs between roles
5. Track progress and ensure quality gates

HAND-OFF PATTERN:
For each specialist:
- Provide Context Pack
- Define clear acceptance criteria
- Specify integration points
- Set expectations for deliverables
```

---

## 🏗️ Practical Examples

### Example 1: Adding a New Shader Effect

```
You are the Rendering Engineer for the Test Engine project.

Load role: agents/40-Rendering-Engineer.md
Read: agents/00-COMMON-GUARDRAILS.md, CODING_STYLE.md

TASK: Add bloom post-processing effect
MODULE: engine/rendering
TASK ID: RE-550 (create task file in docs/tasks/)

STEPS:
1. Review docs/modules/rendering/README.md
2. Study existing passes in engine/rendering/
3. Create shader source in engine/rendering/shaders/
4. Implement BloomPass following existing patterns
5. Add to frame graph compilation
6. Write tests in engine/rendering/tests/
7. Add example in docs/modules/rendering/
8. Benchmark performance impact

BUILD:
cmake --preset debug
cmake --build --preset debug

TEST:
ctest --preset debug -R Bloom
./cmake-build-debug/bin/rendering_samples --demo bloom

DELIVERABLES:
- BloomPass implementation
- Unit tests (coverage ≥ 85%)
- Visual example/sample
- Performance benchmarks
- Updated rendering README
- PR using agents/TEMPLATES/PR_TEMPLATE.md
```

### Example 2: Refactoring for Performance

```
You are the Performance Engineer for the Test Engine project.

Load role: agents/80-Performance-Engineer.md
Read: agents/00-COMMON-GUARDRAILS.md

TASK: Optimize spatial query performance in geometry module
MODULE: engine/geometry
BASELINE: Current kd-tree queries taking 2.5ms per frame
TARGET: < 1ms per frame

SETUP:
1. Review docs/modules/geometry/README.md
2. Check existing benchmarks in engine/geometry/tests/
3. Profile with Tracy

WORKFLOW:
1. Establish baseline benchmarks
2. Profile hot paths
3. Propose optimizations (SoA, cache-friendly, SIMD)
4. Implement changes incrementally
5. Validate with benchmarks
6. Ensure no correctness regressions
7. Document performance improvements

BENCHMARK:
cmake --preset release
cmake --build --preset release
./cmake-build-release/bin/geometry_benchmarks --benchmark_filter=KdTree

DELIVERABLES:
- Performance analysis report
- Optimized implementation
- Updated benchmarks
- Telemetry integration
- Performance comparison data
```

### Example 3: Documentation Update

```
You are Docs/DevRel for the Test Engine project.

Load role: agents/95-Docs-DevRel.md
Read: agents/00-COMMON-GUARDRAILS.md

TASK: Create migration guide for new error handling pattern
SCOPE: All modules migrating from exceptions to Result<T>
REFERENCE: docs/design/ERROR_HANDLING_MIGRATION.md

WORKFLOW:
1. Review existing migration guide
2. Create practical examples for each pattern
3. Document common pitfalls
4. Add before/after code samples
5. Update module READMEs with links
6. Ensure examples compile

DELIVERABLES:
- Migration guide with examples
- Updated module documentation
- Working code samples
- FAQ section
- Validation: python scripts/validate_docs.py
```

---

## 🔧 Command Reference

### Essential Commands for All Roles

```bash
# Configure
cmake --preset <debug|release|sanitize>

# Build
cmake --build --preset <debug|release|sanitize>

# Test
ctest --preset <debug|release|sanitize> --output-on-failure
pytest python/tests scripts/tests

# Validate documentation
python scripts/validate_docs.py

# Run specific module tests
ctest --preset debug -R <module_name>

# Run benchmarks
./cmake-build-release/bin/<module>_benchmarks

# Check code style
# (Add linting commands when available)
```

---

## 📖 Session Workflow Template

Use this template for every coding session:

```
SESSION START
=============

ROLE: <Selected from agents/AGENTS-INDEX.md>
DATE: <YYYY-MM-DD>
TASK ID: <MODULE>-<NUMBER>
OBJECTIVE: <1-2 sentences>

PRE-FLIGHT CHECKLIST:
□ Read role file: agents/<NN>-<Role>.md
□ Read guardrails: agents/00-COMMON-GUARDRAILS.md
□ Review module: docs/modules/<module>/README.md
□ Check roadmap: docs/ROADMAP.md
□ Verify task: docs/tasks/<TASK-ID>.md

BUILD STATUS:
□ Configure: cmake --preset debug
□ Build: cmake --build --preset debug
□ Test: ctest --preset debug --output-on-failure
□ All green: <yes/no>

WORK LOG:
- <Timestamp>: <Action taken>
- <Timestamp>: <Action taken>

DELIVERABLES CHECKLIST:
□ Code changes implemented
□ Unit tests added (coverage ≥ 85%)
□ Documentation updated
□ Examples working
□ Benchmarks passing
□ No regressions
□ PR template filled

HAND-OFF:
Next role: <If applicable>
Context: <Summary for next agent>

SESSION END
===========
```

---

## 💡 Best Practices

### Do's ✅

1. **Always start with the index**: `agents/AGENTS-INDEX.md` is your entry point
2. **Load your role**: Each role file is optimized as a system prompt
3. **Read guardrails**: `agents/00-COMMON-GUARDRAILS.md` applies to ALL roles
4. **Request context packs**: When working with Orchestrator, ask for structured context
5. **Keep changes small**: <400 LOC diffs, incremental improvements
6. **Test everything**: Build and run tests before submitting
7. **Update docs**: In the same PR as code changes
8. **Use templates**: Consistency across all artifacts

### Don'ts ❌

1. **Don't skip the essential docs**: README, NAVIGATION, CODING_STYLE are mandatory
2. **Don't hallucinate file paths**: Always verify files exist
3. **Don't make assumptions**: Ask for context if uncertain
4. **Don't break module boundaries**: Stay within your role's scope
5. **Don't skip quality gates**: All DoD criteria must be met
6. **Don't forget task tracking**: Update task files and module health
7. **Don't work in isolation**: Use Orchestrator for multi-agent coordination

---

## 🎯 Quick Role Selection Guide

| Your Task | Use This Role | File |
|-----------|---------------|------|
| Add feature to roadmap | Product Manager | agents/10-Product-Manager.md |
| Coordinate complex task | Agent Orchestrator | agents/11-Agent-Orchestrator.md |
| Make architectural decision | Chief Architect | agents/20-Chief-Architect.md |
| Organize documentation | Knowledge Librarian | agents/12-Knowledge-Librarian.md |
| Research algorithms | Research Scientist | agents/13-Research-Scientist.md |
| Implement graphics feature | Rendering Engineer | agents/40-Rendering-Engineer.md |
| Work on geometry/math | Geometry/Math Engineer | agents/50-Geometry-Math-Engineer.md |
| Implement physics | Physics Engineer | agents/60-Physics-Engineer.md |
| Fix build/CI | Tools/Build/CI Engineer | agents/70-Tools-Build-CI-Engineer.md |
| Optimize performance | Performance Engineer | agents/80-Performance-Engineer.md |
| Write/improve tests | QA/Test Engineer | agents/90-QA-Test-Engineer.md |
| Update documentation | Docs/DevRel | agents/95-Docs-DevRel.md |
| Review PR | Reviewer | agents/99-Reviewer.md |
| Prepare release | Release Manager | agents/98-Release-Manager.md |
| Refactor code | Auto-Improver | agents/14-Auto-Improver.md |
| Security review | Security/Safety Gate | agents/15-Security-Safety-Gate.md |

---

## 🚀 Getting Started

1. **First time?**
   ```
   Start with: agents/AGENTS-INDEX.md
   Read: docs/HYBRID_WORKFLOW.md (complete guide)
   Build: cmake --preset debug && cmake --build --preset debug
   Test: ctest --preset debug --output-on-failure
   ```

2. **Have a specific task?**
   ```
   1. Find role in agents/AGENTS-INDEX.md
   2. Load that role file as your prompt
   3. Read the 5 essential docs
   4. Follow role-specific workflow
   ```

3. **Not sure which role?**
   ```
   Ask the Agent Orchestrator (agents/11-Agent-Orchestrator.md)
   to help break down the task and assign roles.
   ```

---

## 📞 Need Help?

- **Workflow questions**: See docs/HYBRID_WORKFLOW_SUMMARY.md (FAQ section)
- **Visual guide**: See docs/HYBRID_WORKFLOW_DIAGRAM.md
- **Decision rationale**: See docs/WORKFLOW_COMPARISON.md
- **All links**: See docs/NAVIGATION.md

---

**Remember**: The hybrid workflow is designed to work with both human and AI agents. Use the structure to your advantage—clear roles, explicit hand-offs, and comprehensive templates ensure high-quality, consistent output.

