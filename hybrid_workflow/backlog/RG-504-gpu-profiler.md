---
id: RG-504
title: GPU timeline queries & profiling integration
status: new
priority: P2
area: rendering
size: M
owner: rendering-telemetry
gates: [tests, perf, docs]
relates_to: [bundle:rendering]
blocked_on: []
links:
  - docs/modules/rendering/README.md
  - docs/modules/tools/README.md
  - design/TELEMETRY_INSTRUMENTATION_GUIDE.md
---

# Task RG-504 — GPU Timeline Queries & Profiling Integration

## Intent

Add GPU timestamp query infrastructure and pass-level telemetry reporting so researchers can correlate CPU frame graph passes with GPU execution time directly inside TL-312/TL-314 tooling.

---

## Context

**Current State:**
- Telemetry stack reports CPU timings but lacks GPU timestamps despite hardware support in both OpenGL and Vulkan backends.
- PM-510 demos require GPU timing evidence; currently we rely on external profilers.
- RenderExecutionContext has no hook for optional GPU profiling.

**Desired State:**
- `GpuProfiler` manages timestamp queries, associates them with pass names, and exposes completed timings each frame.
- Frame graph execution optionally wraps each pass with `begin_pass()` / `end_pass()` instrumentation feeding TL-312 metrics panel.
- Telemetry exports include GPU duration per pass for offline benchmarking.

**References:**
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md)
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md)
- [`design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md)

---

## Design / Plan

### Constraints

- Profiling must be optional to avoid query overhead on constrained builds; guard behind runtime flag and compile-time capability macros.
- Ensure query pool recycling handles wraparound; limit outstanding queries per frame.
- Provide backend-specific implementations but keep interface shared.

### API / Data Sketch

```cpp
class GpuProfiler {
 public:
  struct PassTimings {
    std::string pass_name;
    double gpu_time_ms{0.0};
    std::uint64_t timestamp_begin{0};
    std::uint64_t timestamp_end{0};
  };

  void begin_pass(std::string_view name);
  void end_pass();
  [[nodiscard]] std::vector<PassTimings> collect_timings();

 private:
  struct TimestampQuery {
    std::uint32_t query_id;
    std::string pass_name;
    bool completed{false};
  };
  std::vector<TimestampQuery> pending_queries_;
  std::uint32_t next_query_id_{0};
};
```

### Edge Cases & Failure Modes

- **Query allocation failure:** Fallback gracefully by disabling GPU profiling and logging warning.
- **Disjoint timestamp availability:** Some OpenGL drivers require `GL_TIMESTAMP` support; detect and skip if unavailable.
- **Long frames:** Ensure query results are polled for multiple frames to avoid stalling.

### Test Plan

- **Unit Tests:**
  - Mock profiler to ensure begin/end pairs recorded and reported correctly.
  - Validate overflow handling when query pool wraps.
- **Integration Tests:**
  - Run prototype harness with GPU profiling enabled and verify telemetry JSON includes pass durations.
- **Performance:**
  - Measure added overhead (<2% frame time increase) with profiling enabled.
- **Regression Tests:**
  - Extend TL-312 panel tests to display GPU timings without crashing when profiling disabled.

### Tool Integration

**Profiling:**
- [ ] Use GPU timing output to annotate TL-312/TL-314 panels and PM-510 demos.

**Diagnostic UI:**
- [ ] Add GPU timing track to tools module panel.

**Benchmark Automation:**
- [ ] Export GPU timing metrics through telemetry scripts for CI comparison.

**Configuration Management:**
- [ ] Document new runtime flag enabling GPU profiling in README + NAV.

---

## Steps

1. [ ] Implement backend-agnostic `GpuProfiler` interface and backend-specific query helpers.
2. [ ] Integrate profiler hooks into frame graph execution context and rendering passes.
3. [ ] Extend TL-312/TL-314 panels plus telemetry exporters to include GPU timings.
4. [ ] Add tests covering profiler toggles and data export.
5. [ ] Capture GPU timing evidence for PM-510 scenario to validate feature.
6. [ ] Update documentation and backlog/roadmap once feature merged.

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

**Benchmark:** GPU profiling overhead
- Before: _pending_
- After: _pending_
- Delta: _target <2% overhead_

**Artifacts:**
- Telemetry captures: _pending_
- Benchmark logs: _pending_

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Build + test logs |
| perf | [ ] | Performance | GPU timing comparisons |
| docs | [ ] | Docs/DevRel | README/Telemetry guide updates |
| safety | [ ] | Safety | N/A |
| release | [ ] | Release Mgr | N/A |

### Updated Files

- _pending_
