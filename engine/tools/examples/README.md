# Geometry Viewer Example

## Overview

The `geometry_viewer` example demonstrates how to use the Test Engine's rendering system to set up geometry rendering
with the research baseline preset.

## Purpose

This example shows developers how to:

- Initialize the OpenGL rendering backend
- Create a scene with renderable geometry entities
- Configure the research baseline rendering preset (forward/deferred shading)
- Compile and prepare a frame graph for rendering
- Understand the rendering pipeline architecture

## Building

The example is automatically built with the rest of the engine:

```bash
cd cmake-build-debug
cmake --build . --target geometry_viewer
```

## Running

```bash
./engine/tools/examples/geometry_viewer
```

## What It Demonstrates

### 1. Rendering Backend Initialization

- Creates a recording GPU resource provider for OpenGL
- Initializes the OpenGL GPU scheduler

### 2. Scene Setup

- Creates an ECS scene using EnTT
- Adds entities with transform and render geometry components
- Demonstrates the factory pattern for creating geometry components

### 3. Research Baseline Preset

- Configures the research rendering baseline (RE-610)
- Sets up forward shading mode
- Configures viewport resolution
- Optional debug overlays (normals, UVs, materials, light volumes)

### 4. Frame Graph

- Creates and configures the frame graph
- Compiles rendering passes
- Shows available render targets (color, depth, G-buffers)

## Next Steps

To create a fully functional geometry rendering application, you would:

1. **Create a window with OpenGL context** - Use the platform module with GLFW
2. **Execute the frame graph each frame** - Call `graph.execute()` with render context
3. **Present to screen** - Swap buffers and display the final color output
4. **Handle input** - Process user input to manipulate camera and scene
5. **Load actual geometry** - Implement mesh loading from files

---

# Prototype Viewer Example

## Overview

The `prototype_viewer` is an interactive 3D model viewer that serves as a visual testing ground for all engine functions. It combines GLFW windowing, ImGui UI, and the engine's rendering pipeline to provide an interactive environment for testing and demonstrating engine capabilities.

## Purpose

This application demonstrates how to:

- Create an interactive window with GLFW
- Integrate ImGui for UI feedback
- Implement trackball camera controls (rotate around object)
- Implement mouse wheel zoom
- Handle drag-and-drop file loading
- Manage OpenGL context and rendering loop
- Provide real-time camera state feedback

## Features

- **Drag-and-Drop**: Load 3D model files by dragging them into the window
- **Trackball Camera**: Left mouse button + drag to rotate camera around the scene
- **Zoom**: Mouse wheel to zoom in/out
- **ImGui UI**: Real-time display of camera state, loaded files, and controls
- **Interactive**: Designed to be extended with more engine features over time

## Building

The example is automatically built with the rest of the engine:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target prototype_viewer
```

## Running

```bash
./out/build/linux-gcc-debug/engine/tools/examples/prototype_viewer
```

Note: Requires a display connection (X11/Wayland on Linux, native display on Windows/macOS).

## Controls

- **Left Mouse + Drag**: Rotate camera (trackball rotation around target)
- **Mouse Wheel**: Zoom in/out (adjusts camera distance)
- **Drag & Drop**: Load 3D model files into the viewer
- **ESC**: Close the application
- **UI Toggle**: ImGui window can be closed/reopened

## Camera System

The camera uses a trackball orbit system:
- **Target**: Point the camera looks at (default: origin)
- **Distance**: How far the camera is from the target
- **Yaw/Pitch**: Rotation angles around the target
- Camera position is computed from these parameters

## Future Extensions

This prototype is designed to be extended with:
- Actual 3D model loading (using IO module)
- Model rendering (using frame graph and rendering pipeline)
- Multiple rendering modes (wireframe, shaded, textured)
- Lighting controls
- Material editing
- Performance metrics display
- Telemetry integration
- Benchmark execution

## Related Code

- Platform/GLFW: `engine/platform/src/windowing/glfw_window.cpp`
- ImGui Integration: `engine/tools/src/imgui_helpers.cpp`
- Rendering Pipeline: `engine/rendering/src/pipeline/research_baseline.cpp`

## Status

✅ **Complete** - Window creation, camera controls, and UI working
⏳ **Next**: Integration with rendering pipeline and 3D model loading

---

## Related Code

- Research Baseline: `engine/rendering/src/pipeline/research_baseline.cpp`
- Frame Graph: `engine/rendering/include/engine/rendering/frame_graph.hpp`
- Render Components: `engine/rendering/include/engine/rendering/components.hpp`
- OpenGL Scheduler: `engine/rendering/src/backend/opengl/`

## Status

✅ **Complete** - Example builds and runs successfully, demonstrating the research baseline rendering preset (RE-610).

The next priority is to implement actual window creation and frame graph execution to achieve real-time geometry
rendering.

