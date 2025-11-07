# Geometry Viewer - Concrete Implementation Guide

**CRITICAL FINDING:** The `on_render()` callback is already being called every frame by `Application::run_main_loop()`!

The issue is that `on_render()` in geometry_viewer.cpp is **completely empty** (just a comment).

---

## The Problem (Lines 97-101 in geometry_viewer.cpp)

```cpp
void on_render() override
{
    // Note: In full implementation with RT-410, we would:
    // - Execute frame graph with scene
    // - Render all geometry through presentation backend
    // - Present the final image
}
```

**This is a stub!** Nothing actually renders.

---

## The Solution

### Issue 1: Frame Graph is Local Variable

In `setup_frame_graph()`, the frame graph is created locally and destroyed:

```cpp
void setup_frame_graph()
{
    engine::rendering::FrameGraph graph;  // LOCAL! Gets destroyed!
    // ... configure and compile ...
}
```

**Fix:** Make it a member variable.

### Issue 2: No Rendering Execution Context

`FrameGraph::execute()` requires a `RenderExecutionContext`, but Application doesn't provide one yet.

**This is the RT-410 gap:** The presentation backend integration isn't wired up in the Application class yet!

---

## What RT-410 Delivered vs What's Missing

### ✅ RT-410 Delivered (Confirmed)

1. `RuntimeStagePlanner` - Stage scheduling
2. `PresentationBackend` interface - Mock, GLFW, OpenGL implementations
3. `PresentationSurface` - Window ↔ backend bridge
4. Frame graph execution infrastructure

### ❌ What's Still Missing

**The Application class doesn't integrate with PresentationBackend!**

Looking at `application.cpp`:
- `run_main_loop()` calls `on_render()` ✅
- But there's **no RenderExecutionContext** ❌
- No **PresentationBackend** instance ❌  
- No **GPU resource provider** integration ❌

**This is why geometry_viewer has a stub comment referencing RT-410!**

---

## The Real Task: TL-310 Prerequisite Work

### What Actually Needs To Happen

**Task:** Integrate presentation backend into Application framework

**Files that need changes:**

1. **`engine/runtime/include/engine/runtime/application.hpp`**
   - Add `RenderExecutionContext` member
   - Add `PresentationBackend` pointer
   - Add accessor methods

2. **`engine/runtime/src/application.cpp`**
   - Initialize presentation backend in `initialize_subsystems()`
   - Create/bind render execution context
   - Update `run_main_loop()` to begin/end frame
   - Clean up in `shutdown_subsystems()`

3. **`engine/tools/examples/geometry_viewer.cpp`**
   - Store frame graph as member
   - Implement `on_render()` to call `frame_graph_.execute(get_render_context())`

---

## Your Question Answered

### "Which task should I work on to finish geometry_viewer?"

**Answer:** You need to work on **Application ↔ PresentationBackend integration**

This work is likely **part of TL-310** but not explicitly called out in the task steps!

### Why This Isn't Done Yet

Looking at TL-310 status:
```
1. [x] Update CMake presets (ENGINE_ENABLE_TOOLS=ON)
2. [ ] Implement panel registry and editor harness bridge  <-- Current step
3. [ ] Restore unit tests
...
```

The focus has been on **panel registry** (for ImGui), not on **presentation backend wiring**.

**RT-410 delivered the backends, but Application doesn't use them yet!**

---

## Two Paths Forward

### Path A: Complete the Integration (Recommended)

**What:** Integrate PresentationBackend into Application framework

**Effort:** 4-6 hours

**Impact:** 
- ✅ Unlocks geometry_viewer rendering
- ✅ Provides foundation for TL-310+ editor work
- ✅ Makes Application actually useful for rendering apps

**Steps:**

1. **Study RT-410 presentation backend API** (1 hour)
   ```bash
   cat engine/rendering/include/engine/rendering/presentation_backend.hpp
   cat engine/runtime/include/engine/runtime/presentation_surface.hpp
   cat engine/rendering/include/engine/rendering/render_pass.hpp | grep -A20 "RenderExecutionContext"
   ```

2. **Design Application integration** (1 hour)
   - How should config specify backend? (OpenGL vs Vulkan vs Mock)
   - Where to create RenderExecutionContext?
   - When to call begin_frame/end_frame?
   - How to expose to derived classes?

3. **Implement Application changes** (2 hours)
   - Add members to Application class
   - Wire up in initialize_subsystems()
   - Update run_main_loop()
   - Add accessors for derived classes

4. **Update geometry_viewer** (30 min)
   - Store frame graph as member
   - Implement on_render() properly

5. **Test and validate** (1 hour)
   - Build and run
   - Verify rendering works
   - Check for leaks/crashes
   - Document the pattern

6. **Update documentation** (30 min)
   - Update TL-310 status
   - Document the new Application API
   - Update QUICK_START_RENDERING.md

### Path B: Minimal Hack (Quick but Incomplete)

**What:** Make geometry_viewer work standalone without fixing Application

**Effort:** 2-3 hours

**Impact:**
- ✅ geometry_viewer renders
- ❌ Doesn't help other applications
- ❌ Duplicates code that should be in Application
- ❌ Will need refactoring later

**Steps:**

1. Create presentation backend directly in geometry_viewer
2. Manage render context manually
3. Execute frame graph with custom context
4. Handle present calls

**Why not recommended:** You'd be duplicating RT-410 work that should be in Application.

---

## My Strong Recommendation

### 🎯 Work on Application ↔ PresentationBackend Integration

**Rationale:**

1. **This is the missing piece** - RT-410 delivered backends, but they're not connected to Application
2. **Unblocks everything** - geometry_viewer, TL-310, TL-311-314 all need this
3. **Right layer** - Application should manage presentation, not individual examples
4. **Completes RT-410** - Fulfills the "runtime presentation hooks" promise
5. **High leverage** - One fix enables many use cases

### This Work Belongs In TL-310

Update TL-310 to add this step:

```markdown
1. [x] Update CMake presets to re-enable tools module
2. [ ] **Integrate presentation backend into Application framework** ⬅️ ADD THIS
3. [ ] Implement panel registry and editor harness bridge
4. [ ] Restore unit tests
...
```

---

## Immediate Action Plan

### Phase 1: Investigation (30 min - DO THIS NOW)

```bash
cd /home/alex/Documents/Test

# 1. Read presentation backend interface
cat engine/rendering/include/engine/rendering/presentation_backend.hpp

# 2. Check what RenderExecutionContext needs
grep -r "struct RenderExecutionContext" engine/rendering/include/ -A30

# 3. See how backends are created
grep -r "create.*presentation.*backend\|PresentationBackend.*create" \
  engine/rendering/src/backend/ --include="*.cpp" -B3 -A10

# 4. Check if there's already a factory
cat engine/rendering/include/engine/rendering/api.hpp | grep -i present
```

### Phase 2: Design (1 hour)

Create a design document:
```bash
# Create docs/design/APPLICATION_PRESENTATION_INTEGRATION.md
```

Outline:
- How Application gets a PresentationBackend
- How RenderExecutionContext is managed
- API for derived classes to access rendering
- Lifecycle (init/shutdown/per-frame)
- Configuration options

### Phase 3: Implement (2-3 hours)

1. Modify `Application` class
2. Update `geometry_viewer` to use it
3. Add tests

### Phase 4: Validate (1 hour)

1. Build and run geometry_viewer
2. Verify actual rendering
3. Check memory/resource cleanup
4. Run tests

---

## Code Sketch (What You'll Implement)

### Application.hpp changes:

```cpp
class Application
{
protected:
    // ...existing accessors...
    
    /// \brief Access render execution context (available in on_render)
    [[nodiscard]] rendering::RenderExecutionContext& render_context() noexcept;

private:
    // ...existing members...
    
    std::unique_ptr<rendering::PresentationBackend> presentation_backend_;
    std::unique_ptr<rendering::RenderExecutionContext> render_context_;
};
```

### Application.cpp changes:

```cpp
void Application::initialize_subsystems()
{
    // ...existing window/scene init...
    
    // Create presentation backend
    rendering::PresentationConfig present_config{
        .backend_type = config_.render_backend,  // Add to ApplicationConfig
        .window = window_.get(),
        .enable_vsync = true
    };
    
    presentation_backend_ = rendering::create_presentation_backend(present_config);
    presentation_backend_->initialize();
    
    // Create render execution context
    render_context_ = std::make_unique<rendering::RenderExecutionContext>();
    // ... configure context ...
}

void Application::run_main_loop()
{
    while (running_ && !window_->close_requested())
    {
        // ...existing timing/input/update...
        
        // Begin frame
        presentation_backend_->begin_frame(*render_context_);
        
        // Call user render callback (now has access to render_context())
        on_render();
        
        // End frame and present
        presentation_backend_->end_frame(*render_context_);
        presentation_backend_->present();
        
        // ...existing frame limiting...
    }
}
```

### geometry_viewer.cpp changes:

```cpp
class GeometryViewerApp : public engine::runtime::Application
{
private:
    engine::rendering::FrameGraph frame_graph_;  // Now a member!
    engine::rendering::ResearchBaselineResources resources_;
    
    void setup_frame_graph()
    {
        // Configure frame_graph_ member (not local!)
        resources_ = engine::rendering::configure_research_baseline(
            frame_graph_, options);
        frame_graph_.compile();
    }
    
    void on_render() override
    {
        // NOW THIS WORKS!
        frame_graph_.execute(render_context());
    }
};
```

---

## Summary

### The Real Answer

**To finish geometry_viewer, you must first integrate PresentationBackend into Application.**

RT-410 delivered the backends, but Application doesn't use them. This integration work is:
- **Required for geometry_viewer**
- **Required for TL-310+**
- **Should be added as a TL-310 step**
- **Estimated 4-6 hours**

### What To Do Next

1. ✅ **Investigate** presentation backend API (30 min)
2. ✅ **Design** Application integration (1 hour)  
3. ✅ **Implement** and test (3 hours)
4. ✅ **Update** geometry_viewer (30 min)
5. ✅ **Document** for others (30 min)

**Start with investigation - read the files I listed above!**

Then you'll know exactly what the Application integration needs to look like.

