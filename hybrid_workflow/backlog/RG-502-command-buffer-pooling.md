---
id: RG-502
title: Command buffer pooling and trimming
status: ready
priority: P1
area: rendering
size: M
owner: rendering-systems
gates: [tests, perf]
relates_to: [bundle:rendering]
blocked_on: []
links:
  - docs/modules/rendering/README.md
  - docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md
---

# Task RG-502 — Command Buffer Pooling and Trimming

## Intent

Adopt a reusable command buffer pool shared across queue types so command encoder reuse eliminates per-frame allocations and reduces submission latency by 20–30%.

---

## Context

**Current State:**
- Native scheduler hands out fresh command buffers per submission and returns them after execution, but underlying native handles are recreated each time.
- Vulkan and OpenGL backends accumulate transient allocations, increasing CPU overhead when TL-314 telemetry samples flood the pipeline.
- No trimming strategy exists, so idle handles accumulate.

**Desired State:**
- `CommandBufferPool` groups buffers per queue, tracks last-frame usage, and reclaims stale buffers via `trim_unused()`.
- `NativeSchedulerBase` and derived backends request buffers via pool to ensure handles are recycled.
- Telemetry counters expose pool hit rate and trimmed counts for diagnostics.

**References:**
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md)
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)

### Context Ladder Review (2026-05-27)

- ✅ `docs/ROADMAP.md` / [`hybrid_workflow/ROADMAP.md`](../ROADMAP.md): confirms RG-502 is the top P1 rendering follow-up once
  T-0119/T-0120 shipped.
- ✅ [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md): documents scheduler + command encoder
  invariants we must preserve when layering in pooling.
- ✅ [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md):
  constrains how pools integrate with the runtime scheduler + tooling hooks.
- ✅ `docs/reviews/2025-03-24-AS-320-MATERIAL-PERSISTENCE.MD`: reiterates the need to recycle GPU handles deterministically for
  telemetry, reinforcing the pool design requirements.

---

## Design / Plan

### Constraints

- Pool must remain thread-safe only if scheduler issues buffers from multiple threads; otherwise document single-thread assumption and guard with debug asserts.
- Keep deterministic handle assignment to satisfy telemetry and debugging instrumentation.
- Ensure trimming respects backend fences so native resources aren’t destroyed before GPU idle.

### API / Data Sketch

```cpp
class CommandBufferPool {
 public:
  struct PooledBuffer {
    CommandBufferHandle handle;
    std::uint64_t last_frame_used{0};
    resources::CommandBufferNativeHandle native;
  };

  CommandBufferHandle acquire(QueueType queue, std::string_view label);
  void release(CommandBufferHandle handle, std::uint64_t current_frame);
  void trim_unused(std::uint64_t current_frame, std::uint64_t max_age = 30);
 private:
  std::unordered_map<QueueType, std::vector<PooledBuffer>> pools_;
  std::uint64_t next_handle_{0};
};
```

### Edge Cases & Failure Modes

- **Long-running GPU work:** Do not recycle buffers until backend signals completion; integrate with fence tracking.
- **Label changes per acquire:** Expose label metadata for debugging even when buffer reused.
- **Pool exhaustion spikes:** Provide metrics/warnings when pool misses exceed threshold, prompting capacity tuning.

### Test Plan

- **Unit Tests:**
  - Validate acquire/release ordering, trimming behavior, and handle reuse counts.
  - Simulate max_age trimming and ensure stale buffers removed.
- **Integration Tests:**
  - Exercise scheduler under stress (command-encoder tests) to confirm pool reduces allocation count.
- **Performance:**
  - Compare CPU time spent creating command buffers before/after; target ≥20% reduction.
- **Regression Tests:**
  - Telemetry guard verifying pool hit rate remains high (>80%) for PM-510 scenes.

### Tool Integration

**Profiling:**
- [ ] Insert PROFILE_SCOPE around pool acquire/release for targeted traces.

**Diagnostic UI:**
- [ ] Optionally expose pool stats through TL-312 metrics panel.

**Benchmark Automation:**
- [ ] Extend benchmark harness to log command buffer creation counts via telemetry JSON.

---

## Steps

1. [ ] Define pool implementation in `engine/rendering/src/command/command_buffer_pool.*`.
2. [ ] Integrate pool with `NativeSchedulerBase` and backend schedulers.
3. [ ] Add stress/unit tests in `engine/rendering/tests/command_scheduler_tests.cpp`.
4. [ ] Capture telemetry from PM-510 scenario comparing allocation counts.
5. [ ] Update rendering module README with pooling behavior and tuning knobs.
6. [ ] Run validation commands and record evidence before submitting PR.

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

**Benchmark:** Command buffer allocation reduction
- Before: _pending_
- After: _pending_
- Delta: _target ≥20% less CPU allocation time_

**Artifacts:**
- Telemetry captures: _pending_
- Benchmark logs: _pending_

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Build + test logs |
| perf | [ ] | Performance | Telemetry comparison |
| docs | [ ] | Docs/DevRel | README updates |
| safety | [ ] | Safety | N/A |
| release | [ ] | Release Mgr | N/A |

### Updated Files

- _pending_
