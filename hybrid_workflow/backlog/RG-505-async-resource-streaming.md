---
id: RG-505
title: Async resource streaming for GPU providers
status: new
priority: P1
area: rendering
size: L
owner: rendering-runtime
gates: [tests, perf, docs]
relates_to: [bundle:rendering]
blocked_on: []
links:
  - docs/modules/assets/README.md
  - docs/modules/rendering/README.md
  - design/ASYNC_STREAMING.md
---

# Task RG-505 — Async Resource Streaming for GPU Providers

## Intent

Introduce asynchronous resource streaming so mesh/material uploads occur off the render thread, removing frame stalls and enabling smooth streaming demonstrations for PM-510 demos.

---

## Context

**Current State:**
- Resource requests inside `OpenGLRenderResourceProvider` execute synchronously, blocking the render thread until CPU uploads finish.
- Mesh-heavy workloads (TL-314 overlays, PM-510 case studies) hitch when datasets stream in, undermining telemetry.
- Assets module already exposes async-friendly handles, but rendering backend ignores them.

**Desired State:**
- `AsyncResourceLoader` handles futures for mesh/texture handles and processes completions on a worker thread pool.
- OpenGL resource provider defers GPU uploads until `flush_pending_uploads()` executes just before rendering, ensuring GL context safety.
- Runtime telemetry tracks pending uploads, queue depth, and stall counts.

**References:**
- [`design/ASYNC_STREAMING.md`](../../design/ASYNC_STREAMING.md)
- [`docs/modules/assets/README.md`](../../docs/modules/assets/README.md)
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md)

---

## Design / Plan

### Constraints

- Preserve deterministic ordering of uploads to avoid flicker; completed loads must be applied in submission order.
- Keep worker count configurable via runtime config and documented in module README.
- Ensure futures do not capture GL context pointers; only CPU-side decoding allowed off-thread.
- Integrate telemetry counters with TL-314 overlays per AGENTS documentation requirements.

### API / Data Sketch

```cpp
class AsyncResourceLoader {
 public:
  template<typename Handle, typename Resource>
  std::future<Resource> load_async(const Handle& handle);
  void process_completed_loads(RenderResourceProvider& provider);
 private:
  std::vector<std::future<void>> pending_loads_;
  std::mutex mutex_;
};

class OpenGLRenderResourceProvider {
  AsyncResourceLoader async_loader_;
  std::unordered_set<std::string> pending_meshes_;
  void require_mesh_async(const assets::MeshHandle& handle);
  void flush_pending_uploads();
};
```

### Edge Cases & Failure Modes

- **Handle invalidation mid-load:** Validate handles on completion; drop upload with warning if assets subsystem evicted resource.
- **Context loss:** Ensure `flush_pending_uploads()` no-ops when GL context missing (headless runs) but still clears futures.
- **Back-pressure:** Provide max pending load threshold; emit telemetry when threshold exceeded.

### Test Plan

- **Unit Tests:**
  - Mock AsyncResourceLoader to confirm futures resolve and callbacks enqueue uploads.
  - Validate back-pressure logic and telemetry counters.
- **Integration Tests:**
  - Run streaming scenario in prototype harness; ensure render thread frame time stays within baseline ±2% while assets stream.
- **Performance:**
  - Measure frame time variance before/after; expect elimination of >5 ms stalls during asset upload.
- **Regression Tests:**
  - Extend streaming report harness to assert async queue depth metrics remain under configured threshold.

### Tool Integration

**Profiling:**
- [ ] Add PROFILE_SCOPE markers around async processing and flush.

**Diagnostic UI:**
- [ ] Surface pending upload counts in TL-314 telemetry panel.

**Benchmark Automation:**
- [ ] Update `scripts/diagnostics/streaming_report.py` to capture async metrics.

**Configuration Management:**
- [ ] Document new runtime config knobs controlling worker count and thresholds.

---

## Steps

1. [ ] Audit current resource provider API surface and document async integration points here.
2. [ ] Implement `AsyncResourceLoader` utilities in `engine/assets` or shared module per design doc.
3. [ ] Integrate async loader with OpenGL provider and add telemetry counters.
4. [ ] Update runtime execution loop to call `flush_pending_uploads()` before command submission.
5. [ ] Expand tests under `engine/rendering/tests` and `scripts/tests` for streaming diagnostics.
6. [ ] Capture before/after frame time telemetry during PM-510 scenario for evidence.
7. [ ] Update documentation (module READMEs, NAVIGATION references) describing async streaming.
8. [ ] Run validation stack and attach evidence; open PR referencing this task.

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

**Benchmark:** Streaming hitch elimination
- Before: _pending_
- After: _pending_
- Delta: _target = eliminate stalls >5 ms_

**Artifacts:**
- Telemetry captures: _pending_
- Benchmark logs: _pending_

**Profiler Report:**
```
# Capture PROFILE_SCOPE output after implementation
```

**Benchmark Automation:**
```
# Document streaming_report.py invocation
```

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Build + test logs |
| perf | [ ] | Performance | Streaming telemetry |
| docs | [ ] | Docs/DevRel | README/NAV updates |
| safety | [ ] | Safety | Worker thread audit |
| release | [ ] | Release Mgr | N/A |

### Updated Files

- _pending_
