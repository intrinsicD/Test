---
id: RG-501
title: Frame graph compilation cache
status: new
priority: P1
area: rendering
size: M
owner: rendering-systems
gates: [tests, perf, docs]
relates_to: [bundle:rendering]
blocked_on: []
links:
  - docs/modules/rendering/README.md
  - docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md
---

# Task RG-501 — Frame Graph Compilation Cache

## Intent

Cache deterministic frame-graph compilation outputs so repeated graphs can reuse dependency ordering and barrier analysis, cutting repeated reset() CPU costs by 40–60% during research iterations.

---

## Context

**Current State:**
- `FrameGraph::compile()` recomputes dependency ordering, transient resource allocation, and barrier schedules every time `reset()` is invoked.
- Research experiments rebuild identical graphs per camera change, leading to avoidable CPU bubbles on render thread and delaying async streaming.
- Telemetry from PM-510 demos shows noticeable spikes when switching visualization modes even though graph topology is unchanged.

**Desired State:**
- Deterministic hash of pass/resource topology detects whether cached compilation artifacts remain valid.
- Cached execution order, pass barrier lists, and transient resource plans are restored instantly when graph topology and handles match.
- Cache invalidates automatically if passes/resources mutate, ensuring correctness without manual flushing.

**References:**
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md)
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [`design/RESOURCE_MANAGEMENT.md`](../../design/RESOURCE_MANAGEMENT.md)

---

## Design / Plan

### Constraints

- Follow `hybrid_workflow/CONTRIBUTING.md` coding standards and keep cache structures deterministic to avoid cross-platform divergence.
- Maintain frame graph invariants from `docs/modules/rendering/README.md`, including explicit lifetime tracking for transient resources.
- Respect telemetry budgets from PM-510 demos: cache lookup must be sub-microsecond and never add allocations on hot path.
- Update documentation/backlog/roadmap after behavior changes per AGENTS guidance.

### API / Data Sketch

```cpp
class FrameGraph {
  struct CompilationCache {
    std::size_t resource_hash{0};
    std::size_t pass_hash{0};
    std::vector<std::size_t> execution_order;
    std::vector<std::vector<resources::Barrier>> pass_begin_barriers;
    std::vector<std::vector<resources::Barrier>> pass_end_barriers;
  };

  std::optional<CompilationCache> cache_;

  [[nodiscard]] std::size_t compute_graph_hash() const;
  bool can_use_cache() const;
};
```

### Edge Cases & Failure Modes

- **Graph mutation between frames:** Hash mismatch should trigger full recompilation and overwrite cache.
- **Resource renaming but identical structure:** Hashing must include stable identifiers (IDs not string pointers) to prevent false positives.
- **Backend-specific capabilities:** Ensure cached barrier lists remain valid when backend toggles features; include backend capability bits in hash if necessary.

### Test Plan

- **Unit Tests:**
  - Verify compilation cache reuse when identical graphs compile twice.
  - Confirm cache invalidates when pass order or resource metadata changes.
  - Ensure cache handles backend capability toggles.
- **Integration Tests:**
  - Run rendering regression harness with repeated `reset()` calls and assert compile time shrinks.
- **Performance:**
  - Capture CPU timing telemetry before/after; target ≥40% reduction in `FrameGraph::compile()` for repeated graphs.
- **Regression Tests:**
  - Maintain existing frame graph tests; add metric guard verifying compile time stays within regression band.

### Tool Integration

**Profiling:**
- [ ] Instrument `FrameGraph::compile()` with `PROFILE_SCOPE` to capture cache hit/miss timing for evidence.
- [ ] Generate profiler report for PM-510 integration demo.

**Diagnostic UI:**
- [ ] Optionally log cache hit/miss counts via telemetry overlay (TL-314).

**Benchmark Automation:**
- [ ] Extend prototype harness scenario to toggle cache and capture compile timings.

**Configuration Management:**
- [ ] Surface cache enable toggle in runtime config for experimentation.

---

## Steps

1. [ ] Research hashing strategy; document invariants in this file.
2. [ ] Implement cache structures and hashing within `engine/rendering/src/frame_graph/frame_graph.cpp`.
3. [ ] Add unit tests in `engine/rendering/tests/frame_graph_tests.cpp` for cache hit/miss coverage.
4. [ ] Wire telemetry counters for cache hit rate into runtime diagnostics.
5. [ ] Update `docs/modules/rendering/README.md` with caching behavior and tunables.
6. [ ] Capture benchmark evidence (PM-510) proving compile-time reduction.
7. [ ] Validate docs via `python scripts/validate_docs.py` and update roadmap/backlog status.
8. [ ] Open PR referencing this task and attach evidence logs.

---

## Evidence

### Test Results

```bash
# Pending implementation
# cmake --preset linux-gcc-debug
# cmake --build --preset linux-gcc-debug
# ctest --preset linux-gcc-debug --output-on-failure
# pytest python/tests scripts/tests
# python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: _pending_
- Integration tests: _pending_
- Documentation validation: _pending_

### Performance (if applicable)

**Benchmark:** Frame graph compile micro-benchmark
- Before: _pending_
- After: _pending_
- Delta: _target ≥40% reduction_

**Artifacts:**
- Telemetry captures: _pending_
- Benchmark logs: _pending_

**Profiler Report:**
```
# Attach PROFILE_SCOPE output post-implementation
```

**Benchmark Automation:**
```
# Document prototype harness invocation here
```

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Compile + test logs |
| perf | [ ] | Performance | Benchmark results |
| docs | [ ] | Docs/DevRel | README + roadmap updates |
| safety | [ ] | Safety | N/A |
| release | [ ] | Release Mgr | N/A |

### Updated Files

- _pending_
