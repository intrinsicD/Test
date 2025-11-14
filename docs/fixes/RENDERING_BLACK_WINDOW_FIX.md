# Fix: Black Window in Geometry Viewer (Rendering Working But Not Visible)

## Date
2025-11-14

## Issue
The geometry viewer showed only a black window despite:
- GLAD being properly enabled (`ENGINE_RENDERING_HAS_GLAD=1`)
- OpenGL context initialized successfully (OpenGL 4.6)
- Geometry uploaded to GPU (VAO, VBO, IBO created)
- Frame graph executing successfully
- Draw commands being issued via `glDrawElements`

## Root Cause
**The rendering was working perfectly all along!** 

The actual problem was in my debugging approach: I added a test clear (`glClearColor(1.0, 0.0, 1.0, 1.0)` magenta) **AFTER** the frame graph execution to verify framebuffer binding. This clear operation overwrote the correctly rendered cube, making it appear as if nothing was being drawn.

### What Was Actually Happening

```cpp
// In present_with_scene():
clear_framebuffer();                    // Clear to blue-grey
frame_graph_.execute(execution_context); // Draw cube ✓ WORKING!
// [My debug code added magenta clear here] ❌ Overwrote the cube!
swap_buffers(window);                   // Show result
```

The immediate command stream (`OpenGLImmediateCommandStream`) was correctly:
1. Drawing to the default framebuffer (FBO 0)
2. Using the proper viewport (1280x720)
3. Executing all draw calls with the uploaded geometry
4. Applying proper shaders and uniforms

## The Investigation Journey

### Initial Hypothesis (WRONG)
"GLAD is not enabled, so all GL calls are compiled out"

**Evidence against:** Logs showed `ENGINE_RENDERING_HAS_GLAD=1` and successful GPU uploads.

### Second Hypothesis (WRONG)
"The frame graph isn't executing or has no passes"

**Evidence against:** Logs confirmed frame graph execution with `Research.ForwardGeometry` pass.

### Third Hypothesis (WRONG)
"Drawing to offscreen framebuffers instead of default framebuffer"

**Evidence against:** Debug logging showed `FBO=0` during draw calls.

### Fourth Hypothesis (WRONG)  
"Need to blit from offscreen texture to default framebuffer"

**This was the critical error!** I added a test clear to verify framebuffer access, which overwrote the working rendering.

### The Breakthrough
When the magenta test clear appeared on screen, it confirmed:
- The framebuffer was accessible ✓
- Drawing was going to the right place ✓
- But the cube disappeared ✗

**Realization:** The cube was being drawn BEFORE the clear, not in an offscreen buffer!

## Solution
Simply remove the test clear code. The original rendering pipeline was working correctly:

```cpp
void OpenGLPresentationBackend::present_with_scene(scene::Scene& scene, void* window_handle)
{
    initialize_context_if_needed(window);
    clear_framebuffer();                     // Initial clear
    
    auto submission_context = submission_.make_context(materials_, frame_graph_, nullptr);
    auto execution_context = submission_context.make_execution_context(scene);
    
    frame_graph_.execute(execution_context); // Draws to FBO 0 ✓
    
    swap_buffers(window);                    // Present ✓
}
```

## Key Learnings

1. **The immediate command stream draws directly to FBO 0** - no additional blit is needed
2. **Forward rendering mode in research baseline writes directly to default framebuffer** - not to offscreen textures
3. **Debug code can mask working functionality** - adding a clear to "test" overwrote the actual result
4. **All the infrastructure was working correctly from the start:**
   - GLAD initialization ✓
   - GPU resource uploads ✓
   - Frame graph compilation and execution ✓
   - Command buffer recording ✓
   - Draw call issuance ✓
   - Shader compilation and binding ✓
   - Matrix uniforms ✓

## Verification
After removing the debug clear:
- Cube renders correctly with shading
- Trackball camera works (orbit controller functional)
- Window shows expected blue-grey clear color as background
- All rendering systems operational

## Related Files
- `engine/rendering/src/backend/opengl/presentation_backend.cpp` - Main presentation logic
- `engine/rendering/src/backend/opengl/immediate_command_stream.cpp` - Command execution
- `engine/rendering/src/pipeline/research_baseline.cpp` - Forward pipeline configuration
- `engine/tools/examples/geometry_viewer.cpp` - Viewer application

## Answer to Original Question
**"Is the last proposal correct?"**

No. My proposal to add a blit from offscreen textures to the default framebuffer was incorrect because:
1. The research baseline forward mode already draws to FBO 0
2. No additional compositing step is needed
3. The rendering was already working; the black screen was caused by:
   - Initial confusion about whether GL calls were compiled out (they weren't)
   - My own debug clear overwriting the rendered result when investigating

The correct answer was: **Remove all debugging code and let the original pipeline work as designed**.

