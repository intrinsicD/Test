# Rendering Module - Quick Start for Geometry Rendering

## Current Status (October 27, 2025)

✅ **Research Baseline Preset (RE-610)** - COMPLETE
✅ **GPU Resource Provider (T-0120)** - COMPLETE (OpenGL/Vulkan allocations, telemetry, runtime submission wiring)
✅ **Command Encoder (T-0119)** - COMPLETE (frame-graph passes now record draw/dispatch work through OpenGL/Vulkan providers)

## Quick Test

```bash
# Build and run the example
cd cmake-build-debug
cmake --build . --target geometry_viewer -j$(nproc)
./engine/tools/examples/geometry_viewer

# Run all rendering tests
ctest -R rendering
```

Expected output:
```
=== Test Engine Geometry Viewer ===
Research Baseline Rendering Example

Initializing OpenGL rendering backend...
Creating scene...
Scene created with 1 cube entity
Configuring research baseline rendering preset...
Compiling frame graph...

Frame graph configured with resources:
  - Final color output: ✓
  - Depth buffer: ✓
  - G-Buffer albedo: ✗
  - G-Buffer normals: ✗

✓ Geometry viewer example completed successfully
```

## What Works Now

1. **Scene Setup**: Create entities with geometry and transform components
2. **Research Baseline**: Configure forward/deferred shading presets
3. **Frame Graph**: Compile rendering passes with resource dependencies
4. **Backend Scheduling**: OpenGL/Vulkan command queue management

## What's Next

1. **Runtime Stage Planner (RT-410)**
   - Integrate presentation adapters with runtime submission context
   - Synchronise GPU timelines/fences with runtime loop phases

2. **Editor/Tooling Re-Enablement (TL-310)**
   - Hook PM-510 telemetry into editor panels once stage planner exposes presentation callbacks

3. **Window & Presentation Polish**
   - Finalise GLFW window creation + swap chain loop for demos
   - Surface retention/telemetry controls in diagnostics tooling

## Next Task: RT-410

To ship interactive demos, complete the runtime stage planner integration so the presentation backend can drive the GPU-backed submission stack without manual wiring.

**Location**: `hybrid_workflow/backlog/RT-410-runtime-stage-planner.md`

**Key integration points**:
```cpp
rendering::backend::opengl::OpenGLRuntimeSubmission submission(mesh_resolver);
submission.set_retention_frames(2);
auto context = submission.make_context(materials, frame_graph, &pipeline);
runtime::submit_render_graph(host, context);
```

## Timeline Estimate

- **RT-410** (Runtime Stage Planner): 3-4 days
- **TL-310** (Editor Foundations follow-up): 2-3 days
- **Presentation Polish**: 1-2 days

**Total**: ~1 week to fully interactive GPU demos with tooling hooks

## Files Modified Today

- `engine/rendering/src/pipeline/research_baseline.cpp` - Fixed syntax errors
- `engine/tools/examples/geometry_viewer.cpp` - Created working example
- `engine/tools/examples/CMakeLists.txt` - Added build configuration
- `engine/tools/CMakeLists.txt` - Integrated examples subdirectory
- Documentation files created in `docs/` and `engine/tools/examples/`

