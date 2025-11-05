# Geometry Viewer Architecture Analysis

**Date**: November 3, 2025  
**Reviewer**: GitHub Copilot  
**Status**: Analysis Complete  

## Executive Summary

The `geometry_viewer.cpp` example **directly uses GLFW** instead of leveraging the engine's platform abstraction layer. This is **NOT the correct way to build applications** using this engine. The current implementation bypasses several engine subsystems that are designed to provide proper abstractions, but many of these subsystems are **incomplete or blocked**, forcing developers to work around them.

## Problem Statement

### What's Wrong

1. **Duplicate Windowing**: The app creates both a raw GLFW window AND an engine platform window
2. **Manual Input Handling**: Uses raw GLFW callbacks instead of `engine::platform::input::InputState`
3. **No Runtime Integration**: Bypasses `engine::runtime::RuntimeHost` and `RuntimeLoopPlan`
4. **Manual Main Loop**: Implements its own game loop instead of using runtime's stage planner
5. **Missing Presentation Backend**: No proper integration with rendering presentation system
6. **Incomplete Rendering**: Contains placeholder comments for actual GPU rendering work

### Current Code Structure

```cpp
// ❌ WRONG: Creates raw GLFW window
GLFWwindow* glfw_window = create_glfw_window();
glfwSetWindowUserPointer(glfw_window, &state);
glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);

// ❌ WRONG: Also creates engine window (unused!)
state.window = engine::platform::create_window(config, WindowBackend::GLFW);

// ❌ WRONG: Manual main loop
while (!glfwWindowShouldClose(state.glfw_window)) {
    glfwPollEvents();
    render_frame(state);
    glfwSwapBuffers(state.glfw_window);
}
```

## What Should Be Available (But Isn't Complete)

### 1. **Runtime Application Framework** ⚠️ INCOMPLETE

**What Should Exist:**
```cpp
// Ideal application structure
class GeometryViewerApp {
public:
    void initialize(engine::runtime::RuntimeHost& host);
    void update(double delta_time);
    void render();
};

int main() {
    engine::runtime::RuntimeHost host;
    GeometryViewerApp app;
    
    host.add_stage("simulation", RuntimeLoopPhase::Simulation, 
                   [&](double dt) { app.update(dt); });
    host.add_stage("presentation", RuntimeLoopPhase::Presentation,
                   [&](double dt) { app.render(); });
    
    return host.run();  // Built-in main loop
}
```

**Current State:**
- ✅ `RuntimeHost` exists and can execute `RuntimeLoopPlan`
- ✅ `RuntimeLoopBuilder` can construct declarative stage graphs
- ❌ **No built-in `run()` or main loop** - apps must implement their own
- ❌ **No presentation backend integration** (RT-410 in progress)
- ❌ **No window/input integration** with runtime

### 2. **Platform Integration** ⚠️ INCOMPLETE

**What Should Exist:**
```cpp
// Unified window + input
auto window = engine::platform::create_window(config);

// Engine should handle input internally
auto& input = window->input_state();
if (input.is_key_down(Key::W)) {
    camera.move_forward(dt);
}

// No manual GLFW callbacks needed
window->pump_events();  // Processes all input internally
```

**Current State:**
- ✅ `engine::platform::Window` abstraction exists
- ✅ `engine::platform::input::InputState` exists
- ✅ GLFW backend implementation exists
- ❌ **Window doesn't expose `input_state()` accessor** - no integration!
- ❌ **Input must be wired manually** via GLFW callbacks
- ❌ **No high-level input API** on Window interface

### 3. **Rendering Presentation** 🚫 BLOCKED

**What Should Exist:**
```cpp
// Presentation backend handles swapchain automatically
engine::rendering::PresentationBackend* presentation = 
    engine::rendering::create_presentation_backend(
        window, renderer_backend);

// Runtime automatically calls present
host.set_presentation_backend(presentation);
host.run();  // Handles present internally
```

**Current State:**
- ✅ `PresentationBackend` interface defined
- ✅ Concept of `RuntimePresentationContext` exists
- ❌ **No concrete implementations** (OpenGL/Vulkan backends missing)
- ❌ **RT-410 task in progress** to implement this
- ❌ **No automatic swapchain management**

### 4. **GPU Command Execution** 🚫 BLOCKED

**What Should Exist:**
```cpp
// Frame graph should handle GPU rendering
auto& graph = scene.frame_graph();
graph.compile();
graph.execute(renderer);  // Actually renders geometry
```

**Current State:**
- ✅ Frame graph infrastructure exists
- ✅ Command encoder interfaces defined
- ❌ **T-0119**: Command encoder integration incomplete
- ❌ **T-0120**: GPU resource provider missing
- ❌ **Rendering module marked "BLOCKED"** in roadmap
- ❌ **No actual GPU execution** - just placeholder comments

### 5. **Application/Game Loop Abstraction** ❌ MISSING

**What Should Exist:**
```cpp
// High-level application class
class Application {
public:
    virtual void on_initialize() {}
    virtual void on_update(double dt) {}
    virtual void on_render() {}
    virtual void on_shutdown() {}
    
    int run();  // Built-in main loop with runtime integration
};

class MyApp : public Application {
    void on_update(double dt) override {
        // Game logic here
    }
};

int main() {
    MyApp app;
    return app.run();
}
```

**Current State:**
- ❌ **No `Application` base class** exists anywhere
- ❌ **No high-level app framework** - developers build from scratch
- ❌ **No automatic runtime/window/renderer wiring**

## What's Actually Complete

### ✅ Working Subsystems

1. **Scene Management**
   - ECS-based scene with EnTT
   - Transform components
   - Entity lifecycle

2. **Asset System**
   - Mesh handles
   - Material handles
   - Asset loading infrastructure

3. **Math Primitives**
   - Vectors, matrices, quaternions
   - Transform utilities
   - Camera math

4. **Platform Abstraction (Partial)**
   - Window backend selection
   - Event queue
   - Filesystem utilities

5. **Runtime Loop Planning (Partial)**
   - Declarative stage graphs
   - Dependency resolution
   - Per-phase execution

## Missing Components Analysis

### Critical Missing Pieces

| Component | Status | Impact | Blocking Tasks |
|-----------|--------|--------|----------------|
| Application Framework | ❌ Missing | HIGH | None - not planned |
| Window Input Integration | ⚠️ Incomplete | HIGH | Platform design needed |
| Presentation Backends | 🚫 Blocked | CRITICAL | RT-410 |
| GPU Command Execution | 🚫 Blocked | CRITICAL | T-0119, T-0120 |
| Runtime Main Loop | ⚠️ Incomplete | MEDIUM | RT-410 |
| Swapchain Management | ❌ Missing | HIGH | RT-410 |

### Why Developers Use GLFW Directly

Given the incomplete state of the engine's high-level abstractions, developers are **forced to**:

1. **Create raw GLFW windows** because `engine::platform::Window` doesn't expose input
2. **Write manual event loops** because runtime doesn't provide a `run()` method
3. **Handle input via callbacks** because there's no unified input API
4. **Manually swap buffers** because presentation backends don't exist
5. **Fake rendering** because GPU backends are blocked

This creates **technical debt** as these workarounds must be refactored when the proper systems ship.

## Recommended Architectural Path Forward

### Phase 1: Complete Platform Integration (Priority 1)

```cpp
// Add to engine/platform/include/engine/platform/windowing/window.hpp

class Window {
public:
    // ...existing methods...
    
    /// NEW: Expose unified input state
    [[nodiscard]] virtual input::InputState& input_state() = 0;
    [[nodiscard]] virtual const input::InputState& input_state() const = 0;
    
    /// NEW: Automatic input processing during pump_events
    virtual void pump_events() = 0;  // Already exists, needs to update input
};
```

**Implementation:**
- Wire `InputState` into GLFW backend callbacks
- Auto-update input state during `pump_events()`
- Remove need for manual GLFW callback registration

### Phase 2: Complete Presentation Backend (RT-410)

```cpp
// Implement concrete presentation backends
class GLFWPresentationBackend : public PresentationBackend {
public:
    GLFWPresentationBackend(std::shared_ptr<platform::Window> window);
    
    void present(const RuntimePresentationContext& ctx) override {
        // Execute frame graph
        if (ctx.submit_render_graph) {
            ctx.submit_render_graph(ctx.host, submission_ctx_);
        }
        
        // Swap buffers
        glfwSwapBuffers(native_window_);
    }
};
```

**Implementation:**
- Complete RT-410 task
- Implement OpenGL/Vulkan presentation backends
- Integrate with window swapchain surface API

### Phase 3: Runtime Application Integration

```cpp
// Add to engine/runtime/include/engine/runtime/application.hpp

class Application {
public:
    Application(const ApplicationConfig& config);
    virtual ~Application() = default;
    
    /// Run the application main loop
    int run();
    
protected:
    virtual void on_initialize() {}
    virtual void on_update(double dt) {}
    virtual void on_render() {}
    virtual void on_shutdown() {}
    
    [[nodiscard]] RuntimeHost& runtime() { return *runtime_; }
    [[nodiscard]] platform::Window& window() { return *window_; }
    
private:
    std::shared_ptr<platform::Window> window_;
    std::unique_ptr<RuntimeHost> runtime_;
    std::unique_ptr<PresentationBackend> presentation_;
};
```

**Usage:**
```cpp
class GeometryViewer : public engine::runtime::Application {
    void on_initialize() override {
        setup_scene();
        setup_camera();
    }
    
    void on_update(double dt) override {
        update_camera(dt);
    }
};

int main() {
    GeometryViewer app({.width = 1280, .height = 720});
    return app.run();
}
```

### Phase 4: Complete GPU Execution (T-0119, T-0120)

- Finish command encoder integration
- Implement GPU resource provider
- Enable actual rendering (not just stubs)

## How to Build Apps Today (Temporary Guidance)

Until the above systems are complete, developers should:

### Option A: Minimal GLFW Usage (Current Best Practice)

```cpp
#include "engine/platform/windowing/window.hpp"
#include "engine/runtime/loop.hpp"
#include <GLFW/glfw3.h>  // Temporary - will be removed

int main() {
    // Use engine's GLFW backend, but access native handle
    auto window = engine::platform::create_window(
        config, WindowBackend::GLFW);
    
    // Build runtime loop plan
    engine::runtime::RuntimeLoopBuilder builder;
    builder.add_stage("simulation", RuntimeLoopPhase::Simulation,
                     [](double dt) { /* update logic */ });
    
    auto plan = builder.build();
    engine::runtime::RuntimeHost host;
    host.set_loop_plan(plan.value());
    
    // Manual loop (temporary until RT-410 completes)
    while (!window->close_requested()) {
        window->pump_events();
        host.tick(1.0/60.0);
        
        // TODO: Replace with presentation backend when RT-410 ships
        // window->present();  // Not available yet
    }
}
```

### Option B: Full Engine Path (Future - After RT-410)

```cpp
#include "engine/runtime/application.hpp"  // Doesn't exist yet

class MyApp : public engine::runtime::Application {
    void on_update(double dt) override {
        // Update logic
    }
};

int main() {
    MyApp app({.width = 1280, .height = 720});
    return app.run();  // Everything handled internally
}
```

## Conclusion

### Summary of Findings

1. ❌ **geometry_viewer.cpp is NOT the correct pattern** - it's a workaround
2. ⚠️ **Engine has the right architecture** - but key pieces are incomplete
3. 🚫 **Rendering and presentation are blocked** on GPU milestone tasks
4. ⚠️ **Platform input integration is incomplete** but fixable
5. ❌ **No high-level Application framework** exists

### Action Items

**For Engine Developers:**
1. Complete **RT-410** (Presentation backends) - Priority 1
2. Complete **T-0119/T-0120** (GPU execution) - Priority 1
3. Add **Window::input_state()** integration - Priority 2
4. Consider **Application base class** - Priority 3
5. Provide **proper examples** once systems are complete

**For Application Developers:**
1. **Understand this is temporary** - abstractions are in-progress
2. **Use engine::platform::Window** but expect to refactor
3. **Build with RuntimeLoopBuilder** where possible
4. **Minimize GLFW dependencies** - mark as technical debt
5. **Watch RT-410 and GPU milestones** for migration path

### Timeline Estimate

- **RT-410 Presentation**: In Progress → 2-4 weeks
- **GPU Execution**: Blocked → 4-8 weeks
- **Application Framework**: Not Planned → TBD
- **Full App Support**: 2-3 months (optimistic)

The engine has excellent architectural foundations, but several critical integration layers are incomplete. The current workarounds are **necessary but temporary** until the blocked rendering and runtime tasks ship.

## References

- [RT-410: Runtime Stage Planner](../../hybrid_workflow/backlog/RT-410-runtime-stage-planner.md)
- [T-0119: Command Encoder Integration](../../hybrid_workflow/backlog/T-0119-command-encoder-integration.md)
- [T-0120: GPU Resource Provider](../../hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md)
- [Architecture Overview](../ARCHITECTURE.md)
- [Roadmap](../ROADMAP.md)
- [Platform Module](../modules/platform/README.md)

