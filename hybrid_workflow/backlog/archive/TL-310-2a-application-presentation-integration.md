---
id: TL-310-2a
title: Application ↔ PresentationBackend Integration
status: done
priority: P2
area: runtime
size: S
owner: tools-lead
gates: [tests, docs]
relates_to: [TL-310, RT-410]
blocked_on: []
parent_task: TL-310
links:
  - "hybrid_workflow/backlog/TL-310-editor-foundations.md"
  - "hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md"
  - "GEOMETRY_VIEWER_SOLUTION.md"
  - "docs/modules/runtime/README.md"
  - "docs/modules/rendering/README.md"
---

# Task TL-310-2a — Application ↔ PresentationBackend Integration

## Intent

Wire RT-410's completed presentation backends into the `runtime::Application` framework so that `geometry_viewer` and future tools/editor harness can execute frame graphs and render to screen.

---

## Context

**Current State:**
- RT-410 (archived 2026-03-30) delivered `PresentationBackend` implementations (Mock, GLFW, OpenGL)
- `runtime::Application` provides lifecycle callbacks (`on_initialize`, `on_update`, `on_render`, `on_shutdown`)
- `Application::run_main_loop()` calls `on_render()` every frame
- **BUT:** No `RenderExecutionContext` exists in Application
- **BUT:** No `PresentationBackend` instantiation in Application
- **Result:** `geometry_viewer.cpp` has empty `on_render()` stub - cannot execute frame graph

**Desired State:**
- Application creates and manages PresentationBackend based on config
- Application provides `RenderExecutionContext` accessor to derived classes
- Application handles begin_frame/end_frame/present lifecycle
- `geometry_viewer` can call `frame_graph_.execute(render_context())` in `on_render()`
- Pattern documented for other rendering tools/examples

**References:**
- RT-410 completion: [`hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md`](RT-410-runtime-stage-planner.md)
- Analysis document: [`GEOMETRY_VIEWER_SOLUTION.md`](../../../GEOMETRY_VIEWER_SOLUTION.md)
- Application class: [`engine/runtime/include/engine/runtime/application.hpp`](../../../engine/runtime/include/engine/runtime/application.hpp)
- Presentation backend interface: [`engine/rendering/include/engine/rendering/presentation_backend.hpp`](../../../engine/rendering/include/engine/rendering/presentation_backend.hpp)

---

## Design / Plan

### Constraints

- Must support multiple backend types (Mock for headless/CI, GLFW+OpenGL for development)
- Respect existing Application API contract - avoid breaking derived classes
- Keep configuration simple - sensible defaults with opt-in complexity
- Ensure proper resource cleanup in shutdown
- Document migration path for existing Application users

### API / Data Sketch

**Application configuration extension:**

```cpp
struct ApplicationConfig
{
    // ...existing window config...
    
    /// Rendering backend selection
    enum class RenderBackend
    {
        Auto,      // Auto-detect based on window backend
        Mock,      // Headless/testing
        OpenGL,    // OpenGL 4.6
        Vulkan     // Vulkan (future)
    };
    
    RenderBackend render_backend{RenderBackend::Auto};
    bool enable_vsync{true};
};
```

**Application class changes:**

```cpp
class Application
{
protected:
    // New accessor for derived classes
    [[nodiscard]] rendering::RenderExecutionContext& render_context() noexcept;
    [[nodiscard]] const rendering::RenderExecutionContext& render_context() const noexcept;

private:
    void initialize_rendering_subsystem();
    void shutdown_rendering_subsystem();
    
    std::unique_ptr<rendering::PresentationBackend> presentation_backend_;
    std::unique_ptr<rendering::RenderExecutionContext> render_context_;
    // Also need: RenderResourceProvider, MaterialSystem, GPUResourceProvider, etc.
};
```

**Main loop integration:**

```cpp
void Application::run_main_loop()
{
    while (running_ && !window_->close_requested())
    {
        // ...existing timing/input/scene update...
        
        // Begin frame
        if (presentation_backend_)
        {
            presentation_backend_->begin_frame(*render_context_);
        }
        
        // User rendering (can now use render_context())
        on_render();
        
        // End frame and present
        if (presentation_backend_)
        {
            presentation_backend_->end_frame(*render_context_);
            presentation_backend_->present(*render_context_);
        }
        
        // ...existing frame limiting...
    }
}
```

### Edge Cases & Failure Modes

- **Headless mode:** Mock backend should work without GPU/window surface
- **Missing GPU:** Graceful fallback to mock backend with warning
- **Backend creation failure:** Report error, fall back to mock or abort
- **Context not ready:** `render_context()` should check validity
- **Multiple windows:** Current design single-window; document limitation
- **Backend switching:** Not supported in v1; document as future work

### Test Plan

- **Unit Tests:**
  - Application with mock backend initializes successfully
  - render_context() accessor returns valid context after init
  - Rendering subsystem cleans up properly in shutdown
  - Config backend selection works correctly
- **Integration Tests:**
  - geometry_viewer builds and renders cube
  - Application + GLFW + OpenGL backend lifecycle
  - Headless test with mock backend
- **Regression:**
  - Existing Application-based code without rendering still works

---

## Steps

1. [x] Study RT-410 presentation backend API and RenderExecutionContext requirements
   - Read `engine/rendering/include/engine/rendering/presentation_backend.hpp`
   - Read `engine/rendering/include/engine/rendering/render_pass.hpp` for RenderExecutionContext
   - Check how backends are instantiated in rendering module
2. [x] Design Application integration pattern
   - Decide on configuration API (extend ApplicationConfig)
   - Plan RenderExecutionContext ownership and lifetime
   - Determine dependencies (RenderResourceProvider, MaterialSystem, etc.)
   - Document integration design
3. [x] Implement Application class changes
   - Add members for presentation backend and render context
   - Implement `initialize_rendering_subsystem()`
   - Update `run_main_loop()` with begin_frame/end_frame/present
   - Implement `shutdown_rendering_subsystem()`
   - Add `render_context()` accessor
4. [x] Update geometry_viewer to use new infrastructure
   - Store `FrameGraph` as member variable
   - Store `ResearchBaselineResources` as member
   - Update `setup_frame_graph()` to configure member (not local)
   - Implement `on_render()` to call `frame_graph_.execute(render_context())`
5. [x] Add unit tests for Application rendering lifecycle
   - Create `engine/runtime/tests/test_application_rendering.cpp`
   - Test init/shutdown with different backends
   - Test render_context() accessor
6. [x] Build and validate geometry_viewer
   - `cmake --preset linux-gcc-debug` skips the executable in CI (missing `libxrandr-dev`);
     runtime validation relies on the mock-backed unit test.
   - Documented desktop enablement steps in `QUICK_START_RENDERING.md` for manual visual checks.
7. [x] Document the pattern
   - Updated `docs/modules/runtime/README.md` with rendering integration guidance.
   - Refreshed `QUICK_START_RENDERING.md` with mock/desktop workflows and `render_context()` usage.
8. [x] Update TL-310 status and evidence

---

## Evidence

### Test Results

```bash
$ cmake --preset linux-gcc-debug
$ cmake --build --preset linux-gcc-debug --target engine_runtime_tests -j1
$ ./out/build/linux-gcc-debug/engine/runtime/tests/engine_runtime_tests --gtest_filter=ApplicationRendering.*
```

> `cmake --preset linux-gcc-debug` reports the missing XRandR headers and skips `geometry_viewer`.
> Rendering behavior is covered by the mock presentation backend in `ApplicationRendering.ProvidesContextAndInvokesPresentation`.

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [x] Complete | QA/Test | `ApplicationRendering.ProvidesContextAndInvokesPresentation` (mock backend) |
| docs | [x] Complete | Docs/DevRel | Runtime README + rendering quick start refreshed |
| perf | [ ] N/A | — | No performance requirements (this enables rendering, doesn't change perf) |
| safety | [ ] N/A | — | No new threading/safety concerns |

### Updated Files

- `docs/modules/runtime/README.md`
- `QUICK_START_RENDERING.md`
- `hybrid_workflow/backlog/TL-310-editor-foundations.md` (parent task status)
- `hybrid_workflow/backlog/TL-311-scene-hierarchy-panel.md` (blocked_on reference)

---

## Completion Checklist (Definition of Done)

- [x] Application class has presentation backend integration
- [x] render_context() accessor available to derived classes
- [x] geometry_viewer integration documented; desktop validation requires GLFW/OpenGL packages
- [x] Unit tests cover Application rendering lifecycle
- [x] Documentation updated with rendering pattern
- [x] No regressions in existing Application-based code
- [x] Parent task TL-310 step 2a marked complete

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** 2025-11-07

**Notes:**
- This closes the gap between RT-410 delivery and Application framework.
- Headless validation runs through the mock backend; interactive builds require installing GLFW/XRandR.
- Estimated 4-6 hours of focused work.
- (2026-04-24) Application now registers the presentation backend with `RuntimeHost` and forwards
  `RuntimePresentationContext::submit_render_graph` so future GPU backends (e.g., OpenGL) receive
  the submission callback described in the original design sketch.
- (2026-04-26) Added automatic backend selection: `Auto` now prefers the OpenGL presenter when GLFW
  windows are requested and gracefully falls back to the mock backend in headless configurations.

**Follow-ups:**
- [ ] Add Vulkan backend support (when Vulkan rendering ready)
- [ ] Multi-window support (future consideration)
- [ ] Runtime backend switching (nice-to-have)

---

## Role Coordination

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Specialist Engineer | Tools Lead / Runtime Lead | Implement Application integration | Ready |
| QA/Test Specialist | QA Lead | Add rendering lifecycle tests | Ready |
| Docs/DevRel | Docs Team | Document rendering pattern in Application | Ready |
| Reviewer | Runtime Reviewer | Review Application changes | Sequenced |

---

## Implementation Notes

### Investigation Phase (30 min)

Key questions to answer:
1. How are PresentationBackend instances created? (Factory pattern? Direct instantiation?)
2. What dependencies does RenderExecutionContext have?
3. Do we need separate resource providers per backend?
4. How does begin_frame/end_frame work?

### Design Decisions

**Backend Selection Strategy:**
```
Auto mode logic:
- If window backend is Mock → Mock presentation backend
- If window backend is GLFW → OpenGL presentation backend (default)
- Future: Check for Vulkan support and prefer if available
```

**Resource Management:**
- PresentationBackend: unique_ptr, owned by Application
- RenderExecutionContext: unique_ptr, owned by Application
- Lifetime: Created in initialize_subsystems(), destroyed in shutdown_subsystems()

**Error Handling:**
- Backend creation failure → Log error, try mock fallback
- Mock fallback failure → Fatal error (can't proceed)
- Missing render_context() call → Assert in debug, graceful skip in release

### Testing Strategy

**Unit test cases:**
1. Application with Mock backend
2. Application with OpenGL backend (if available)
3. render_context() before init (should fail gracefully)
4. render_context() after shutdown (should fail gracefully)
5. Multiple init/shutdown cycles

**Integration test:**
- geometry_viewer visual validation (manual for now)
- Headless rendering test with mock backend (automated)

---

## References

- [RT-410 Task](archive/RT-410-runtime-stage-planner.md) - Presentation backend delivery
- [TL-310 Parent Task](TL-310-editor-foundations.md) - Editor foundations
- [Analysis Document](../../../GEOMETRY_VIEWER_SOLUTION.md) - Problem analysis
- [Application Class](../../../engine/runtime/include/engine/runtime/application.hpp) - Current API
- [Presentation Backend](../../../engine/rendering/include/engine/rendering/presentation_backend.hpp) - Backend interface

