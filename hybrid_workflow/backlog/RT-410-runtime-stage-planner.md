---
id: RT-410
title: Runtime stage planner & presentation loop
status: in_progress
priority: P1
area: runtime
size: L
owner: runtime-lead
gates: [tests, perf, docs]
relates_to: [bundle:B]
blocked_on: []
links: ["docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md", "docs/design/RT_410_STAGE_PLANNER_API.md", "docs/modules/runtime/README.md", "hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md", "hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md"]
---

# Task RT-410 — Runtime Stage Planner & Presentation Loop

## Intent

Deliver the ADR-0008 stage planner, presentation adapters, and synchronization surfaces so the runtime drives GPU presentation deterministically across backends and tooling.

---

## Context

**Current State:**
- `RuntimeHost` executes declarative loop plans but lacks backend-aware presentation hooks.
- Tooling requires ad-hoc wiring to preview GPU work because synchronization APIs remain stubbed.
- PM-510 demos track runtime progress yet depend on mock presentation paths.

**Desired State:**
- Stage planner produces schedulable stages with explicit synchronization handles.
- Presentation backends for mock, OpenGL, and Vulkan share runtime configuration and telemetry.
- Tooling and scripting reuse the same presentation adapters exposed through the runtime API.

**References:**
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [`docs/modules/runtime/README.md`](../docs/modules/runtime/README.md)
- [`hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md`](archive/T-0119-command-encoder-integration.md)
- [`hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md`](archive/T-0120-gpu-resource-provider.md)
- Presentation notes in PM-510 weekly demos

---

## Design / Plan

### Constraints

- Keep runtime loop deterministic across headless and presentation modes.
- Coordinate scheduler integration with rendering milestones (T-0119/T-0120).
- Propagate telemetry for frame phases and presentation latency to diagnostics tooling.
- Maintain thread-safety of synchronization primitives reviewed by safety gate.
- Use feature flags to stage rollout with PM-510 oversight.

### API / Data Sketch

```cpp
namespace engine::runtime {

struct StageHandle {
  std::string_view name;
  RuntimeStageKind kind;
  Duration budget;
};

class RuntimeStagePlanner {
public:
  Result<void, ErrorCode> ConfigurePlan(const RuntimeLoopPlan& plan);
  Result<StageExecution, ErrorCode> NextStage(const RuntimeContext& ctx);
};

class PresentationBackend {
public:
  virtual Result<void, ErrorCode> Initialize(const PresentationConfig& config) = 0;
  virtual Result<void, ErrorCode> Present(const PresentationFrame& frame) = 0;
  virtual void Shutdown() = 0;
};

} // namespace engine::runtime
```

### Edge Cases & Failure Modes

- **Backend capability mismatch:** Detect unsupported presentation modes and degrade gracefully to mock backend.
- **Synchronization deadlocks:** Validate timeline semaphores and CPU/GPU fences under sanitizer builds.
- **Headless mode regressions:** Ensure stage planner honours headless-safe requirements without requiring presentation surfaces.
- **Telemetry overload:** Bound emitted metrics to avoid overwhelming PM-510 dashboards.

### Test Plan

- **Unit Tests:**
  - Stage planner scheduling order and dependency handling.
  - Presentation backend initialization and teardown for mock + GLFW.
  - Synchronization primitives under sanitizer configurations.
- **Integration Tests:**
  - Runtime harness executes sample loop plan with presentation enabled.
  - Tooling preview uses shared adapters without manual wiring.
  - PM-510 smoke scenario exercises GPU pipeline once T-0119/T-0120 land.
- **Performance:**
  - Capture presentation latency (p95) before/after integration (target ≤2% increase).
  - Measure runtime loop overhead across mock and real backends.

---

## Steps

1. [x] Reconcile ADR-0008 plan with latest runtime loop implementation and log deltas here.
   - `RuntimeLoopStage`/`RuntimeLoopBuilder` already expose declarative stage metadata with dependencies and thread affinity, matching ADR-0008 foundations, but there is no `RuntimeStagePlanner` abstraction or stage handles/budget metadata yet (`engine/runtime/include/engine/runtime/loop.hpp`, `engine/runtime/src/loop.cpp`).
   - `RuntimeHost::tick` consumes the compiled plan sequentially and records telemetry/diagnostics directly; it does not provide resumable `StageExecution` objects or scheduling APIs expected from the planner (`engine/runtime/src/api.cpp`).
   - Presentation backends are partially integrated (mock + OpenGL implementations exist), yet they rely on callback invocation without the synchronized stage planner hooks that ADR-0008 reserves for deterministic presentation sequencing (`engine/runtime/src/api.cpp`, `engine/rendering/include/engine/rendering/presentation_backend.hpp`).
2. [x] Implement stage planner core plus serialization hooks in `engine/runtime/src/`.
   - [x] (2025-05-07) Introduced `RuntimeStagePlanner`, integrated planner iteration into the runtime host, and documented planner error handling.
3. [x] Design presentation backend API contracts (RT-410-A)
   - [x] (2025-11-06) Created comprehensive API design document: `docs/design/RT_410_STAGE_PLANNER_API.md`
   - [x] Specified `PresentationConfig`, `PresentationFrame`, enhanced `PresentationBackend` interface
   - [x] Defined `PresentationStageBuilder` for runtime loop integration
   - [x] Documented telemetry integration and tooling patterns
   - [ ] Pending review with module leads before implementation
4. [x] Deliver mock + GLFW presentation backends with shared configuration surfaces.
    - Added `runtime::create_presentation_surface()` plus config/surface bundles so mock and GLFW integrations share window + swapchain plumbing, recording errors through new runtime presentation surface error codes and covering the helper with unit tests.
5. [ ] Expose synchronization APIs to scripting/tooling and update documentation.
6. [ ] Extend harness/integration tests plus PM-510 demo scenario.
7. [ ] Capture telemetry + benchmark evidence and update quality gate table.
8. [ ] Coordinate review, update ROADMAP, and advance status to `review`/`done`.

---

## Evidence

### Test Results

```bash
# Pending — fill after implementation milestones
```

**Test Summary:**
- Unit tests: pending implementation
- Integration tests: pending implementation
- Documentation validation: pending implementation

### Performance

**Benchmark:** PresentationLatency
- Before: 8.4 ms p95 (mock backend)
- After: _TBD_
- Delta: _TBD_

**Artifacts:**
- Telemetry captures: `telemetry/runtime_stage_planner_baseline.json` (planned)
- Demo recordings: PM-510 weekly integration demos

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] Pending | QA/Test | Harness + unit test outputs |
| perf | [ ] Pending | Performance | Presentation latency telemetry |
| docs | [ ] Pending | Docs/DevRel | Runtime README + prototyping playbook updates |
| safety | [ ] Pending | Safety | Synchronization audit, sanitizer logs |
| release | [ ] Pending | Release Mgr | Feature flag + rollout documentation |

### Updated Files

- `engine/runtime/include/engine/runtime/errors.hpp`
- `engine/runtime/include/engine/runtime/runtime_loop_plan.hpp`
- `engine/runtime/src/api.cpp`
- `engine/runtime/src/runtime_stage_planner.cpp`
- `engine/runtime/CMakeLists.txt`
- `engine/runtime/tests/runtime_stage_planner_tests.cpp`
- `engine/runtime/tests/CMakeLists.txt`
- `docs/modules/runtime/README.md`

---

## Completion Checklist (Definition of Done)

- [ ] Stage planner implemented with deterministic scheduling.
- [ ] Presentation backends operational for mock + GLFW (Vulkan path tracked with rendering).
- [ ] Synchronization APIs exposed to runtime, tooling, and scripting consumers.
- [ ] Integration tests and harness scenarios cover presentation flow.
- [ ] Telemetry + performance baselines captured and signed off.
- [ ] Documentation refreshed across runtime README, prototyping playbook, and roadmap.
- [ ] PM-510 demos capture runtime milestone progress.
- [ ] Task advanced to `done` and archived once gates close.

---

## Result

**PR:** (pending completion)

**SHA:** (pending merge)

**Completion Date:** (in progress)

**Notes:**
- Coordinate with platform team for windowing capability detection while wiring GLFW backend.
- Ensure tooling harness consumes new presentation APIs before TL-310 begins.
- Capture sanitizer coverage for synchronization primitives prior to safety gate sign-off.

**Follow-ups:**
- [ ] Author presentation backend troubleshooting guide → spawn docs task.
- [ ] Evaluate Vulkan presentation backend scheduling once T-0119/T-0120 land → spawn follow-up.

---

## Role Coordination

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Agent Orchestrator | Agent Orchestrator | Track runtime/presentation blockers and escalate | Active |
| Product Manager | Product Manager | Sequence runtime milestone with GPU deliverables | Active |
| Knowledge Librarian | Knowledge Librarian | Keep ADR-0008 updates and documentation synced | Active |
| Specialist Engineer(s) | Runtime Lead | Implement planner, presentation backends, synchronization APIs | In Progress |
| Docs/DevRel | Docs Team | Update runtime docs + prototyping playbook | Queued |
| QA/Test Specialist | QA Lead | Extend harness/tests for presentation coverage | In Progress |
| Performance Engineer | Performance Lead | Benchmark loop & presentation latency | In Progress |
| Safety Reviewer | Security Reviewer | Review synchronization + threading invariants | Queued |
| Reviewer | Runtime Reviewer | Provide code reviews for runtime patches | Queued |
| Release Manager | Release Manager | Manage feature flags, release notes | Queued |
