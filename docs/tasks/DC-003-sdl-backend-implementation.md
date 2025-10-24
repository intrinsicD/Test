# Task ID
`DC-003`

## Title
SDL Backend Parity Implementation

## Type
- [x] Feature
- [ ] Bug Fix
- [ ] Refactor
- [ ] Documentation
- [ ] Research
- [ ] Performance Optimization

## Priority
- [ ] Critical (P0)
- [x] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
3–4 weeks (multi-platform implementation, validation, and CI enablement)

---

## Description

### Problem Statement
The platform module currently exposes GLFW and mock window backends while the SDL backend remains a stub. Roadmap initiative `DC-003` requires SDL parity so the engine can run on systems where GLFW is unavailable, unblock editor integration work, and allow CI to exercise real input/event pumping. Without this implementation, runtime consumers cannot rely on SDL for cross-platform deployments, and downstream tasks (rendering visibility culling demos, tooling UX) lack test coverage on the SDL path.

### Proposed Solution
Implement the SDL backend following the parity checklist in `docs/modules/platform/SDL_BACKEND_CHECKLIST.md`. Deliver native window lifecycle management, deterministic event translation, swapchain surface export for Vulkan/OpenGL, and telemetry instrumentation. Update build presets and CI scripts to install SDL, add unit/integration coverage, and extend documentation so contributors can configure and troubleshoot the backend.

### Success Criteria
- SDL backend selected at build or runtime produces native windows with deterministic event order on Linux, Windows, and macOS.
- Runtime integration tests pass with `ENGINE_PLATFORM_WINDOW_BACKEND=sdl`, and fallback logic demotes to the mock backend when SDL is missing.
- Diagnostics tooling reports SDL initialization/capability failures with actionable telemetry and documentation links.
- CI executes at least one preset with SDL enabled to guard regressions.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::platform`
- `engine::runtime`
- `engine::rendering` (swapchain surface plumbing)
- `scripts` (CI/tooling)

**Files to Modify:**
- `engine/platform/include/engine/platform/window.hpp`
- `engine/platform/src/windowing/sdl_window.cpp`
- `engine/platform/tests/` (add SDL-specific coverage)
- `engine/runtime/tests/test_module.cpp`
- `scripts/build/presets/*.json` and CI orchestration under `scripts/ci/`
- `docs/modules/platform/README.md`
- `docs/modules/platform/SDL_BACKEND_CHECKLIST.md`

**New Files:**
- `engine/platform/tests/test_sdl_window.cpp`
- SDL fixture assets or configuration helpers as needed

### Dependencies
**Depends On:**
- Task: `PL-215` (SDL parity checklist)
- Task: `CC-002` (hot reload/telemetry plumbing)
- Library: `SDL2`/`SDL3` development packages per target OS

**Blocks:**
- Task: `T-0122` (Rendering visibility culling system requires deterministic window/input events for benchmarks)
- Task: `TL-120` follow-ups that rely on cross-platform runtime samples

### Related Work
- Epic: `DC-003` (central roadmap)
- Doc: `docs/modules/platform/SDL_BACKEND_CHECKLIST.md`
- Issue/Task references: `docs/ROADMAP.md`, `docs/modules/platform/README.md`

---

## Acceptance Criteria

### Functional Requirements
- [ ] SDL backend creates/destroys native windows respecting `WindowConfig` (size, fullscreen, DPI) on Linux, Windows, and macOS.
- [ ] Event pump translates SDL window/input/controller events into the engine event queue with deterministic ordering guarantees.
- [ ] Swapchain surfaces for Vulkan and OpenGL are exported via `SwapchainSurface` so rendering backends operate without GLFW.
- [ ] Runtime fallback demotes to the mock backend when SDL prerequisites fail, returning structured error codes.

### Non-Functional Requirements
- [ ] Performance: SDL event pumping adds ≤ 5% overhead compared to GLFW baseline in diagnostics runs.
- [ ] Memory: SDL backend allocations remain within existing platform module budgets (≤ 1 MiB persistent heap usage).
- [ ] Latency: Initial window creation completes in ≤ 50 ms on reference hardware.

### Testing Requirements
- [ ] Platform unit tests cover SDL window lifecycle, event translation, and fallback logic.
- [ ] Runtime integration tests execute with `ENGINE_PLATFORM_WINDOW_BACKEND=sdl` and pass deterministically.
- [ ] CI presets compile and run SDL tests on at least one Linux and one Windows builder (or equivalent local gating).
- [ ] Coverage ≥ 85% on new SDL backend code paths.

### Documentation Requirements
- [ ] Platform README documents SDL configuration, troubleshooting, and telemetry signals.
- [ ] SDL parity checklist updated with implementation status and residual gaps (if any).
- [ ] Roadmap (`docs/ROADMAP.md`) and root README references kept in sync with task completion.

---

## Test Plan

### Unit Tests
```cpp
TEST(SdlWindowBackend, CreatesNativeWindow) {
    auto backend = engine::platform::make_sdl_window(WindowConfig{/* ... */});
    ASSERT_TRUE(backend);
    EXPECT_TRUE(backend->is_valid());
}

TEST(SdlWindowBackend, TranslatesKeyboardEventsDeterministically) {
    // Inject synthetic SDL events and verify EventQueue ordering
}
```

### Integration Tests
- Extend `engine/tests/integration/test_runtime_integration.cpp` (or similar harness) to run with SDL backend, validating window creation, resize events, and shutdown while rendering mock frames.
- Add smoke test invoking `RuntimeHost` with SDL enabled via environment override, asserting telemetry counters increment correctly.

### Performance Tests
- Record SDL vs GLFW event pump timings within diagnostics harness; fail test if SDL overhead exceeds 5% tolerance.
- Capture swapchain creation latency metrics for regression tracking.

---

## Implementation Notes

### Design Considerations
- Mirror GLFW backend abstractions so subsystems remain backend-agnostic.
- Guard SDL initialization failures with structured error codes and telemetry.
- Preserve deterministic ordering by buffering SDL events before publishing to the engine queue.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| SDL API differences across 2.x and 3.x | Medium | Medium | Abstract version-specific calls behind helper layer; document supported versions. |
| CI instability due to missing display servers | High | Medium | Use headless SDL hints (`SDL_HINT_VIDEO_DRIVER`) and extend CI scripts to install dummy drivers. |
| Swapchain integration regressions | Medium | High | Add regression tests comparing GLFW vs SDL swapchain handles; instrument telemetry for failures. |

### Alternative Approaches
1. **Continue using GLFW-only support**: rejected—fails roadmap goal and blocks cross-platform/editor work.
2. **Wrap SDL via third-party abstraction layer**: rejected—introduces additional dependency and diverges from existing engine abstractions.

---

## Deliverables

- [ ] SDL backend implementation in `engine/platform`
- [ ] Platform unit tests for SDL path
- [ ] Runtime integration tests covering SDL selection/fallback
- [ ] CI preset updates + documentation for SDL dependencies
- [ ] Telemetry logging and diagnostics updates
- [ ] Documentation updates (Platform README, checklist, roadmap)
- [ ] PR opened and linked to `DC-003`
- [ ] All CI checks passing

---

## Definition of Done

- [ ] Builds cleanly on CI (Clang-22, MSVC)
- [ ] All tests pass (unit, integration, sanitizers)
- [ ] Performance regression ≤ 2%
- [ ] Code coverage ≥ 85% on touched lines
- [ ] Documentation updated and reviewed
- [ ] Code review approved by Platform Tech Lead
- [ ] PR merged to main

---

## Assigned To
**Role**: Platform Engineer
**Name**: _TBD_

## Estimated Timeline
**Start Date**: 2025-10-25
**Target Completion**: 2025-11-15
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with the Tools/CI team to provision SDL packages on runners (follow-up ticket if automation is missing).
- Capture telemetry schema updates in `design/TELEMETRY_SCHEMA.md` if new counters are introduced.
- Sync with rendering team to validate swapchain handles against Vulkan scheduler tests.
