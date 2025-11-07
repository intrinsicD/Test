# Quick Start: Rendering Development Guide

**Date:** 2025-11-07  
**Goal:** Get you rendering stuff ASAP 🚀

---

## ✅ Current Status

Everything is **READY TO GO**:
- ✅ Build system configured
- ✅ All backends implemented (GLFW, Mock, OpenGL)
- ✅ Runtime presentation hooks complete (RT-410)
- ✅ geometry_viewer working at 254k+ FPS
- ✅ Tools module enabled by default

---

## Quick Commands

### 1. Build and Run geometry_viewer (Proven Working!)

```bash
cd /home/alex/Documents/Test

# Build
cmake --build out/build/linux-gcc-debug --target geometry_viewer -j$(nproc)

# Run
./out/build/linux-gcc-debug/engine/tools/examples/geometry_viewer
```

**What you'll see:**
- Interactive 3D viewer with orbit camera
- Real-time FPS counter
- Mouse controls (drag to rotate, scroll to zoom)

### 2. Build Everything

```bash
# Configure (already done, but just in case)
cmake --preset linux-gcc-debug

# Build all
cmake --build out/build/linux-gcc-debug -j$(nproc)

# Run tests
ctest --preset linux-gcc-debug --output-on-failure
```

### 3. Create Your Own Viewer

Copy `geometry_viewer.cpp` as a starting point:

```bash
cp engine/tools/examples/geometry_viewer.cpp engine/tools/examples/my_viewer.cpp
# Edit CMakeLists.txt to add your target
```

---

## Architecture Overview

### Application Framework (Working!)

```cpp
class MyApp : public engine::runtime::Application {
public:
    MyApp() : Application({
        .window = {
            .title = "My Viewer",
            .width = 1280,
            .height = 720,
            .visible = true
        },
        .window_backend = engine::platform::WindowBackend::GLFW,
        .target_fps = 60.0
    }) {}

protected:
    void on_initialize() override {
        // Setup scene, camera, rendering
    }

    void on_update(double delta_time) override {
        // Update logic, input handling
    }

    void on_render() override {
        // Frame graph execution happens here
    }
};

int main() {
    MyApp app;
    return app.run();
}
```

### Key Subsystems

1. **Scene Management** - `engine::scene::Scene`
   - Entity/component system (EnTT-based)
   - Hierarchy with transforms
   - Component helpers

2. **Rendering** - `engine::rendering`
   - Frame graph for pass scheduling
   - Research baseline pipeline (working reference)
   - OpenGL backend (functional)
   - Material system
   - Camera abstraction

3. **Platform** - `engine::platform`
   - Window management (GLFW working)
   - Input handling
   - Event queue

4. **Runtime** - `engine::runtime`
   - Application lifecycle
   - Stage planner (RT-410 complete)
   - Presentation surface
   - Subsystem coordination

---

## Common Patterns

### Create a Scene Entity

```cpp
auto& registry = scene().registry();

// Create entity
auto entity = registry.create();

// Add transform
auto& transform = registry.emplace<engine::scene::components::WorldTransform>(entity);
transform.value = engine::math::Transform<float>::Identity();

// Add renderable geometry
auto mesh = engine::assets::MeshHandle{std::string{"path/to/mesh"}};
auto material = engine::assets::MaterialHandle{std::string{"path/to/material"}};
registry.emplace<engine::rendering::components::RenderGeometry>(
    entity,
    engine::rendering::components::RenderGeometry::from_mesh(mesh, material)
);
```

### Setup Camera

```cpp
// Create camera entity
auto camera_entity = registry.create();

// Add camera component
auto& camera = registry.emplace<engine::rendering::Camera>(camera_entity);

// Configure perspective projection
float aspect = (float)width / height;
camera.set_perspective(
    1.047f,     // ~60 degrees FOV
    aspect,
    0.1f,       // Near plane
    100.0f      // Far plane
);

// Add orbit controller for interactive viewing
registry.emplace<engine::rendering::OrbitCameraController>(
    camera_entity,
    engine::rendering::OrbitCameraController{
        .target = {0.0f, 0.0f, 0.0f},
        .distance = 5.0f,
        .sensitivity = 0.005f
    }
);
```

### Configure Frame Graph

```cpp
// Use research baseline preset (proven working)
auto pipeline_config = engine::rendering::pipeline::create_research_baseline_preset();

// Compile frame graph
auto frame_graph_result = engine::rendering::compile_frame_graph(
    pipeline_config,
    window().native_window()
);

if (!frame_graph_result) {
    // Handle error
}
```

---

## File Locations Reference

### Examples
- `engine/tools/examples/geometry_viewer.cpp` - Working reference implementation

### Headers You'll Need
```cpp
#include "engine/runtime/application.hpp"
#include "engine/platform/input/input_state.hpp"
#include "engine/rendering/api.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/rendering/camera_controllers.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/math/transform.hpp"
```

### Module Documentation
- `docs/modules/rendering/README.md` - Rendering module details
- `docs/modules/runtime/README.md` - Runtime/application framework
- `docs/modules/scene/README.md` - Scene graph and ECS
- `docs/modules/platform/README.md` - Windowing and input
- `docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md` - Runtime architecture

---

## Editor Panel Development (TL-310+)

Once you want to add ImGui panels:

### Panel Registry (Already Implemented)

```cpp
#include "engine/tools/imgui/panel_registry.hpp"

// Register a panel (RAII style)
auto handle = panel_registry.register_scoped_panel({
    .identifier = "my_panel",
    .factory = []() { return std::make_unique<MyPanel>(); },
    .lifecycle = { /* callbacks */ }
});

// Panel auto-unregisters when handle goes out of scope
```

### Next Tasks (Unblocked!)
- **TL-310:** Complete editor smoke tests, docs refresh
- **TL-311:** Scene hierarchy panel (can start soon)
- **TL-312:** Performance metrics panel
- **TL-313:** Asset browser panel
- **TL-314:** Telemetry visualization panel

---

## Performance Notes

Current geometry_viewer achieves:
- **254k+ FPS** on debug build
- Single cube scene
- Interactive camera controls
- Real-time input handling

This proves the pipeline works end-to-end!

---

## Troubleshooting

### Build Issues

If glad::gl_core is missing:
```bash
# Install Jinja2 for Python (required for GLAD generation)
pip install jinja2

# Reconfigure
cmake --preset linux-gcc-debug
```

### Window Not Showing

Check backend selection in preset:
```json
"ENGINE_WINDOW_BACKEND": "GLFW"  // Must be GLFW for visual output
```

Mock backend is for headless/CI only.

### Missing Dependencies

All required dependencies are in `third_party/`:
- GLFW - window management
- glad - OpenGL loader
- ImGui - UI toolkit
- spdlog - logging
- EnTT - entity component system
- stb - image loading

They're fetched automatically via CMake FetchContent.

---

## Next Development Steps

### This Week
1. ✅ **Run geometry_viewer** - Proven reference
2. ✅ **Study the code** - Understand patterns
3. 🔄 **Modify geometry_viewer** - Add your own geometry/effects
4. 🔄 **Complete TL-310** - Editor smoke tests + docs

### Next Sprint
1. **TL-311:** Scene hierarchy panel
2. **Custom viewers:** Build domain-specific tools
3. **Asset pipeline:** Hook up your own mesh/material loading

---

## Key Insight

**You don't have blockers - you have a working reference implementation!**

`geometry_viewer.cpp` is your proof that:
- Runtime lifecycle works
- Window creation works
- Scene management works
- Camera controls work
- Frame graph works
- OpenGL rendering works
- Input handling works

**Clone it, modify it, ship it!** 🚀

---

## Resources

- **Working Example:** `engine/tools/examples/geometry_viewer.cpp`
- **Blocker Analysis:** `TL_BLOCKERS_ANALYSIS.md` (comprehensive breakdown)
- **Module Docs:** `docs/modules/*/README.md`
- **Architecture:** `docs/ARCHITECTURE.md`
- **Roadmap:** `docs/ROADMAP.md`

---

**Bottom Line:** Everything is ready. Start coding! 💪

