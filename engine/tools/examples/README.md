# Geometry Viewer Example

## Overview

The `geometry_viewer` is a fully interactive 3D viewer demonstrating the Test Engine's Application framework. It shows
how to build a complete graphics application by inheriting from `runtime::Application` and implementing lifecycle
callbacks. The viewer renders a cube with an orbit camera controller, accepting mouse and keyboard input in real-time.

## Purpose

This example shows developers how to:

- Use the Application framework for automatic subsystem lifecycle management
- Implement lifecycle callbacks (`on_initialize`, `on_update`, `on_render`, `on_shutdown`)
- Integrate GLFW windowing and input handling
- Create and manage an interactive orbit camera controller
- Build an ECS scene with renderable geometry entities
- Configure the research baseline rendering preset (RE-610)
- Handle mouse and keyboard input for interactive camera control

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

### 1. Application Framework Pattern

- Inherits from `runtime::Application` for automatic subsystem management
- Implements lifecycle callbacks for clean separation of concerns:
  - `on_initialize()` - Setup scene, camera, and rendering
  - `on_update(delta_time)` - Process input and update state
  - `on_render()` - Render frame (GPU execution pending RT-410)
  - `on_shutdown()` - Cleanup
- Minimal `main()` function - just create app and call `run()`

### 2. Window & Input Integration

- GLFW backend with native surface support
- 1280x720 resizable window
- Interactive controls:
  - **Left mouse drag**: Rotate camera around target (orbit controller)
  - **Mouse scroll**: Zoom in/out (1.0 to 20.0 units)
  - **ESC**: Exit application
- Real-time FPS display with camera state

### 3. Scene Management

- ECS-based scene using EnTT registry
- Cube entity with transform and render geometry components
- Demonstrates factory pattern for creating geometry components
- Material and mesh handle references

### 4. Camera System

- Perspective camera with ~60° FOV and aspect ratio
- Orbit camera controller with:
  - Yaw and pitch rotation (pitch clamped to avoid gimbal lock)
  - Radius-based zoom
  - Look-at positioning
- Smooth camera updates each frame

### 5. Rendering Setup

- Research baseline configuration (RE-610)
- Forward shading mode
- Frame graph compilation with color and depth outputs
- Viewport resolution configuration

**Note:** Full GPU command execution is pending completion of RT-410 (Runtime Stage Planner). Currently demonstrates
setup and interactivity; GPU submission will be enabled when RT-410 presentation backends land.

## Next Steps

The viewer demonstrates a complete interactive application using the Application framework. To extend it, you could:

1. **Add more geometry** - Load meshes from files using the assets module
2. **Implement lighting** - Add point lights, directional lights, or spotlights
3. **Material system** - Create and apply different materials with textures
4. **Post-processing** - Add effects using additional frame graph passes
5. **UI overlay** - Integrate Dear ImGui for controls and diagnostics

The full rendering pipeline will execute GPU commands once RT-410 (Runtime Stage Planner & Presentation Loop) is
complete, enabling real-time geometry rendering to screen.

## Related Code

- Application Framework: `engine/runtime/include/engine/runtime/application.hpp`
- Research Baseline: `engine/rendering/src/pipeline/research_baseline.cpp`
- Frame Graph: `engine/rendering/include/engine/rendering/frame_graph.hpp`
- Render Components: `engine/rendering/include/engine/rendering/components.hpp`
- Camera System: `engine/rendering/include/engine/rendering/camera.hpp`
- Input System: `engine/platform/input/input_state.hpp`
- GLFW Backend: `engine/platform/src/window/glfw/`

## Status

✅ **Complete** - Interactive viewer with Application framework integration, orbit camera controls, and input handling.

⚠️ **Note** - Full GPU rendering execution to screen is pending RT-410 (Runtime Stage Planner) completion. The viewer
currently demonstrates the complete Application lifecycle, input handling, camera control, and frame graph setup. GPU
command submission and presentation will be enabled when RT-410 presentation backends are integrated.

