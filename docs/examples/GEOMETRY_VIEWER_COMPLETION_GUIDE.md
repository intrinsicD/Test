# Geometry Viewer Completion Guide

## Overview
The current `geometry_viewer.cpp` is a minimal skeleton that demonstrates initialization but doesn't actually render anything. This guide outlines what's missing to make it a fully functional interactive 3D model viewer.

## What's Currently Missing

### 1. **Window and OpenGL Context**
- ✗ No window creation
- ✗ No OpenGL context initialization
- ✗ No event loop

**What you need:**
```cpp
#include "engine/platform/windowing/window.hpp"
#include <GLFW/glfw3.h>  // For OpenGL context creation

// Create window
auto window = engine::platform::create_window(
    engine::platform::WindowConfig{
        .title = "Geometry Viewer",
        .width = 1920,
        .height = 1080,
        .visible = true,
        .resizable = true,
        .capability_requirements = {
            .require_native_surface = true
        }
    },
    engine::platform::WindowBackend::GLFW
);

// Initialize OpenGL context (via GLFW)
glfwMakeContextCurrent(static_cast<GLFWwindow*>(window->native_handle()));
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
```

### 2. **Main Event/Render Loop**
- ✗ No continuous rendering
- ✗ No event processing
- ✗ No frame timing

**What you need:**
```cpp
while (!window->close_requested())
{
    // 1. Poll window events
    window->pump_events();
    
    // Process events
    engine::platform::Event event;
    while (window->event_queue().poll(event))
    {
        handle_event(event);
    }
    
    // 2. Update scene/camera
    update_camera(delta_time);
    
    // 3. Execute frame graph
    scheduler->execute(graph, scene);
    
    // 4. Render ImGui
    render_ui();
    
    // 5. Swap buffers
    glfwSwapBuffers(window_handle);
}
```

### 3. **Camera System**
- ✗ No camera entity in the scene
- ✗ No camera controller for user input
- ✗ No view/projection matrices set up

**What you need:**
```cpp
#include "engine/rendering/camera.hpp"
#include "engine/rendering/camera_controllers.hpp"

// Create camera entity
auto camera_entity = scene.registry().create();

// Add camera component
auto& camera = scene.registry().emplace<engine::rendering::Camera>(camera_entity);
camera.set_perspective(
    glm::radians(60.0f),  // FOV
    1920.0f / 1080.0f,    // Aspect ratio
    0.1f,                 // Near plane
    1000.0f               // Far plane
);
camera.look_at(
    {0.0f, 2.0f, 5.0f},   // Eye position
    {0.0f, 0.0f, 0.0f},   // Target
    {0.0f, 1.0f, 0.0f}    // Up vector
);

// Add camera controller for user input
auto& controller = scene.registry().emplace<engine::rendering::ArcballCameraController>(
    camera_entity
);
```

### 4. **Actual Mesh Loading**
- ✗ Currently just creates empty mesh handles
- ✗ No actual geometry data
- ✗ No vertex/index buffers

**What you need:**
```cpp
#include "engine/io/importers/mesh.hpp"
#include "engine/geometry/mesh.hpp"

// Load mesh from file
auto mesh_result = engine::io::load_mesh("path/to/model.obj");
if (!mesh_result)
{
    std::cerr << "Failed to load mesh\n";
    return;
}

engine::geometry::Mesh mesh = std::move(mesh_result.value());

// Upload to GPU (through resource provider)
auto gpu_mesh = resource_provider->upload_mesh(mesh);

// Create entity with the mesh
auto entity = scene.registry().create();
scene.registry().emplace<engine::rendering::components::RenderGeometry>(
    entity,
    engine::rendering::components::RenderGeometry::from_mesh(
        gpu_mesh,
        material_handle
    )
);
```

### 5. **Material System**
- ✗ Empty material handles
- ✗ No shaders loaded
- ✗ No textures

**What you need:**
```cpp
#include "engine/rendering/material_system.hpp"

// Create material system
engine::rendering::MaterialSystem material_system;

// Create a basic PBR material
engine::rendering::MaterialDescriptor material_desc{};
material_desc.albedo = {1.0f, 1.0f, 1.0f, 1.0f};
material_desc.metallic = 0.0f;
material_desc.roughness = 0.5f;

auto material = material_system.create_material(material_desc);
```

### 6. **ImGui Integration**
- ✗ No ImGui context
- ✗ No ImGui rendering
- ✗ No UI panels

**What you need:**
```cpp
#include "engine/tools/imgui_helpers.hpp"
#include "engine/tools/imgui/panel_registry.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Initialize ImGui
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();

// Setup platform/renderer bindings
ImGui_ImplGlfw_InitForOpenGL(window_handle, true);
ImGui_ImplOpenGL3_Init("#version 450");

// In render loop:
void render_ui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Render custom UI
    ImGui::Begin("Scene Controls");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::SliderFloat3("Camera Position", &camera_pos.x, -10.0f, 10.0f);
    ImGui::ColorEdit3("Clear Color", &clear_color.x);
    ImGui::End();
    
    // Render profiler
    engine::tools::imgui::render_profiler_window();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

### 7. **Input Handling**
- ✗ No keyboard/mouse input processing
- ✗ No camera movement

**What you need:**
```cpp
#include "engine/platform/input/input_state.hpp"

// Input state
engine::platform::InputState input_state;

// Handle events
void handle_event(const engine::platform::Event& event)
{
    if (event.type == engine::platform::EventType::Resized)
    {
        auto& resize = std::get<engine::platform::ResizeEvent>(event.payload);
        // Update viewport and camera aspect ratio
        glViewport(0, 0, resize.width, resize.height);
        camera.set_perspective(
            glm::radians(60.0f),
            static_cast<float>(resize.width) / resize.height,
            0.1f, 1000.0f
        );
    }
}

// Update camera from input
void update_camera(float delta_time)
{
    // Get mouse delta
    auto mouse_delta = input_state.mouse_delta();
    
    // Rotate camera with right mouse button
    if (input_state.is_mouse_button_down(1))  // Right button
    {
        camera_controller.rotate(mouse_delta.x * 0.01f, mouse_delta.y * 0.01f);
    }
    
    // Zoom with scroll wheel
    float scroll = input_state.scroll_delta();
    camera_controller.zoom(scroll * 0.1f);
    
    camera_controller.update(camera, delta_time);
}
```

### 8. **Frame Graph Execution**
- ✗ Frame graph is created but never executed
- ✗ No scene data passed to renderer

**What you need:**
```cpp
// Execute frame graph with scene data
engine::rendering::FrameGraphExecutionContext context{};
context.scene = &scene;
context.camera_entity = camera_entity;
context.viewport_width = 1920;
context.viewport_height = 1080;

scheduler->execute(graph, context);
```

### 9. **Resource Cleanup**
- ✗ No shutdown/cleanup code

**What you need:**
```cpp
// Before exit:
ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplGlfw_Shutdown();
ImGui::DestroyContext();

glfwDestroyWindow(window_handle);
glfwTerminate();
```

## Complete Minimal Example Structure

```cpp
// Headers
#include "engine/rendering/api.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/platform/windowing/window.hpp"
#include "engine/io/importers/mesh.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>

class GeometryViewerApp
{
public:
    void init()
    {
        // 1. Create window
        // 2. Initialize OpenGL
        // 3. Create scene with camera
        // 4. Load mesh
        // 5. Setup frame graph
        // 6. Initialize ImGui
    }
    
    void run()
    {
        while (!should_close)
        {
            // 1. Poll events
            // 2. Update camera
            // 3. Execute frame graph
            // 4. Render ImGui
            // 5. Swap buffers
        }
    }
    
    void shutdown()
    {
        // Cleanup resources
    }

private:
    std::shared_ptr<engine::platform::Window> window;
    engine::scene::Scene scene;
    engine::rendering::FrameGraph graph;
    // ... other members
};
```

## Priority Implementation Order

1. **Window + OpenGL Context** - Get a window displaying
2. **Event Loop** - Get the application continuously running
3. **Camera** - Add a camera to see the scene
4. **Basic Rendering** - Execute the frame graph to clear the screen
5. **Mesh Loading** - Load and display actual geometry
6. **Input Handling** - Add camera controls
7. **ImGui** - Add UI for controls and diagnostics
8. **Materials/Lighting** - Improve visual quality

## Sample Datasets

Check the `assets/datasets/rendering_sample/` directory for example 3D models you can load.

## Additional Features to Consider

- **Multiple meshes** - Load and display multiple models
- **Lighting controls** - Adjust light position/color via UI
- **Shader hot-reload** - Reload shaders without restarting
- **Screenshot** - Save rendered frames to disk
- **Model transform controls** - Rotate/scale/translate models
- **Grid/axes display** - Visual reference for orientation
- **Performance metrics** - Frame time graphs, vertex counts
- **Asset browser** - UI to load different models at runtime

## Reference Examples

- Look at GLFW samples for window/context setup
- Check ImGui demo for UI inspiration (`ImGui::ShowDemoWindow()`)
- Review the `prototype_harness.cpp` for runtime initialization patterns

