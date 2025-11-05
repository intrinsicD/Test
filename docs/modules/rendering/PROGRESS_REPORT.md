# Rendering Roadmap Progress Update

**Date**: October 27, 2025  
**Task Completed**: RE-610 - Research Rendering Baseline

## Summary

The **Research Baseline Rendering Preset (RE-610)** is now fully implemented, tested, and documented. This was identified as the highest priority task to advance geometry rendering capabilities.

## What Was Delivered

### 1. ✅ Fixed Research Baseline Implementation
- **Fixed critical bug**: Missing pipe operators (`|`) in ResourceUsage flag combinations that prevented compilation
- All rendering tests now pass successfully

### 2. ✅ Working Example Application
- Created `geometry_viewer` example in `engine/tools/examples/`
- Demonstrates complete workflow: scene setup → frame graph configuration → compilation
- Builds and runs successfully
- Includes comprehensive inline documentation

### 3. ✅ Complete Documentation
- Task documentation: `docs/archive/backlog/legacy/tasks/RE_610_RESEARCH_RENDERING_BASELINE.md`
- Example documentation: `engine/tools/examples/README.md`
- Clear next steps identified

## Current Rendering Capabilities

The rendering module now has:
- ✅ Research baseline preset with forward/deferred shading
- ✅ Frame graph compilation and execution infrastructure
- ✅ OpenGL scheduler with queue normalization
- ✅ Vulkan backend prototype
- ✅ Debug overlay system (normals, UVs, materials, light volumes)
- ✅ Telemetry integration
- ✅ Working example demonstrating the API

## Critical Path to Real-Time Rendering

To achieve **actual on-screen geometry rendering**, you need to complete these two tasks **in order**:

### HIGHEST PRIORITY: T-0120 - GPU Resource Provider Implementation
**Status**: 🟡 In Progress
**Blocks**: All real rendering
**Estimated**: 3-4 days

**What's needed**:
- Implement actual OpenGL buffer creation (vertex/index buffers)
- Implement texture creation and binding
- Upload geometry data to GPU
- Shader compilation and program linking
*2025-12-08 update*: Command buffer allocation, fence/semaphore translation, and frame-graph acquire/release hooks exist via the
new `OpenGLGpuResourceProvider`; actual GPU allocations remain pending.

### SECOND PRIORITY: T-0119 - Command Encoder Implementation
**Status**: ✅ Complete
**Depends on**: T-0120 (for real resource residency)

**What shipped**:
- `FrameGraphPassExecutionContext::command_encoder()` hands passes a backend encoder tied to the scheduler command buffer.
- OpenGL/Vulkan providers translate recorded draws/dispatches into submission payloads consumed by schedulers and diagnostics.
- Command encoder tracing surfaces per-pass draw/dispatch counts for PM-510 telemetry and runtime tooling.
*2025-06-02 update*: The encoder integration is merged; remaining work focuses on feeding real GPU resources once T-0120 lands.

### THIRD PRIORITY: Window & Execution Loop
**Status**: Not Started  
**Estimated**: 1-2 days

**What's needed**:
- Create GLFW window with OpenGL context
- Implement frame graph execution loop
- Present rendered frames to screen
- Basic camera controls

## Technical Architecture

The rendering system is now structured as:

```
Application Layer
  ↓
Research Baseline Preset (RE-610) ← YOU ARE HERE
  ↓
Frame Graph (passes + resources)
  ↓
GPU Scheduler (OpenGL/Vulkan)
  ↓
[MISSING] → GPU Resource Provider (T-0120) ← NEXT STEP
  ↓
Command Encoder (T-0119) ✅
  ↓
OpenGL Driver
```

## Recommendation

**Immediate Action**: Implement **T-0120 (GPU Resource Provider)** next. This is the single most critical blocker preventing actual geometry rendering.

With T-0119 complete, finishing T-0120 will unlock live GPU resources so the shipped encoders can drive on-screen rendering.

## Build & Run

```bash
# Build the example
cd cmake-build-debug
cmake --build . --target geometry_viewer

# Run it
./engine/tools/examples/geometry_viewer

# Run tests
ctest -R rendering
```

All tests pass ✓

