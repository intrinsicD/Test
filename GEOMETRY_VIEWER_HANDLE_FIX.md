# Geometry Viewer Handle Validation Fix

**Date:** 2025-11-07  
**Issue:** geometry_viewer crashed with assertion failure in handle validation  
**Status:** ✅ FIXED

---

## The Problem

### Error Message
```
geometry_viewer: /home/alex/Documents/Test/engine/assets/include/engine/assets/validation.hpp:230: 
bool engine::assets::validate_handle(const Handle&, std::string_view) 
[with Handle = engine::assets::ResourceHandle<engine::assets::MeshHandleTag>]: 
Assertion `(result.valid) && "Resource handle validation failed"' failed.
```

### Root Cause

The geometry_viewer was trying to create `RenderGeometry` components with mesh handles that referenced non-existent files:

```cpp
// This code was creating invalid handles:
auto mesh = engine::assets::MeshHandle{std::string{"examples/cube.mesh"}};
auto material = engine::assets::MaterialHandle{std::string{"examples/default.material"}};
registry.emplace<engine::rendering::components::RenderGeometry>(
    cube,
    engine::rendering::components::RenderGeometry::from_mesh(mesh, material));
```

**The problem:**
1. Created `MeshHandle` with identifier "examples/cube.mesh"
2. File doesn't exist, so handle is **not bound**
3. Handle validation in debug mode checks if handles are bound
4. Validation fails → assertion fires → crash

### Why This Happened

The geometry_viewer was written as a placeholder/example showing the _structure_ of how to use the rendering system, but it was incomplete:
- Scene setup was done ✅
- Camera setup was done ✅
- Frame graph setup was done ✅
- **But:** No actual mesh data existed ❌
- **And:** `on_render()` was empty (no frame graph execution) ❌

---

## The Fix

### What Was Changed

**File:** `engine/tools/examples/geometry_viewer.cpp`

**Before:**
```cpp
void setup_scene()
{
    // ... entity creation ...
    
    // Add render geometry component
    auto mesh = engine::assets::MeshHandle{std::string{"examples/cube.mesh"}};
    auto material = engine::assets::MaterialHandle{std::string{"examples/default.material"}};
    registry.emplace<engine::rendering::components::RenderGeometry>(
        cube,
        engine::rendering::components::RenderGeometry::from_mesh(mesh, material));
}
```

**After:**
```cpp
void setup_scene()
{
    // ... entity creation ...
    
    // TODO: Add render geometry component once we have:
    // 1. Actual mesh data (procedural cube generation or loaded mesh)
    // 2. Application presentation backend integration (TL-310-2a)
    // For now, we just create an entity with transform to validate the scene system.
    
    // Future code (once assets exist):
    // auto mesh = engine::assets::MeshHandle{...};
    // ...
}
```

### What This Accomplishes

✅ **geometry_viewer now runs without crashing**
- Creates scene with entity and transform
- Sets up camera and input handling
- Configures frame graph
- Runs main loop successfully

❌ **Still doesn't render anything visible**
- No geometry attached to entities
- `on_render()` is still empty (no frame graph execution)
- Still needs TL-310-2a (Application presentation backend integration)

---

## Current Status

### What Works ✅

```bash
$ ./out/build/linux-gcc-debug/engine/tools/examples/geometry_viewer

=== Test Engine Geometry Viewer ===
Interactive 3D Viewer with Orbit Camera

=== Initializing Geometry Viewer ===
Creating scene...
  ✓ Scene created with 1 entity (no geometry yet)
Setting up camera...
  ✓ Camera created with orbit controller
Configuring research baseline rendering preset...
Compiling frame graph...
  ✓ Final color: ✓
  ✓ Depth buffer: ✓
=== Initialization Complete ===

Controls:
  - Left mouse drag: Rotate camera
  - Mouse scroll: Zoom in/out
  - ESC: Exit

FPS: 76212.6 (Camera: yaw=0, pitch=0.3, radius=5)

=== Shutting down ===
```

**No crash!** The viewer runs successfully.

### What's Still Missing ❌

1. **Actual mesh data** - Need to either:
   - Create procedural geometry (cube, sphere, etc.)
   - Load mesh files from disk
   - Generate geometry programmatically

2. **Frame graph execution** - `on_render()` is still empty
   - Need TL-310-2a to integrate presentation backend
   - Then can call `frame_graph_.execute(render_context())`

3. **Visible output** - Nothing renders to screen yet
   - Window appears but is empty/black
   - Need both mesh data AND frame graph execution

---

## Next Steps

### Short Term (To Make geometry_viewer Actually Render)

**Option A: Add Procedural Geometry**

Create a simple cube mesh programmatically:

```cpp
// In setup_scene():
auto cube_mesh = create_procedural_cube(); // Need to implement
auto mesh_handle = asset_system.register_mesh(cube_mesh);
registry.emplace<RenderGeometry>(
    entity,
    RenderGeometry::from_mesh(mesh_handle, {}));
```

**Requirements:**
- Implement procedural cube generation in geometry module
- Wire up to asset system for handle registration
- Estimated effort: 2-3 hours

**Option B: Use Empty Scene (Current Approach)**

Keep scene without geometry, focus on infrastructure:
- ✅ Validates scene system works
- ✅ Validates camera/input works  
- ✅ Validates frame graph compiles
- → Shifts focus to TL-310-2a (more important)

### Medium Term (Required for Rendering)

**Work on TL-310-2a** (Application ↔ PresentationBackend Integration)

This is the **critical blocker** for actual rendering:

1. Integrate presentation backend into Application
2. Provide `render_context()` accessor
3. Wire up begin_frame/end_frame/present in main loop
4. Update geometry_viewer to execute frame graph

**Estimated effort:** 4-6 hours  
**Impact:** Unblocks ALL rendering work

**Task document:** `hybrid_workflow/backlog/TL-310-2a-application-presentation-integration.md`

---

## Technical Details

### Handle Validation System

The assets module has a validation system that checks handles in debug mode:

**File:** `engine/assets/include/engine/assets/validation.hpp`

```cpp
template <typename Handle>
[[nodiscard]] bool validate_handle(const Handle& handle, std::string_view context)
{
    const auto result = validate_handle_status(handle, context);
#ifndef NDEBUG
    if (!result.valid)
    {
        assert((result.valid) && "Resource handle validation failed");
    }
#endif
    return result.valid;
}
```

**Validation checks:**
1. Is handle empty? (allowed - returns valid)
2. Is handle bound? (must be bound to data)
3. Do validators accept it? (custom validation)

**In our case:**
- Handle was NOT empty ❌
- Handle was NOT bound (no actual mesh data) ❌
- Validation failed → assertion → crash

### Why This Is Actually Good Design

The handle validation caught a real bug:
- ✅ Prevents using invalid/dangling handles
- ✅ Catches missing resources early
- ✅ Forces proper resource management
- ✅ Debug-only overhead (no cost in release)

The fix was to **not create invalid handles** in the first place, which is the correct solution.

---

## Workarounds Considered (Not Chosen)

### ❌ Disable Handle Validation

```cpp
// BAD: Would hide real bugs
#define NDEBUG  // Disable assertions
```

**Why not:** Validation is catching real issues. Disabling would mask problems.

### ❌ Create Mock/Stub Handles

```cpp
// BAD: Would require changing asset system
auto mesh = MeshHandle::create_unbound("fake");
```

**Why not:** Unbound handles shouldn't be used. System is working as designed.

### ✅ Don't Create Invalid Handles (Chosen)

```cpp
// GOOD: Don't create components until we have real data
// Just create entity + transform for now
```

**Why yes:** Clean, honest, doesn't hide problems, focuses on infrastructure first.

---

## Lessons Learned

### 1. Handle Validation is Strict (Good!)

The asset system enforces that handles are bound before use. This is correct behavior that prevents bugs.

### 2. Examples Need Real Data or None

Half-complete examples that reference non-existent resources are worse than minimal examples that work correctly.

### 3. Incremental Development

Building from the bottom up (scene → camera → frame graph → execution → rendering) is better than trying to do everything at once.

### 4. Focus on Infrastructure First

Getting the Application presentation integration (TL-310-2a) done is more valuable than having a single example with procedural geometry.

---

## Summary

### Problem
geometry_viewer crashed due to invalid mesh handle validation failure.

### Root Cause
Code tried to create `RenderGeometry` components with handles referencing non-existent mesh files.

### Solution
Removed the invalid handle creation. Scene now creates entities with transforms only.

### Result
✅ geometry_viewer runs without crashing  
✅ Validates scene/camera/frame-graph setup works  
❌ Still doesn't render (needs TL-310-2a + actual geometry)

### Next Action
**Work on TL-310-2a** to integrate presentation backend into Application framework. This is the critical path to actual rendering.

---

## Files Modified

- `engine/tools/examples/geometry_viewer.cpp` - Removed invalid handle creation

## Related Documents

- `TL-310-2a-application-presentation-integration.md` - Task for presentation backend integration
- `GEOMETRY_VIEWER_SOLUTION.md` - Analysis of what's needed to complete viewer
- `GEOMETRY_VIEWER_COMPLETION_PLAN.md` - Step-by-step completion guide

---

**Status:** Handle validation error is FIXED. geometry_viewer now runs successfully.  
**Next:** Work on TL-310-2a to enable actual rendering.

