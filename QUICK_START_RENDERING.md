# Quick Start: Rendering Development Guide

**Date:** 2025-11-07  
**Goal:** Get you rendering stuff ASAP 🚀

---

## ✅ Current Status

The runtime/application pipeline now owns the presentation lifecycle:
- ✅ `Application` constructs a `RenderExecutionContext` and presentation backend when rendering is enabled.
- ✅ Mock presentation backend keeps headless CI builds green and powers automated tests.
- ✅ Runtime presentation hooks from [`RT-410`](hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) are integrated.
- ⚠️ Interactive `geometry_viewer` binaries require GLFW/XRandR development packages; headless containers skip that target by default.

---

## Quick Commands

### 1. Configure the toolchain

```bash
cmake --preset linux-gcc-debug
```

The preset disables GLFW automatically when the container lacks `libxrandr-dev`. You will see a
warning and CMake will skip the `geometry_viewer` executable.

### 2. Build the runtime rendering tests

```bash
cmake --build --preset linux-gcc-debug --target engine_runtime_tests
```

This target compiles the Application rendering fixtures together with the mock presentation backend.

### 3. Run the Application rendering suite

```bash
ctest --preset linux-gcc-debug -R ApplicationRendering --output-on-failure
```

The `ApplicationRendering.ProvidesContextAndInvokesPresentation` case verifies that `Application`
constructs a render context, drives begin/end frame, and triggers presentation callbacks even in
headless mode.

### 4. (Optional) Enable geometry_viewer on a desktop

Install GLFW/XRandR development packages, then rebuild:

```bash
sudo apt install libglfw3-dev libxrandr-dev
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target geometry_viewer
./out/build/linux-gcc-debug/engine/tools/examples/geometry_viewer
```

The binary will open a windowed viewer with orbit camera controls once the system packages are
present. In headless CI the mock backend remains the supported validation path.

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
        .window_backend = engine::platform::WindowBackend::Mock,
#if ENGINE_ENABLE_RENDERING
        .rendering = {
            .enable = true,
            .backend = engine::runtime::ApplicationConfig::RenderingConfig::Backend::Mock,
        },
#endif
    }) {}

protected:
    void on_initialize() override {
        // Setup scene, camera, rendering
    }

    void on_update(double delta_time) override {
        // Update logic, input handling
    }

    void on_render() override {
#if ENGINE_ENABLE_RENDERING
        // Example: viewer_frame_graph.execute(render_context());
#endif
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
   - Mock presentation backend (default for CI)
   - OpenGL backend (requires GLFW/XRandR packages)
   - Material system
   - Camera abstraction

3. **Platform** - `engine::platform`
   - Window management (Mock backend in CI, GLFW available on desktops)
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

### Execute the Frame Graph inside `Application::on_render`

The snippet below assumes `frame_graph_` holds the compiled frame graph from the previous step.

```cpp
void on_render() override
{
#if ENGINE_ENABLE_RENDERING
    if (frame_graph_) {
        frame_graph_->execute(render_context());
    }
#endif
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

The default CI configuration runs against the mock presentation backend, so no GPU work is executed
and no frame rate is reported. Profile rendering performance on a workstation after installing the
GLFW/OpenGL dependencies and running the viewer locally. The mock backend keeps tests deterministic
while still covering the Application integration path.

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

- Confirm `ApplicationConfig::window_backend` is set to `platform::WindowBackend::GLFW` (or the
  equivalent JSON preset entry if you are launching through tooling).
- Install the system packages listed in [Quick Commands](#4-optional-enable-geometry_viewer-on-a-desktop).
- If the code runs inside the CI container it will fall back to the mock backend and no window is
  expected.

### Missing Dependencies

CMake fetches engine-owned dependencies from `third_party/`, but platform libraries such as GLFW
still require system headers and shared libraries. Install `libglfw3-dev`, `libxrandr-dev`, and
matching OpenGL drivers on the host if you need an interactive build. The mock backend has no
external dependencies and remains available without additional packages.

---

## Next Development Steps

### Immediate
1. ✅ **Run Application rendering tests** – Use the commands above to verify the mock backend path.
2. ✅ **Document the integration** – Update module READMEs and quick starts (this document).
3. 🔄 **Install desktop dependencies (optional)** – Add GLFW/XRandR if you need an interactive viewer.

### Upcoming
1. **TL-311:** Scene hierarchy panel – depends on TL-310 and this rendering work.
2. **Custom viewers:** Build domain-specific tools on top of `Application` once rendering is enabled.
3. **Performance validation:** Capture GPU metrics after enabling a real backend on hardware.

---

## Key Insight

The `Application` base class now drives rendering end-to-end when `ApplicationConfig::rendering.enable`
is set. Use the mock backend to exercise the workflow in CI. On workstations with GLFW/OpenGL
dependencies installed, set `ApplicationConfig::rendering.backend` to `Auto` (with a GLFW window) or
`OpenGL` to launch the interactive presenter. The configuration gracefully falls back to the mock
backend when native surfaces are unavailable. `engine/tools/examples/geometry_viewer.cpp` remains the
authoritative reference for frame-graph setup even though the binary is skipped in headless builds.

---

## Resources

- **Working Example:** `engine/tools/examples/geometry_viewer.cpp`
- **Blocker Analysis:** `TL_BLOCKERS_ANALYSIS.md` (comprehensive breakdown)
- **Module Docs:** `docs/modules/*/README.md`
- **Architecture:** `docs/ARCHITECTURE.md`
- **Roadmap:** `docs/ROADMAP.md`

---

**Bottom Line:** Rendering is wired through `Application` today—run the mock-backed tests in CI and
add the GLFW/OpenGL packages locally when you are ready for an interactive viewer.

