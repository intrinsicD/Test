# Task ID
`CO-170`

## Title
Kernel Dispatcher Runtime Integration Sample

## Type
- [x] Feature
- [ ] Bug Fix
- [ ] Refactor
- [ ] Documentation
- [x] Research
- [x] Performance Optimization

## Priority
- [ ] Critical (P0)
- [x] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
2 engineering weeks (including telemetry + documentation polish)

---

## Description

### Problem Statement
The compute module exposes `compute::KernelDispatcher`, but there is no end-to-end
sample that shows how the runtime submits asynchronous compute queues,
collects telemetry, and coordinates completion callbacks. Without a reference
integration we cannot validate queue orchestration against real workloads,
leaving roadmap items such as `AN-230`'s GPU sampling benchmarks blocked.

### Proposed Solution
Implement a standalone runtime integration sample that:

1. Instantiates `RuntimeHost` with a minimal scene graph and assets required to
   drive compute workloads.
2. Submits a batch of representative kernels (animation sampling, physics prep)
   through `compute::KernelDispatcher` using at least two async queues and fence
   callbacks.
3. Captures telemetry (queue occupancy, dispatch latency, completion jitter) via
   the shared diagnostics pipeline (`CC-001`).
4. Ships with analysis scripts and documentation so other modules can replay the
   sample and extend it for their workloads.

### Success Criteria
- A new executable (`engine_compute_runtime_sample`) builds on all presets and
  demonstrates dispatch across multiple compute queues with deterministic
  completion ordering.
- Telemetry emitted by the sample is consumable by `scripts/diagnostics` tooling
  and surfaces queue occupancy / latency metrics.
- Documentation in the compute and runtime module READMEs describes how to run
  the sample and interpret results, and the central roadmap reflects the task's
  completion.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::compute`
- `engine::runtime`
- `engine::animation` (sample workload stubs)
- `scripts::diagnostics`
- `docs`

**Files to Modify:**
- `engine/compute/CMakeLists.txt`
- `engine/runtime/CMakeLists.txt`
- `engine/runtime/include/engine/runtime/runtime_host.hpp`
- `engine/runtime/src/runtime_host.cpp`
- `engine/tests/integration/test_runtime_integration.cpp`
- `docs/modules/compute/README.md`
- `docs/modules/runtime/README.md`
- `docs/ROADMAP.md`

**New Files:**
- `engine/compute/samples/runtime_dispatch_demo/CMakeLists.txt`
- `engine/compute/samples/runtime_dispatch_demo/runtime_dispatch_demo.cpp`
- `engine/compute/samples/runtime_dispatch_demo/README.md`
- `scripts/diagnostics/compute_dispatch_report.py`
- `scripts/tests/test_compute_dispatch_report.py`
- `docs/design/CO-170-runtime-integration-playbook.md`

### Dependencies
**Depends On:**
- Task: `RU-307` (runtime submission parity) — ✅ completed, required API surface exists.
- Spec: `docs/design/ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md` (establishes
  telemetry expectations shared with `AN-230`).

**Blocks:**
- Task: `AN-230` (GPU/parallel sampling benchmarks)

### Related Work
- Issue: N/A (internal roadmap item)
- PR: N/A
- Epic: `CO-170` (Compute module roadmap), supports `AI-002` and `AN-230`

---

## Acceptance Criteria

### Functional Requirements
- [ ] Sample executable initialises `RuntimeHost`, registers compute kernels,
      and executes at least 1,024 frame ticks with deterministic results across
      runs.
- [ ] Dispatcher submissions cover multi-queue scheduling with timeline
      semaphore or fence callbacks recorded in telemetry.
- [x] CLI arguments allow configuring queue counts, workload mix, and telemetry
      output path for automation.

### Non-Functional Requirements
- [x] Performance: `--baseline` records a single-queue run, reports the
      observed speed-up, and warns when it drops below the ≥1.5× target for the
      60-joint animation workload captured in telemetry.
- [x] Memory: GPU staging buffers for the sample remain ≤256 MiB per frame,
      matching assumptions in `AN-230`.
- [x] Latency: Dispatch-to-completion jitter stays ≤0.5 ms at 60 FPS targets.

### Testing Requirements
- [x] Unit tests cover workload adapters feeding the dispatcher.
- [x] Integration tests validate deterministic telemetry output, dispatcher
      dependency graphs, and queue ordering (extend
      `engine_integration_tests`).
- [ ] Benchmarks captured by the sample show ≤2% variance between runs on
      reference hardware.
- [ ] Coverage ≥85% on touched lines in compute/runtime modules.

### Documentation Requirements
- [ ] Compute and runtime module READMEs updated with sample instructions.
- [ ] Add a design playbook (`CO-170-runtime-integration-playbook.md`) outlining
      the sample architecture and extension hooks.
- [ ] Central roadmap (`docs/ROADMAP.md`) reflects updated status of `CO-170`.
- [ ] Telemetry workflow documented under `scripts/diagnostics/` README.

---

## Test Plan

### Unit Tests
```cpp
TEST(ComputeRuntimeSample, DispatcherWorkloadAdaptersProduceDeterministicPoses) {
    // Arrange sample workload inputs (rigs, physics buffers)
    // Act: feed adapters into KernelDispatcher
    // Assert deterministic output buffers and telemetry counters
}
```

### Integration Tests
Extend `engine/tests/integration/test_runtime_integration.cpp` with a scenario
that executes the sample harness for a fixed number of frames and asserts queue
ordering, telemetry presence, and repeatability under seeded inputs.

### Performance Tests
Run the new sample executable with `--scenario animation` and `--scenario
hybrid` on reference GPUs (RTX 4080, Radeon 7900 XT). Record throughput, queue
occupancy, and jitter; compare against CPU baselines logged by the harness.

---

## Implementation Notes

### Design Considerations
- Reuse existing dispatcher configuration APIs; avoid special-casing the sample
  path so production code can adopt it with minimal changes.
- Capture telemetry using the shared schema from `design/TELEMETRY_SCHEMA.md`
  to keep dashboards consistent with other modules.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Asynchronous queues starve graphics submissions during sample runs | Medium | High | Allow queue depth configuration, document safe presets, and monitor telemetry for contention. |
| Hardware/driver variance skews telemetry | Medium | Medium | Gate reference metrics on pinned driver versions and ship baseline JSON for comparison. |
| Sample complexity slows onboarding | Low | Medium | Provide README walkthrough and scripted run commands in `scripts/diagnostics`. |

### Alternative Approaches
1. **Mock dispatcher harness**: Simulate queues without the runtime. → Rejected
   because it would not validate real integration or unblock `AN-230`.
2. **Integrate directly into runtime integration tests without standalone
   sample**: Avoids new executable but limits discoverability and interactive
   experimentation.

**Update Log:**
- 2025-11-07: Added `workload_configuration.hpp` and
  `test_runtime_dispatch_workload_configuration.cpp` to validate that workload
  adapters provision mesh subdivisions and physics body counts consistently
  across sample profiles.

---

## Deliverables

- [ ] Code implementation
- [x] Unit tests
- [ ] Integration tests
- [ ] Benchmarks
- [ ] API documentation
- [ ] Example code
- [ ] PR opened and linked
- [ ] All CI checks passing

---

## Definition of Done

- [ ] Builds cleanly on CI (Clang-22, MSVC)
- [ ] All tests pass (unit, integration, sanitizers)
- [ ] Performance regression ≤ 2%
- [ ] Code coverage ≥ 85% on touched lines
- [ ] Documentation updated and reviewed
- [ ] Code review approved by Tech Lead
- [ ] PR merged to main

---

## Assigned To
**Role**: Compute Engineer / Runtime Engineer pairing
**Name**: TBD (`@compute-lead`, `@runtime-lead`)

## Estimated Timeline
**Start Date**: 2025-10-29
**Target Completion**: 2025-11-12
**Actual Completion**: TBD

---

## Notes
- Align telemetry output with `python/scripts/telemetry_viewer.py` ingestion
  patterns so dashboards pick up new metrics automatically.
- Coordinate with animation team to ensure workloads mirror the planned
  scenarios in `AN-230`.
- Capture any new dispatcher APIs in `docs/design/CO-170-runtime-integration-playbook.md`
  for future reference.
- 2025-10-30: Runtime sample harness and Python report landed (JSON telemetry +
  jitter analysis). Next steps include multi-queue scenarios and workload
  expansion for animation GPU sampling.
- 2025-10-31: CLI exposes workload profiles and queue counts; telemetry and the
  diagnostics report now include per-queue aggregates to validate dispatcher
  utilisation ahead of GPU sampling benchmarks.
- 2025-11-01: CLI now supports queue naming and per-category overrides; JSON
  exports queue assignments for diagnostics and CI tooling.
- 2025-11-02: Integration tests assert steady-clock telemetry, deterministic
  dispatch ordering, and the dispatcher dependency graph to lock queue
  affinity regressions.
- 2025-11-03: Queue attribution now uses deterministic FNV-1a hashing so
  category-to-queue assignments remain stable across platforms even without
  explicit overrides.
- 2025-11-04: Added `--baseline` mode that captures a single-queue reference,
  includes speed-up metrics in telemetry, and surfaces warnings when the
  observed acceleration misses the 1.5× goal.
- 2025-11-05: Runtime sample now computes GPU staging estimates, embeds them in
  telemetry, and flags runs that exceed the 256 MiB animation budget alongside
  the diagnostics report.
- 2025-11-06: Added `--jitter-budget-ms` CLI option and wired runtime +
  diagnostics warnings when frame dispatch jitter exceeds the 0.5 ms budget,
  satisfying the latency acceptance criterion.
