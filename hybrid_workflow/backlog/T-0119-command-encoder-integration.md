---
id: T-0119
title: Command encoder integration
status: in_progress
priority: P1
area: rendering
size: L
owner: rendering-lead
gates: [tests, perf, docs]
relates_to: [bundle:A]
blocked_on: []
links: ["docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md", "docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md", "hybrid_workflow/backlog/T-0120-gpu-resource-provider.md"]
---

# Task T-0119 — Command Encoder Integration

## Intent

Restore the GPU command encoder so frame-graph passes emit backend work for OpenGL and Vulkan, enabling real draw and compute submissions that align with runtime presentation and tooling.

---

## Context

**Current State:**
- Frame-graph execution stops at validation since encoder hooks remain stubbed.
- Backend schedulers cannot translate graph operations into GPU command buffers.
- Runtime presentation and tooling depend on encoder outputs to drive demos captured in PM-510.

**Desired State:**
- Encoder APIs translate frame-graph workloads into backend-specific command lists.
- OpenGL and Vulkan schedulers submit encoded work with telemetry, error reporting, and synchronization points.
- Runtime presentation loop consumes encoder submissions without bespoke glue code.

**References:**
- [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](../docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md)
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [`hybrid_workflow/backlog/T-0120-gpu-resource-provider.md`](T-0120-gpu-resource-provider.md)
- Rendering module README (`docs/modules/rendering/README.md`)

---

## Design / Plan

### Constraints

- Maintain frame-graph contracts from ADR-0003 when mapping passes to encoder commands.
- Keep error handling deterministic using `engine::Result<T, ErrorCode>`.
- Surface telemetry compatible with PM-510 integration demos.
- Coordinate API surfaces with GPU resource provider work (T-0120).
- Preserve cross-platform determinism in command ordering and synchronization semantics.

### API / Data Sketch

```cpp
namespace engine::rendering {

struct EncoderPassContext {
  FrameGraphNodeId node_id;
  std::span<const ResourceHandle> read_resources;
  std::span<const ResourceHandle> write_resources;
};

class ICommandEncoder {
public:
  virtual Result<void, ErrorCode> BeginPass(const EncoderPassContext& ctx) = 0;
  virtual Result<void, ErrorCode> EncodeDraw(const DrawCommand& command) = 0;
  virtual Result<void, ErrorCode> EncodeCompute(const ComputeCommand& command) = 0;
  virtual Result<void, ErrorCode> InsertBarrier(const BarrierDesc& barrier) = 0;
  virtual Result<SubmissionHandle, ErrorCode> Finalize() = 0;
};

} // namespace engine::rendering
```

### Edge Cases & Failure Modes

- **Resource mismatch:** Detect resources missing from provider bindings and return structured errors.
- **Backend capability gaps:** Fallback to mock encoders or mark passes unsupported with telemetry annotations.
- **Synchronization deadlocks:** Validate barrier ordering using scheduler integration tests and sanitize command dependencies.
- **Shader/PSO compilation failure:** Bubble errors with contextual diagnostics and link to resource provider telemetry.

### Test Plan

- **Unit Tests:**
  - Encode draw and compute operations covering resource transitions.
  - Validate barrier sequencing and error propagation for unsupported states.
  - Exercise command list finalization and submission handles across backends.
- **Integration Tests:**
  - Frame-graph workload renders simple geometry through OpenGL and Vulkan.
  - Runtime presentation loop consumes encoder submissions without regressions.
  - PM-510 smoke scenario captures telemetry for GPU execution paths.
- **Performance:**
  - Benchmark command recording overhead relative to recording provider baseline (≤2% regression target).
  - Capture queue submission latency using telemetry harness shared with T-0120.

---

## Steps

1. [ ] Finalize encoder/provider handshake with T-0120 owners and record decisions in this file.
2. [ ] Implement encoder core in `engine/rendering/src/command_encoder.cpp` with unit coverage.
3. [ ] Wire OpenGL and Vulkan scheduler backends to consume encoded commands.
   - Frame-graph execution now finalizes command encoders before GPU submission so backend providers observe a completed recording prior to scheduler hand-off.
4. [ ] Add frame-graph integration tests and PM-510 smoke scenario hooks.
   - [x] Added `FrameGraph.RecordsCommandsThroughEncoderProvider` to verify command encoder submissions (2025-03-27).
   - [ ] Extend PM-510 smoke scenarios once runtime submission wiring lands.
5. [ ] Document encoder usage in rendering/runtime READMEs and update roadmap entries.
6. [ ] Run performance benchmarks and capture telemetry artefacts.
7. [ ] Request review, update task status, and synchronize roadmap/README references.

---

## Evidence

### Test Results

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

**Test Summary:**
- Build + unit/integration suites: linux-gcc-debug preset ✅
- Python + scripts: pytest ✅
- Documentation validation: validate_docs ✅

### Performance

**Benchmark:** CommandRecordingOverhead (target)
- Before: 0.38 ms per submission (recording provider)
- After: _TBD_
- Delta: _TBD_

**Artifacts:**
- Telemetry captures: `telemetry/gpu_encoder_baseline_2025-02-28.json` (planned)
- Demo outputs: PM-510 weekly demo notes

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] Pending | QA/Test | Populate after encoder test suites land |
| perf | [ ] Pending | Performance | Benchmark summary once telemetry captured |
| docs | [ ] Pending | Docs/DevRel | Rendering/runtime README diffs |
| safety | [ ] Pending | Safety | Synchronization audit + sanitizer runs |
| release | [ ] Pending | Release Mgr | Feature flag & rollout notes |

### Updated Files

- `engine/rendering/include/engine/rendering/command_encoder.hpp`
- `engine/rendering/src/backend/opengl/opengl_command_encoder.cpp`
- `engine/rendering/src/backend/vulkan/vulkan_command_encoder.cpp`
- `engine/rendering/tests/command_encoder_tests.cpp`
- `docs/modules/rendering/README.md`
- `docs/modules/runtime/README.md`
- `docs/ROADMAP.md`
- `engine/rendering/tests/test_frame_graph.cpp`

---

## Completion Checklist (Definition of Done)

- [ ] Encoder APIs implemented with unit coverage.
- [ ] Backend schedulers submit encoded work for OpenGL and Vulkan.
- [ ] Frame-graph integration tests validate end-to-end rendering.
- [ ] Runtime/tooling docs updated with encoder usage patterns.
- [ ] Performance benchmarks captured and within tolerance.
- [ ] PM-510 demos include encoder progress artefacts.
- [ ] `hybrid_workflow/ROADMAP.md` Bundle A checkbox updated.
- [ ] `docs/ROADMAP.md` and root README synchronized.
- [ ] Status moved to `review` → `done` after approvals.

---

## Result

**PR:** (pending completion)

**SHA:** (pending merge)

**Completion Date:** (in progress)

**Notes:**
- Coordinate with runtime team to surface encoder telemetry in PM-510 demos.
- Validate synchronization semantics with safety reviewers before release gate closes.
- Spawn follow-up tasks for command recorder tooling once baseline ships.

**Follow-ups:**
- [ ] Investigate command batching heuristics → create task T-0121 follow-up.
- [ ] Extend encoder diagnostics to scripting APIs → create task RT-415.

---

## Role Coordination

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Agent Orchestrator | Agent Orchestrator | Align GPU milestone sequencing and unblock cross-team dependencies | Active |
| Product Manager | Product Manager | Sequence delivery with T-0120 and runtime presentation | Active |
| Knowledge Librarian | Knowledge Librarian | Document encoder architecture and ADR updates | Active |
| Specialist Engineer(s) | Rendering Lead | Implement encoder core and backend wiring | In Progress |
| Docs/DevRel | Docs Team | Refresh encoder sections in docs/modules/rendering & runtime | Queued |
| QA/Test Specialist | QA Lead | Extend backend smoke tests and frame-graph coverage | In Progress |
| Performance Engineer | Performance Lead | Benchmark command recording & submission latency | In Progress |
| Safety Reviewer | Security Reviewer | Review synchronization/threading guarantees | Queued |
| Reviewer | Rendering Reviewer | Perform code reviews for encoder patches | Queued |
| Release Manager | Release Manager | Manage feature flags, rollout coordination | Queued |
