# Geometry Viewer Example

## Overview

The `geometry_viewer` example demonstrates how to use the Test Engine's rendering system to set up geometry rendering with the research baseline preset.

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

## Related Code

- Research Baseline: `engine/rendering/src/pipeline/research_baseline.cpp`
- Frame Graph: `engine/rendering/include/engine/rendering/frame_graph.hpp`
- Render Components: `engine/rendering/include/engine/rendering/components.hpp`
- OpenGL Scheduler: `engine/rendering/src/backend/opengl/`

## Status

✅ **Complete** - Example builds and runs successfully, demonstrating the research baseline rendering preset (RE-610).

The next priority is to implement actual window creation and frame graph execution to achieve real-time geometry rendering.

