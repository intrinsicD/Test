---
id: RG-503
title: Frame-graph barrier optimization & batching
status: new
priority: P2
area: rendering
size: M
owner: rendering-systems
gates: [tests, perf]
relates_to: [bundle:rendering]
blocked_on: []
links:
  - docs/modules/rendering/README.md
  - design/RESOURCE_MANAGEMENT.md
---

# Task RG-503 — Frame-Graph Barrier Optimization & Batching

## Intent

Reduce redundant resource barriers during frame-graph compilation by batching compatible transitions and eliminating no-op state changes, cutting synchronization overhead 15–25% on complex graphs.

---

## Context

**Current State:**
- Barrier generation is conservative; every pass transition emits independent barriers even when states already match.
- GPU captures from PM-510 demos show back-to-back transitions between identical states wasting command buffer space.
- No instrumentation reports barrier counts per pass, making regressions hard to detect.

**Desired State:**
- Dedicated `BarrierOptimizer` examines per-pass barrier sets, merges compatible ones, and removes redundant transitions.
- Frame-graph telemetry reports total barriers before/after optimization to guide tuning.
- Implementation remains backend-agnostic while allowing future vendor-specific overrides.

**References:**
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md)
- [`design/RESOURCE_MANAGEMENT.md`](../../design/RESOURCE_MANAGEMENT.md)

---

## Design / Plan

### Constraints

- Maintain strict ordering guarantees for hazards; optimizer must never reorder across passes, only within per-pass begin/end lists.
- Keep algorithm deterministic; use stable sorting or preserve original order for non-mergeable barriers.
- Document heuristics and edge cases in module README per documentation checklist.

### API / Data Sketch

```cpp
class BarrierOptimizer {
 public:
  struct OptimizedBarriers {
    std::vector<resources::Barrier> barriers;
    std::uint32_t eliminated_count{0};
  };

  [[nodiscard]] OptimizedBarriers optimize(
      std::vector<resources::Barrier> barriers,
      const std::vector<ResourceState>& current_states);
 private:
  bool can_merge(const resources::Barrier& a, const resources::Barrier& b) const;
  bool is_redundant(const resources::Barrier& barrier, ResourceState current) const;
};
```

### Edge Cases & Failure Modes

- **Aliased resources:** Avoid merging barriers where resources alias to the same memory but different views; require metadata check.
- **Queue family transfers:** Preserve queue ownership transitions even if states match.
- **Debug builds:** Add assertions verifying optimized output is ≤ original count.

### Test Plan

- **Unit Tests:**
  - Validate redundant barriers are removed without altering required transitions.
  - Confirm queue transfer barriers remain intact.
- **Integration Tests:**
  - Run command encoder tests with optimizer enabled and verify draw correctness.
- **Performance:**
  - Capture GPU event counts showing ≥15% reduction in barrier submissions on complex graphs.
- **Regression Tests:**
  - Add telemetry guard ensuring optimizer output never exceeds input.

### Tool Integration

**Profiling:**
- [ ] Use GPU markers/timestamps to correlate barrier reductions with frame time improvements.

**Diagnostic UI:**
- [ ] Surface per-pass barrier stats via TL-312 metrics panel.

**Benchmark Automation:**
- [ ] Extend telemetry exporters to include barrier counts for PM-510 demos.

---

## Steps

1. [ ] Implement `BarrierOptimizer` helper in frame-graph module.
2. [ ] Integrate optimizer into `FrameGraph::compile()` for begin/end barrier vectors.
3. [ ] Add targeted unit tests covering merge/elimination cases.
4. [ ] Capture GPU profiling evidence via prototype harness.
5. [ ] Update documentation describing optimizer heuristics and toggles.
6. [ ] Run validation stack and document outcomes before PR.

---

## Evidence

### Test Results

```bash
# Pending implementation
# cmake --preset linux-gcc-debug
# cmake --build --preset linux-gcc-debug
# ctest --preset linux-gcc-debug --output-on-failure
# pytest python/tests scripts/tests
```

**Test Summary:**
- Unit tests: _pending_
- Integration tests: _pending_

### Performance (if applicable)

**Benchmark:** Barrier elimination
- Before: _pending_
- After: _pending_
- Delta: _target ≥15% fewer barriers_

**Artifacts:**
- Telemetry captures: _pending_
- Benchmark logs: _pending_

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Build + test logs |
| perf | [ ] | Performance | GPU profiling |
| docs | [ ] | Docs/DevRel | README updates |
| safety | [ ] | Safety | N/A |
| release | [ ] | Release Mgr | N/A |

### Updated Files

- _pending_
