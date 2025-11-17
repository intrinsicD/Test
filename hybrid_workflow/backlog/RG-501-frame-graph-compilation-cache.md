---
id: RG-501
title: Frame graph compilation cache
status: review
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

### Cache Lifecycle

1. **Hashing:** Combine backend capability bits, pass dependency signatures, and transient resource descriptors into a 128-bit
   hash (two `std::size_t` lanes) using xxHash seeded from the runtime build ID. This prevents collisions across different GPU
   feature sets or ABI toggles.
2. **Lookup:** On `reset()`, compute the hash and compare against the cached artifact. A quick pointer/size check guards against
   stale allocations before deeper verification.
3. **Validation:** Confirm the cached execution order length matches the live pass count and that each transient resource entry
   resolves to a currently registered handle. Failures trigger a full compile and cache overwrite.
4. **Restoration:** When validation succeeds, repopulate execution ordering, begin/end barrier tables, and transient heap
   reservations directly from the cache without touching the expensive dependency solver.
5. **Invalidation:** Any mutation to pass metadata, resource lifetime, or backend toggles marks the cache dirty. A diagnostic
   counter increments so telemetry surfaces invalidation frequency during PM-510 demos.

### Implementation Notes

- Extend `FrameGraph::compile()` with a lightweight `CacheLookupScope` helper so cache probes remain encapsulated and easy to
  instrument.
- Store cached barrier vectors in POD arenas owned by the frame graph to avoid heap churn on cache hits.
- Serialize cache hit/miss counters through the runtime telemetry bridge to unblock TL-314 overlays from surfacing hit-rate
  regressions.

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

0. [x] Scope cache design, hash inputs, and cache lifecycle so the task can progress from `status: new` to `status: ready`.
1. [x] Research hashing strategy; document invariants in this file.
2. [x] Implement cache structures and hashing within `engine/rendering/src/frame_graph/frame_graph.cpp`.
3. [x] Add unit tests in `engine/rendering/tests/frame_graph_tests.cpp` for cache hit/miss coverage.
4. [x] Wire telemetry counters for cache hit rate into runtime diagnostics.
5. [x] Update `docs/modules/rendering/README.md` with caching behavior and tunables.
6. [x] Capture benchmark evidence (PM-510) proving compile-time reduction.
7. [x] Validate docs via `python scripts/validate_docs.py` and update roadmap/backlog status.
8. [ ] Open PR referencing this task and attach evidence logs.

---

## Evidence

### Test Results

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target engine_runtime_tests
ctest --preset linux-gcc-debug -R engine_runtime_tests
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: `engine_runtime_tests`
- Integration tests: _pending_
- Documentation validation: `python scripts/validate_docs.py`
- Full evidence captured on 2025-11-16:
  - `cmake --preset linux-gcc-debug`
  - `cmake --build --preset linux-gcc-debug --target engine_runtime_tests`
  - `ctest --preset linux-gcc-debug -R engine_runtime_tests`
  - `pytest python/tests scripts/tests`
  - `python scripts/validate_docs.py`

### Performance (if applicable)

**Benchmark:** Frame graph compile micro-benchmark (`engine_rendering_frame_graph_benchmark`)

```
./out/build/linux-gcc-debug/engine/rendering/benchmarks/engine_rendering_frame_graph_benchmark \
  --passes 16 --iterations 64 \
  --output out/build/linux-gcc-debug/engine/rendering/benchmarks/frame_graph_cache_benchmark.json
```

**Results (64 iterations, 16 passes):**
- Cache miss avg: **84.04 µs** (min 65.97 µs, max 412.80 µs)
- Cache hit avg: **29.17 µs** (min 24.78 µs, max 70.30 µs)
- Average speedup: **2.88×** (≈65% compile-time reduction on hits)

Artifacts stored alongside the build directory JSON capture for PM-510 import.

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [x] | QA/Test | `cmake --preset linux-gcc-debug`, `cmake --build --preset linux-gcc-debug --target engine_runtime_tests`, `ctest --preset linux-gcc-debug -R engine_runtime_tests`, `pytest python/tests scripts/tests` |
| perf | [x] | Performance | `engine_rendering_frame_graph_benchmark` evidence (16 passes × 64 iterations) |
| docs | [x] | Docs/DevRel | `python scripts/validate_docs.py`, module/telemetry README updates |
| safety | [ ] | Safety | N/A |
| release | [ ] | Release Mgr | N/A |

### Updated Files

- `engine/rendering/include/engine/rendering/frame_graph.hpp`
- `engine/rendering/src/frame_graph.cpp`
- `engine/rendering/tests/test_frame_graph.cpp`
- `docs/modules/rendering/README.md`
- `engine/rendering/CMakeLists.txt`
- `engine/rendering/benchmarks/CMakeLists.txt`
- `engine/rendering/benchmarks/frame_graph_cache_benchmark.cpp`
