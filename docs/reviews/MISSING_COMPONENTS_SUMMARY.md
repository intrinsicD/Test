# Missing Engine Components - Quick Reference

**Date**: November 3, 2025  
**Status**: Gap Analysis Complete  

## TL;DR

**Question**: Why does `geometry_viewer.cpp` use raw GLFW instead of engine abstractions?  
**Answer**: Because the engine's high-level abstractions are **incomplete or blocked**.

## Component Status Matrix

| Component | Exists? | Complete? | Usable? | Blocking Issue |
|-----------|---------|-----------|---------|----------------|
| **Platform::Window** | ✅ Yes | ⚠️ Partial | ⚠️ Limited | No input integration |
| **Platform::InputState** | ✅ Yes | ✅ Complete | ❌ No | Not exposed on Window |
| **Runtime::RuntimeHost** | ✅ Yes | ⚠️ Partial | ⚠️ Limited | No main loop |
| **Runtime::RuntimeLoopPlan** | ✅ Yes | ✅ Complete | ⚠️ Limited | No window integration |
| **Rendering::FrameGraph** | ✅ Yes | ⚠️ Partial | ❌ No | GPU execution blocked |
| **Rendering::PresentationBackend** | ⚠️ Interface | ❌ No | ❌ No | RT-410 in progress |
| **Rendering::CommandEncoder** | ⚠️ Interface | ❌ No | ❌ No | T-0119 blocked |
| **Rendering::GPUResourceProvider** | ❌ No | ❌ No | ❌ No | T-0120 blocked |
| **Runtime::Application** | ❌ No | ❌ No | ❌ No | Not planned |
| **Swapchain Management** | ⚠️ Partial | ❌ No | ❌ No | RT-410 in progress |

## What Works vs What Doesn't

### ✅ What Works Today

```cpp
// Scene management with ECS
auto& scene = engine::scene::Scene();
auto entity = scene.create_entity();

// Runtime loop planning
engine::runtime::RuntimeLoopBuilder builder;
builder.add_stage("update", RuntimeLoopPhase::Simulation, update_fn);
auto plan = builder.build();

// Platform window creation
auto window = engine::platform::create_window(config, WindowBackend::GLFW);

// Asset handles
auto mesh = engine::assets::MeshHandle{"cube.mesh"};
auto material = engine::assets::MaterialHandle{"default.mat"};
```

### ❌ What Doesn't Work

```cpp
// ❌ Window doesn't expose input
auto& input = window->input_state();  // DOESN'T EXIST

// ❌ Runtime doesn't have built-in loop
host.run();  // DOESN'T EXIST

// ❌ No presentation backend
auto presentation = create_presentation_backend(window);  // DOESN'T EXIST

// ❌ Frame graph doesn't actually render
graph.execute(renderer);  // STUB - NO GPU EXECUTION

// ❌ No application framework
class MyApp : public engine::Application {};  // DOESN'T EXIST
```

## Gap Analysis by Subsystem

### 1. Platform Module: 60% Complete ⚠️

**Missing:**
- `Window::input_state()` accessor
- Automatic input processing during `pump_events()`
- High-level input API (no keyboard/mouse convenience methods)
- Input event callbacks

**Workaround:**
```cpp
// Must use raw GLFW callbacks
glfwSetKeyCallback(window, key_callback);
glfwSetMouseButtonCallback(window, mouse_callback);
```

### 2. Runtime Module: 70% Complete ⚠️

**Missing:**
- Built-in main loop (`run()` method)
- Window integration
- Presentation backend integration
- Application lifecycle management

**Workaround:**
```cpp
// Must implement own loop
while (!should_quit) {
    window->pump_events();
    host.tick(dt);
    // manual present
}
```

### 3. Rendering Module: 30% Complete 🚫

**Missing:**
- GPU command execution (T-0119)
- Resource provider (T-0120)
- Presentation backends (RT-410)
- Swapchain management
- Actual shader/buffer execution

**Workaround:**
```cpp
// Can only build frame graph, not execute
graph.compile();  // OK
// graph.execute() is stubbed out - no rendering happens
```

### 4. Application Framework: 0% Complete ❌

**Missing:**
- Base `Application` class
- Automatic subsystem wiring
- Lifecycle callbacks (init/update/shutdown)
- Built-in main loop
- Configuration management

**Workaround:**
```cpp
// Must wire everything manually
int main() {
    // Create window manually
    // Create runtime manually
    // Wire presentation manually
    // Implement main loop manually
}
```

## Why geometry_viewer.cpp Uses GLFW Directly

### The Problem Chain

1. **Window doesn't expose input** → Must use GLFW callbacks
2. **GLFW callbacks need GLFWwindow*** → Must store raw GLFW window
3. **Runtime has no main loop** → Must implement own loop
4. **Runtime loop needs GLFW events** → Must call `glfwPollEvents()`
5. **No presentation backend** → Must call `glfwSwapBuffers()`
6. **No GPU execution** → Must skip rendering entirely

### Result: Dual Window Management

```cpp
// Creates engine window (mostly unused)
state.window = engine::platform::create_window(config, WindowBackend::GLFW);

// Also creates raw GLFW window (actually used)
state.glfw_window = create_glfw_window();
glfwSetWindowUserPointer(state.glfw_window, &state);
glfwSetMouseButtonCallback(state.glfw_window, mouse_button_callback);
```

This is **not ideal** but **necessary given current engine state**.

## Correct Architecture (When Complete)

### Ideal Code (Future)

```cpp
#include "engine/runtime/application.hpp"

class GeometryViewer : public engine::runtime::Application {
public:
    GeometryViewer() : Application({
        .title = "Geometry Viewer",
        .width = 1280,
        .height = 720
    }) {}
    
    void on_initialize() override {
        setup_scene();
        setup_camera();
    }
    
    void on_update(double dt) override {
        auto& input = window().input_state();
        
        if (input.is_mouse_button_down(MouseButton::Left)) {
            auto delta = input.cursor_delta();
            camera_.rotate(delta.x, delta.y);
        }
        
        if (input.was_key_pressed(Key::Escape)) {
            quit();
        }
    }
    
    void on_render() override {
        // Frame graph automatically executes
        // Presentation backend automatically presents
    }
    
private:
    OrbitCamera camera_;
};

int main() {
    GeometryViewer app;
    return app.run();  // Everything handled internally
}
```

### What This Requires

1. ✅ **Application base class** - needs implementation
2. ✅ **Window::input_state()** - needs wiring
3. ✅ **Runtime::run()** - needs implementation
4. ✅ **PresentationBackend** - RT-410 in progress
5. ✅ **GPU execution** - T-0119/T-0120 blocked

**Estimated Timeline**: 2-3 months for full stack

## Immediate Next Steps

### For Engine Core Team

1. **RT-410** (Priority 1): Implement presentation backends
2. **T-0119/T-0120** (Priority 1): Complete GPU execution
3. **Platform Input** (Priority 2): Wire InputState to Window
4. **Application Framework** (Priority 3): Consider base class design

### For Application Developers

1. **Accept temporary workarounds** are necessary
2. **Use RuntimeLoopBuilder** where possible
3. **Minimize GLFW usage** to ease future migration
4. **Mark GLFW dependencies** as technical debt
5. **Plan refactoring** when abstractions ship

## Migration Path

### Phase 1: RT-410 Ships (2-4 weeks)

```cpp
// Can remove manual buffer swap
// window->present();  // Now available!

// But still need manual loop
while (!window->close_requested()) {
    window->pump_events();
    host.tick(dt);
    window->present();
}
```

### Phase 2: GPU Ships (4-8 weeks)

```cpp
// Frame graph actually renders now!
graph.execute();  // Real GPU commands

// Presentation backend works
presentation->present(ctx);
```

### Phase 3: Input Integration (Timeline TBD)

```cpp
// No more GLFW callbacks
auto& input = window->input_state();  // Finally works!
if (input.is_key_down(Key::W)) {
    // Move forward
}
```

### Phase 4: Application Framework (Not Scheduled)

```cpp
// Clean app structure
class MyApp : public engine::Application {
    void on_update(double dt) override { /* ... */ }
};

int main() {
    return MyApp().run();
}
```

## Conclusion

The engine has **solid architectural foundations** but several **critical gaps**:

- **Platform module**: Has windowing, missing input integration
- **Runtime module**: Has loop planning, missing main loop
- **Rendering module**: Has frame graph, missing GPU execution
- **Application layer**: Completely absent

Until these ship, developers **must use workarounds** like direct GLFW access. This is **expected and acceptable** during the transition period.

The `geometry_viewer.cpp` pattern is a **temporary necessity**, not the intended design.

---

**Track Progress**: Watch RT-410, T-0119, and T-0120 in the backlog for completion status.

