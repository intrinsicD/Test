# Geometry Viewer - What's Missing to Actually Render

**Current State:** Interactive viewer with Application framework, input handling, camera controls, and frame graph setup - but no GPU rendering execution.

**Goal:** See a spinning cube rendered on screen.

---

## Priority 1: GPU Resource Creation (T-0120)

The frame graph compiles but can't create actual GPU resources (buffers, textures, shaders).

### What's Missing:
- [ ] **GPU Buffer Creation** - Vertex/index buffers for cube geometry
- [ ] **Shader Compilation** - Vertex and fragment shaders for the research baseline
- [ ] **Texture/Sampler Creation** - Material textures and samplers
- [ ] **Pipeline State Objects** - Render state configuration (depth, blending, etc.)
- [ ] **Resource Provider Implementation** - Switch from recording provider to real GPU provider

### Current Blocker:
The `ResourceProvider` interface exists but only has a "recording" implementation that doesn't actually allocate GPU memory or compile shaders.

**Estimated Work:** 2-3 days
**Files to Modify:**
- `engine/rendering/src/backend/opengl/opengl_resource_provider.cpp`
- `engine/rendering/src/backend/vulkan/vulkan_resource_provider.cpp`
- `engine/rendering/include/engine/rendering/resource_provider.hpp`

---

## Priority 1: Command Encoding & Submission (T-0119)

✅ **Complete.** Frame-graph passes now receive backend encoders, record draw/dispatch work, and submit through OpenGL and Vulkan schedulers.

### What Shipped:
- `FrameGraph::execute` acquires encoders from the provider and finalises them before queue submission.
- OpenGL/Vulkan command encoder providers translate recorded work into scheduler submissions and telemetry payloads.
- Tracing encoders expose per-pass draw/dispatch counts consumed by runtime diagnostics and PM-510 demos.

**Follow-up Focus:** ensure real GPU resources arrive with [`T-0120`](../../hybrid_workflow/backlog/T-0120-gpu-resource-provider.md) so the shipped encoder executes against live buffers/textures.

---

## Priority 1: Presentation Backend (RT-410)

The GPU can render but we need to present the result to the screen.

### What's Missing:
- [ ] **Swapchain/Framebuffer Creation** - Where to render the final image
- [ ] **Present Operation** - Display rendered image in window
- [ ] **V-Sync/Frame Pacing** - Control frame timing
- [ ] **Resize Handling** - Recreate swapchain when window resizes
- [ ] **Integration with Application::on_render()** - Hook up the presentation loop

### Current Blocker:
No presentation backend exists. The window is created but nothing connects rendering output to it.

**Estimated Work:** 1-2 days
**Files to Modify:**
- `engine/runtime/src/presentation/` (new directory)
- `engine/rendering/src/backend/opengl/opengl_swapchain.cpp` (new file)
- `engine/runtime/src/application.cpp` (wire up on_render)

---

## Priority 2: Geometry Data

The viewer references cube geometry but the actual vertex/index data doesn't exist.

### What's Missing:
- [ ] **Cube Mesh Data** - Vertex positions, normals, UVs, indices
- [ ] **Material Data** - Default material with color/texture info
- [ ] **Asset Loading** - Load mesh/material from files or create procedurally
- [ ] **Geometry Upload** - Transfer CPU mesh data to GPU buffers

### Current Blocker:
`examples/cube.mesh` and `examples/default.material` are referenced but don't exist.

**Estimated Work:** Few hours
**Quick Fix:** Generate procedural cube in code instead of loading from file
**Files to Modify:**
- `engine/tools/examples/geometry_viewer.cpp` (add procedural cube generation)
- OR create `assets/examples/cube.mesh` and `default.material`

---

## Priority 3: Shader Code

The research baseline needs actual shader programs.

### What's Missing:
- [ ] **Vertex Shader** - Transform vertices, pass through attributes
- [ ] **Fragment Shader** - Calculate lighting/color for pixels
- [ ] **Shader Compilation Pipeline** - Compile GLSL/SPIR-V at runtime or offline
- [ ] **Shader Parameter Binding** - MVP matrices, lighting params, material params

### Current Blocker:
Research baseline references shaders but they're not implemented.

**Estimated Work:** 1 day
**Files to Create/Modify:**
- `engine/rendering/shaders/research_baseline_forward.vert`
- `engine/rendering/shaders/research_baseline_forward.frag`
- `engine/rendering/src/pipeline/research_baseline.cpp` (shader loading)

---

## Minimal Path to Rendering (Quick Win)

Here's the **fastest path** to see something on screen:

### Phase 1: Minimal OpenGL Backend (2-3 days)
1. Implement basic OpenGL resource provider (buffers, shaders only)
2. Implement basic OpenGL command encoder (draw calls only)
3. Create simple vertex/fragment shaders (MVP transform + solid color)
4. Generate procedural cube geometry in code
5. Implement basic GLFW swapchain (no resize, no v-sync)

### Phase 2: Wire Up Presentation (1 day)
1. Call `graph.execute()` in `on_render()`
2. Present swapchain to window
3. Clear color buffer each frame

### Phase 3: Fix Camera/Input Integration (few hours)
1. Pass camera matrices to shaders
2. Verify orbit controls work with rendering

### Expected Result:
A spinning, lit cube rendered at 60+ FPS with working camera controls.

---

## Recommended Execution Order

### Week 1: OpenGL Minimal Backend
- **Day 1:** T-0120 Part 1 - Buffer creation & shader compilation
- **Day 2:** T-0119 Part 1 - Command encoder draw calls
- **Day 3:** RT-410 Part 1 - Basic presentation/swapchain

### Week 2: Integration & Polish
- **Day 1:** Procedural cube + basic shaders
- **Day 2:** Wire up frame graph execution
- **Day 3:** Test, debug, fix issues

### Week 3: Vulkan & Full Features (Optional)
- Repeat for Vulkan backend
- Add textures, lighting, materials
- Implement resize handling

---

## Current Code Gaps in geometry_viewer.cpp

Looking at the current implementation, here's what needs to be added to `on_render()`:

```cpp
void on_render() override
{
    // MISSING: Get or create frame graph (stored as member)
    // MISSING: Update frame graph with current window size
    // MISSING: Get camera matrices from camera entity
    // MISSING: Gather renderable entities from scene
    // MISSING: Execute frame graph with scene data
    // MISSING: Present rendered image to window
    // MISSING: Handle any errors
}
```

---

## Bottom Line

**To actually render:**
1. ✅ You have: Window, input, camera, scene setup (done!)
2. ❌ You need: GPU resources, command recording, presentation (3 Priority-1 tasks)
3. 🎯 Quick path: Focus on OpenGL backend only, minimal features, ~1 week of work

**Recommendation:** Start with T-0120 (GPU resources) since nothing else can work without it.


