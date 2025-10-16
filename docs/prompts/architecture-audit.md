# Architecture Audit Prompt

Use this prompt to perform deep architectural reviews of the Test Engine codebase. This is distinct from individual PR reviews and focuses on system-wide consistency, debt accumulation, and alignment with architectural goals.

## When to Use
- Quarterly architecture health checks
- Before major milestone releases
- When onboarding new contributors
- After merging multiple related PRs
- When architectural drift is suspected

## Scope Options

### Full System Audit
Review entire codebase against architectural principles

### Module-Focused Audit
Deep dive into specific module(s) and their interactions

### Cross-Cutting Concern Audit
Examine how a theme (e.g., error handling, resource lifetime) is handled across modules

### Dependency Audit
Validate module boundaries and dependency graph integrity

## Prompt Template
```
You are conducting an architectural audit of the Test Engine repository.

**Audit Scope:** [Full System | Module: {names} | Theme: {concern} | Dependencies]

**Phase 1: Establish Baseline**
1. Read `docs/README.md` → `docs/architecture.md` → `docs/agents.md` in sequence
2. Review the architecture improvement plan in `docs/ROADMAP.md`
3. Read all specs in `docs/specs/` to understand binding decisions
4. Map the dependency graph from the root README build target inventory

**Phase 2: Invariant Verification**

For each architectural invariant in `docs/architecture.md`:

**Deterministic Scheduler:**
- [ ] Frame-graph compilation is deterministic for identical inputs
- [ ] Resource transitions are explicit and validated
- [ ] Backends cannot reorder passes
- [ ] Test: `frame_graph_serialization_is_deterministic` exists and passes

**Resource Ownership (AI-001):**
- [ ] All asset handles use `ResourceHandle<Tag>`
- [ ] Handles backed by `ResourcePool` with generation counters
- [ ] Invalid handle access caught in debug builds
- [ ] Test: `stale_handle_detection` exists and passes

**Geometry Fidelity:**
- [ ] Spatial structures (kd-tree, octree) sync with mesh mutations
- [ ] Bounds and centroid updated before publishing
- [ ] IO round-trips preserve topology
- [ ] Test: `geometry_roundtrip_preserves_checksum` exists and passes

**Physics Integration:**
- [ ] Mass/damping clamped to valid ranges
- [ ] Static bodies (mass ≤ 0) ignore forces
- [ ] Substep progression is monotonic
- [ ] Collision detection delegates to geometry predicates
- [ ] Test: `physics_static_bodies_ignore_gravity` exists and passes

**Documentation Discipline:**
- [ ] Every module has README following `docs/README_TEMPLATE.md`
- [ ] Roadmap items reference architectural decisions
- [ ] ADRs exist for major subsystem designs

**Phase 3: Module Boundary Analysis**

For each module in `engine/`:

**Module:** {name}

**Public API Surface:**
- Headers in `include/engine/{module}/`: [list]
- Exported symbols: [count from `nm` or equivalent]
- API stability: [stable | evolving | experimental]

**Dependencies:**
- Direct dependencies: [from CMakeLists.txt]
- Transitive dependencies: [from build graph]
- Unexpected dependencies: [violations of layering]

**Internal Complexity:**
- Lines of code: [from `cloc`]
- Cyclomatic complexity: [if available]
- Test coverage: [% if available]

**Documentation Health:**
- README complete: [yes/no]
- Roadmap aligned with central plan: [yes/no]
- API examples present: [yes/no]
- Migration guides for breaking changes: [yes/no]

**Phase 4: Cross-Cutting Concern Analysis**

**Error Handling (DC-004):**
For each module, audit:
- [ ] Public APIs return `Result<T, Error>` (not exceptions)
- [ ] Module-specific error enums defined
- [ ] `[[nodiscard]]` applied to Result-returning functions
- [ ] Error paths have test coverage

Document violations:
- Module: {name}
- Function: {signature}
- Issue: {description}
- Recommendation: {fix}

**Resource Lifetime (AI-001):**
For each module managing resources:
- [ ] Uses `ResourcePool` with typed handles
- [ ] Handle validation before access
- [ ] Lifecycle documented in README
- [ ] Debug assertions catch violations

Document violations:
- Module: {name}
- Resource: {type}
- Issue: {description}
- Recommendation: {fix}

**Telemetry & Diagnostics (CC-001):**
For performance-critical paths:
- [ ] Profiling scopes present
- [ ] Telemetry hooks exposed
- [ ] Metrics documented
- [ ] Overhead characterized

Document gaps:
- Module: {name}
- Path: {function/loop}
- Missing: {scope/metric}
- Recommendation: {instrumentation}

**Phase 5: Dependency Graph Validation**

Build a complete module dependency graph:
```
                    ┌──────────┐
                    │  Runtime │
                    └────┬─────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
    ┌────▼────┐     ┌───▼────┐     ┌───▼────┐
    │Animation│     │Physics │     │Rendering│
    └────┬────┘     └───┬────┘     └───┬─────┘
         │              │              │
         │         ┌────▼────┐    ┌────▼─────┐
         │         │Geometry │    │ Assets   │
         │         └────┬────┘    └────┬─────┘
         │              │              │
         └──────────────┼──────────────┘
                        │
                   ┌────▼────┐
                   │  Math   │
                   └─────────┘
```

**Validate:**
- [ ] No circular dependencies
- [ ] Longest dependency chain: [count] hops
- [ ] High-fanout modules: [list modules with >5 dependents]
- [ ] Isolated modules: [list modules with no dependents]

**Identify:**
- Unexpected dependencies: [violations]
- Missing abstraction layers: [where interfaces would help]
- Overly coupled modules: [candidates for refactoring]

**Phase 6: Architecture Improvement Plan Progress**

For each item in `docs/ROADMAP.md#architecture-improvement-plan`:

**Item:** {ID} - {Title}
- **Status:** [Not Started | In Progress | Completed]
- **Priority:** {HIGH | MEDIUM | LOW}
- **Dependencies:** {list}
- **Completion:** {X/Y tasks done}
- **Blockers:** {issues preventing progress}
- **Drift:** {divergence from plan}

**Critical Design Corrections (DC-*):**
- Total items: {count}
- Completed: {count}
- Blocked: {count}
- At risk: {count}

**Architecture Improvements (AI-*):**
- Total items: {count}
- Completed: {count}
- Blocked: {count}
- At risk: {count}

**Roadmap TODOs (RT-*):**
- Total items: {count}
- Completed: {count}
- Blocked: {count}
- At risk: {count}

**Phase 7: Technical Debt Assessment**

**Code Smells:**
- [ ] God objects (>1000 LOC classes)
- [ ] Long functions (>50 LOC)
- [ ] Deep nesting (>4 levels)
- [ ] High cyclomatic complexity (>15)
- [ ] Duplicated code (>50 LOC blocks)

**Architectural Debt:**
- [ ] Missing abstractions
- [ ] Leaky abstractions
- [ ] Inappropriate coupling
- [ ] Missing documentation
- [ ] Inadequate test coverage

**Process Debt:**
- [ ] Missing task records
- [ ] Outdated ADRs
- [ ] Stale roadmap items
- [ ] Unclosed follow-up issues

**Output Format:**

# Architecture Audit Report
**Date:** {YYYY-MM-DD}
**Scope:** {Full System | Module | Theme | Dependencies}
**Auditor:** {name/AI}

## Executive Summary
[2-3 paragraphs highlighting overall health, major concerns, recommendations]

## Invariant Compliance
[Table showing pass/fail for each invariant with evidence]

| Invariant | Status | Evidence | Issues |
|-----------|--------|----------|--------|
| Deterministic Scheduler | ✅ PASS | Tests passing | None |
| Resource Ownership | ⚠️ PARTIAL | 2/12 modules non-compliant | Animation, Scene |
| Geometry Fidelity | ✅ PASS | Round-trip tests pass | None |
| ... | ... | ... | ... |

## Module Health Matrix
[Health score for each module: 🟢 Healthy | 🟡 Needs Attention | 🔴 Critical]

| Module | API Stability | Dependencies | Tests | Docs | Overall |
|--------|--------------|--------------|-------|------|---------|
| Animation | 🟢 Stable | 🟢 Clean | 🟢 85% | 🟢 Complete | 🟢 |
| Assets | 🟡 Evolving | 🟢 Clean | 🟡 65% | 🟡 Partial | 🟡 |
| Compute | 🟢 Stable | 🔴 CUDA hardcoded | 🟢 80% | 🟢 Complete | 🟡 |
| ... | ... | ... | ... | ... | ... |

## Dependency Graph Analysis
[Visual diagram + analysis of coupling, longest chains, violations]

**Longest Chain:** Runtime → Assets → IO → Geometry → Math (5 hops)
**Circular Dependencies:** None ✅
**High Fanout:** Math (8 dependents), Core (6 dependents)
**Violations:** [list unexpected dependencies]

## Cross-Cutting Concern Compliance

### Error Handling (DC-004)
**Adoption Rate:** 60% (3/5 modules migrated)
- ✅ IO module: Complete
- ✅ Geometry module: Complete  
- ✅ Assets module: Complete
- ❌ Animation module: TODO
- ❌ Physics module: TODO

### Resource Lifetime (AI-001)
**Adoption Rate:** 75% (9/12 resource types)
- ✅ Meshes, Point Clouds, Graphs, Shaders, Textures
- ❌ Materials (descriptor-only)
- ❌ Animation clips (not pooled)
- ❌ Physics bodies (not pooled)

## Improvement Plan Progress
**Overall Completion:** {X}%

**Critical Design Corrections:** {X/Y} complete
- 🔴 Blocked: {items}
- ⚠️ At Risk: {items}
- ✅ Complete: {items}

**Architecture Improvements:** {X/Y} complete
- 🔴 Blocked: {items}
- ⚠️ At Risk: {items}
- ✅ Complete: {items}

## Technical Debt Assessment
**Total Debt Items:** {count}
**High Priority:** {count}
**Medium Priority:** {count}
**Low Priority:** {count}

### Top 5 Debt Items
1. {Description} - Priority: {HIGH|MED|LOW} - Module: {name}
2. ...

## Recommendations

### Immediate Actions (0-2 weeks)
1. {Specific action} - Rationale: {why} - Owner: {who}
2. ...

### Short-Term Actions (2-6 weeks)
1. {Specific action} - Rationale: {why} - Owner: {who}
2. ...

### Long-Term Actions (2-3 months)
1. {Specific action} - Rationale: {why} - Owner: {who}
2. ...

## Follow-Up Tasks
- [ ] Create task records for immediate actions
- [ ] Update roadmap priorities based on findings
- [ ] Schedule follow-up audit in {X} weeks
- [ ] Communicate findings to module owners

## Appendices

### A: Test Coverage by Module
[Detailed breakdown if available]

### B: Complexity Metrics
[Cyclomatic complexity, LOC, function counts]

### C: Documentation Gaps
[Missing READMEs, ADRs, migration guides]

### D: Build Time Analysis
[Module build times, bottlenecks]
```

## Audit Frequency Recommendations

- **Full System:** Quarterly or before major releases
- **Module-Focused:** Monthly for active modules
- **Cross-Cutting:** After completing improvement plan items
- **Dependency:** Monthly or when adding modules

## Tools and Automation

### Static Analysis
```
# Check for circular dependencies
cmake --graphviz=deps.dot .
dot -Tpng deps.dot -o deps.png

# Count lines of code
cloc engine/ --by-file

# Find complex functions
# Use complexity tools like lizard or pmccabe
```

### Test Coverage
```
# Generate coverage report (if configured)
cmake --preset linux-gcc-debug-coverage
cmake --build --preset linux-gcc-debug-coverage
ctest --preset linux-gcc-debug-coverage
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Documentation Validation
```
# Check for broken links
python scripts/validate_docs.py

# Find missing READMEs
find engine/ -type d -mindepth 1 ! -path "*/tests/*" ! -name "tests" \
  -exec test ! -e {}/README.md \; -print
```

### Dependency Graph Generation
```
# Generate full dependency graph
cmake --graphviz=full.dot -B build
# Filter to engine modules only
grep "engine_" full.dot > engine_deps.dot
dot -Tpng engine_deps.dot -o engine_deps.png
```

## Example Audit Findings

### Finding: Resource Lifetime Inconsistency
**Severity:** 🟡 Medium
**Modules:** Animation, Physics
**Issue:** Animation clips and physics bodies use raw pointers/indices instead of
generational handles, creating potential for stale references.
**Evidence:**
- `engine/animation/include/engine/animation/api.hpp:45` - raw `uint32_t` IDs
- `engine/physics/api.hpp:67` - body indices without generation

**Recommendation:**
1. Introduce `ClipHandle` and `BodyHandle` backed by `ResourcePool`
2. Migrate storage in Animation and Physics modules
3. Add debug validation similar to asset handles
4. Update tests to attempt stale access

**Task:** Create `T-0125-animation-physics-handle-migration.md`

### Finding: CUDA Hard Dependency
**Severity:** 🔴 High
**Modules:** Compute, Runtime
**Issue:** CUDA is mandatory even for CPU-only builds, preventing lean deployments
**Evidence:**
- `engine/compute/CMakeLists.txt:15` - unconditional `find_package(CUDA)`
- `engine/runtime/CMakeLists.txt:23` - always links `engine_compute_cuda`

**Recommendation:**
1. Implement DC-002 from improvement plan
2. Add `ENGINE_ENABLE_CUDA` CMake option
3. Create dispatcher abstraction shared by CPU/CUDA
4. Add CPU-only CI preset

**Task:** Already tracked as DC-002 in roadmap

### Finding: Missing Integration Tests
**Severity:** 🟡 Medium
**Modules:** Runtime, Rendering
**Issue:** No end-to-end tests for runtime → frame graph → backend submission
**Evidence:**
- `engine/tests/integration/` exists but is empty
- Module tests are isolated, no cross-module scenarios

**Recommendation:**
1. Create integration test framework in `engine/tests/integration/`
2. Add test: tick runtime → build frame graph → submit to mock backend
3. Add test: load asset → bind to handle → reference in rendering
4. Document integration test patterns

**Task:** Tracked as TI-001 in roadmap

## Post-Audit Actions

1. **Triage Findings:**
   - Critical (🔴): Create immediate action tasks
   - Warning (⚠️): Add to sprint backlog
   - Info (💡): Document for future consideration

2. **Update Plans:**
   - Adjust roadmap priorities based on findings
   - Update module ROADMAPs with debt items
   - Create ADRs for architectural decisions

3. **Communicate:**
   - Share report with module owners
   - Discuss in architecture review meeting
   - Update docs/architecture.md if invariants changed

4. **Track Progress:**
   - Schedule follow-up audit
   - Monitor improvement plan completion
   - Celebrate wins (debt paid down, invariants restored)
```
