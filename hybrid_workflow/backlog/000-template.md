---
id: NNN
title: Short imperative title
status: new            # new | ready | in_progress | review | done | archived
priority: P1           # P0 | P1 | P2 | P3
area: rendering        # rendering | geometry | runtime | tools | docs | infra | ...
size: M                # XS | S | M | L | XL
owner: unassigned      # or agent/person identifier
gates: [tests]         # add: perf | docs | safety | release
relates_to: [bundle:A] # bundle tags from ROADMAP.md
blocked_on: []         # ["dependency description or task ID"]
links: []              # ["PR #123", "ADR-0004", "docs/specs/..."]
---

# Task NNN — Short Imperative Title

## Intent

<!-- One sentence: what value does this deliver? -->

Deliver [capability/fix/improvement] so that [user/system] can [outcome/benefit].

---

## Context

<!-- Brief background: why now? What problem exists? -->

**Current State:**
- Describe the current situation or limitation

**Desired State:**
- Describe the target outcome after completion

**References:**
- Link to relevant ADRs, design docs, module READMEs
- Cite Context Ladder items (see hybrid_workflow/AGENTS.md §Context Ladder)

---

## Design / Plan

### Constraints

- Follow `hybrid_workflow/CONTRIBUTING.md` coding standards
- Maintain architectural invariants from `docs/ARCHITECTURE.md`
- Preserve determinism and data-oriented layouts (SoA) in hot paths
- Keep documentation/backlog/roadmap synchronized with behavior
- Respect telemetry budgets and performance targets (≤2% regression unless mitigated)

### API / Data Sketch

```cpp
// (Optional) Sketch key types, functions, or data structures

namespace engine::module {
  struct NewType {
    // fields
  };
  
  Result<Foo, ErrorCode> DoSomething(Args args);
}
```

### Edge Cases & Failure Modes

- **Case 1:** [Describe edge case] → [Mitigation strategy]
- **Case 2:** [Describe failure mode] → [Handling approach]

### Test Plan

- **Unit Tests:** What proves correctness?
  - Test X validates Y
  - Test Z covers edge case A
  
- **Integration Tests:** What proves end-to-end behavior?
  - Scenario X exercises path Y
  
- **Performance (if `perf` gate set):**
  - Dataset: [which dataset]
  - Target: [e.g., ≥2× speedup, p95 < X ms]
  - Baseline: [current performance]
  
- **Regression Tests:** What ensures no future breakage?
  - Metric X must not degrade
  - Test Y guards against previous bug

### Tool Integration

<!-- Select tools that apply to this task from engine/tools/ -->

**Profiling (for performance-critical code):**
- [ ] Use `PROFILE_SCOPE("SectionName")` macro for timing
- [ ] Generate profiler report for evidence section
- [ ] Identify hot paths and optimization opportunities

**Diagnostic UI (for runtime/editor features):**
- [ ] Use `render_diagnostics()` for runtime health visualization
- [ ] Use `render_validation_report()` for scene validation UI
- [ ] Use `render_profiler_window()` for performance visualization
- [ ] Register custom panels with `PanelRegistry` (if applicable)

**Benchmark Automation (for comparative testing):**
- [ ] Use `PrototypeHarnessBenchmarkRunner` for headless benchmarks
- [ ] Use `ComparativeBenchmarkRunner` for engine vs reference testing
- [ ] Configure via `ExperimentSandbox` for interactive workflows

**Configuration Management (for prototyping workflows):**
- [ ] Load experiment configs with `load_summary_from_json()`
- [ ] Validate dataset manifests and asset checksums
- [ ] Wire configuration callbacks to sandbox UI

**References:**
- Full tool documentation: `docs/modules/tools/README.md`
- Integration patterns: `hybrid_workflow/CONTRIBUTING.md` §Diagnostic Tools
- Available tools inventory: `hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md`

---

## Steps

<!-- Ordered implementation steps; update as work progresses -->

1. [ ] Research/design: Load context, review ADRs, document approach
2. [ ] Implement core functionality in `engine/<module>/src/`
3. [ ] Add unit tests in `engine/<module>/tests/`
4. [ ] Update module README with new capabilities
5. [ ] Add integration tests or samples
6. [ ] Run performance benchmarks (if `perf` gate)
7. [ ] Update documentation (docs/modules/, NAVIGATION.md)
8. [ ] Create PR and request review

---

## Evidence

<!-- Capture validation results, benchmarks, test outputs, logs -->

### Test Results

```bash
# Paste command outputs here after testing
# cmake --preset linux-gcc-debug
# cmake --build --preset linux-gcc-debug
# ctest --preset linux-gcc-debug
# pytest python/tests scripts/tests
# python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: [X passed / Y total]
- Integration tests: [X passed / Y total]
- Documentation validation: [pass/fail]

### Performance (if applicable)

**Benchmark:** [benchmark name]
- Before: [baseline metric]
- After: [new metric]
- Delta: [improvement/regression %]

**Artifacts:**
- Telemetry captures: `telemetry/[filename]`
- Benchmark logs: `[path or attachment]`

**Profiler Report (if using engine::tools::profiling):**
```
# Example profiler output
PhysicsUpdate: avg=2.341ms, min=1.890ms, max=3.120ms, calls=1000
RenderSubmission: avg=8.123ms, min=7.340ms, max=10.230ms, calls=1000
AssetLoading: avg=15.670ms, min=12.100ms, max=22.340ms, calls=50
```

**Benchmark Automation (if using benchmark runners):**
```bash
# PrototypeHarnessBenchmarkRunner output
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --dataset remesh-sample --frame-count 1000 \
    --summary-json telemetry/benchmark_result.json

# Or ComparativeBenchmarkRunner output
python scripts/benchmarks/run_comparative_benchmarks.py \
    --config benchmarks/scenario.json \
    --output-dir telemetry/comparative/
```

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Test output above |
| perf | [ ] | Performance | Benchmark section above |
| docs | [ ] | Docs/DevRel | Updated files list below |
| safety | [ ] | Safety | Sanitizer logs, security checklist |
| release | [ ] | Release Mgr | Changelog, packaging notes |

### Updated Files

<!-- List documentation/code files changed -->

- `engine/<module>/include/...`
- `engine/<module>/src/...`
- `engine/<module>/tests/...`
- `docs/modules/<module>/README.md`
- `docs/NAVIGATION.md` (if new docs added)

---

## Completion Checklist (Definition of Done)

- [ ] All implementation steps above completed
- [ ] Tests added and passing (unit + integration)
- [ ] Performance targets met and recorded (if `perf` gate)
- [ ] Documentation updated (module READMEs, navigation, ADRs if needed)
- [ ] Code follows `CONTRIBUTING.md` standards (formatting, naming, error handling)
- [ ] PR created referencing this task file
- [ ] All quality gates signed off (see table above)
- [ ] ROADMAP.md bundle checkbox updated
- [ ] Cross-links validated: `python scripts/validate_docs.py`
- [ ] Task metadata updated: `status: done`

---

## Result

**PR:** [link to pull request]  
**SHA:** [merge commit hash]  
**Completion Date:** YYYY-MM-DD

**Notes:**

<!-- Capture anything notable for future reference -->
- Key decisions made during implementation
- Deviations from original plan and rationale
- Follow-up tasks spawned (create new task files)
- Lessons learned or process improvements

**Follow-ups:**

- [ ] [Follow-up task description] → Create task NNN+1

---

## Role Coordination (Optional - for complex tasks)

<!-- For simple tasks, skip this section -->
<!-- For complex tasks, populate from agents/ROLES.md -->

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Agent Orchestrator | | Phase coordination, blocker resolution | |
| Product Manager | | Scope, acceptance criteria | |
| Knowledge Librarian | | Context package, documentation | |
| Specialist Engineer(s) | | Implementation | |
| Docs/DevRel | | Documentation updates | |
| QA/Test Specialist | | Test validation | |
| Performance Engineer | | Benchmark validation | |
| Safety Reviewer | | Security/safety review | |
| Reviewer | | Code review | |
| Release Manager | | Release preparation | |

**Escalation Path:**  
[Define if needed - see agents/ROLES.md for standard paths]

**Additional Artifacts Created:**  
<!-- Link if complex task requires separate documents -->
- Task Brief: `agents/task_briefs/NNN-task-name.md`
- Context Package: `agents/context_packages/NNN-task-name.md`
- Quality Report: `agents/quality_reports/NNN-task-name.md`

---

_Template version: 1.0 (2025-11-04)_

