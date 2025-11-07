# Geometry Viewer Completion Plan

**Date:** 2025-11-07  
**Current Status:** geometry_viewer runs but has stubbed rendering (on_render is empty)  
**Goal:** Complete the rendering implementation to actually display geometry

---

## Current State Analysis

### ✅ What's Working

1. **Application lifecycle** - Inherits from `runtime::Application` correctly
2. **Window creation** - GLFW backend operational
3. **Scene setup** - Creates entities with transform and render components
4. **Camera system** - Orbit camera with interactive controls
5. **Input handling** - Mouse drag, scroll, ESC key
6. **Frame graph setup** - Research baseline configured and compiled
7. **FPS counter** - Displaying 254k+ FPS

### ❌ What's Missing (Line 97-101)

```cpp
void on_render() override
{
    // Note: In full implementation with RT-410, we would:
    // - Execute frame graph with scene
    // - Render all geometry through presentation backend
    // - Present the final image
}
```

**The rendering loop is stubbed!** It sets up everything but doesn't actually render.

---

## The Problem

Looking at the code:
1. ✅ `setup_frame_graph()` creates and compiles a `FrameGraph`
2. ❌ But it's a **local variable** - gets destroyed after setup
3. ❌ `on_render()` is empty - no frame graph execution
4. ❌ No connection to presentation backend

**Result:** Window appears, input works, but nothing renders because `FrameGraph::execute()` is never called.

---

## Task to Complete: Implement Frame Graph Execution

### What You Need To Do

**Goal:** Store the frame graph and execute it every frame with proper scene/camera data.

### Step 1: Store Frame Graph as Member Variable

Add to `GeometryViewerApp` class:

```cpp
private:
    // ...existing members...
    
    // Frame graph and rendering
    engine::rendering::FrameGraph frame_graph_;
    engine::rendering::ResearchBaselineResources baseline_resources_;
```

### Step 2: Update `setup_frame_graph()` to Store Graph

```cpp
void setup_frame_graph()
{
    std::cout << "Configuring research baseline rendering preset...\n";

    // Configure frame graph (don't use local variable!)
    engine::rendering::ResearchBaselineOptions options{};
    options.shading_mode = engine::rendering::ResearchShadingMode::Forward;
    options.width = WINDOW_WIDTH;
    options.height = WINDOW_HEIGHT;
    options.enable_normals_overlay = false;

    baseline_resources_ = engine::rendering::configure_research_baseline(
        frame_graph_, options);

    std::cout << "Compiling frame graph...\n";
    frame_graph_.compile();

    std::cout << "  ✓ Final color: " 
              << (baseline_resources_.lighting_output.valid() ? "✓" : "✗") << "\n";
    std::cout << "  ✓ Depth buffer: " 
              << (baseline_resources_.depth.valid() ? "✓" : "✗") << "\n";
}
```

### Step 3: Implement `on_render()` to Execute Frame Graph

```cpp
void on_render() override
{
    // Get rendering context (provided by Application base class)
    auto* render_context = get_render_context();
    if (!render_context)
    {
        return; // Headless mode or context not ready
    }

    // Execute frame graph with scene
    frame_graph_.execute(*render_context);
    
    // Present to screen (handled by runtime presentation backend)
}
```

### Step 4: Verify Get Render Context API

Check if `Application` provides render context access:

```bash
grep -r "get_render_context\|RenderExecutionContext" engine/runtime/include/
```

If not available, you may need to:
1. Access it through runtime subsystem
2. Or create it from presentation surface
3. Check `docs/modules/runtime/README.md` for the API

---

## Alternative: Check Existing Working Examples

### Search for Complete Rendering Examples

```bash
# Find other examples that execute frame graphs
grep -r "frame_graph.*execute" engine/ --include="*.cpp"

# Find how presentation backends are used
grep -r "present\|swap" engine/rendering/src/backend/ --include="*.cpp"
```

### Look at Test/Sample Code

Check these locations:
- `engine/rendering/tests/` - Unit tests might show execution pattern
- `engine/rendering/samples/` - If any samples exist
- `engine/runtime/tests/` - Integration test examples
- `docs/examples/` - Documentation examples

---

## Recommended Approach: Follow the Breadcrumbs

### 1. Check Runtime Integration (HIGHEST PRIORITY)

Since RT-410 is complete, there should be presentation backend integration. Look at:

**File to investigate:**
```
engine/runtime/src/application.cpp
```

**What to find:**
- Does `Application::run()` already call a render method?
- Is there a presentation backend instance?
- How should `on_render()` connect to it?

### 2. Check Rendering Module API

**File to investigate:**
```
engine/rendering/include/engine/rendering/api.hpp
```

**What to find:**
- Helper functions for execution
- Presentation backend creation
- Render context management

### 3. Look at Research Baseline Implementation

**File to investigate:**
```
engine/rendering/src/pipeline/research_baseline.cpp
```

**What to find:**
- How passes are wired up
- What execution expects
- Scene/camera data flow

---

## Concrete Next Steps (DO THIS NOW)

### Step A: Understand Application Framework

```bash
cd /home/alex/Documents/Test

# Read the application base class
cat engine/runtime/include/engine/runtime/application.hpp | less

# Check the implementation
cat engine/runtime/src/application.cpp | grep -A20 "void Application::run"
```

**Questions to answer:**
1. Does `Application` already have rendering infrastructure?
2. What's the connection between `on_render()` and presentation?
3. Is there a `RenderExecutionContext` already created?

### Step B: Find Working Integration Tests

```bash
# Find tests that execute frame graphs with scenes
grep -r "FrameGraph.*execute\|frame_graph.execute" engine/ \
  --include="*.cpp" -B5 -A5

# Find presentation backend usage
grep -r "PresentationBackend\|presentation_backend" engine/runtime/ \
  --include="*.cpp" -B3 -A3
```

### Step C: Check Documentation

```bash
# Read the runtime module docs
cat docs/modules/runtime/README.md | grep -A50 "Application"

# Read rendering module docs  
cat docs/modules/rendering/README.md | grep -A50 "frame.graph"
```

---

## Expected Implementation Pattern

Based on RT-410 completion and the architecture, the pattern should be:

```cpp
class GeometryViewerApp : public engine::runtime::Application 
{
protected:
    void on_initialize() override 
    {
        setup_scene();
        setup_camera();
        setup_frame_graph();
        setup_presentation(); // <-- Might need this
    }

    void on_render() override 
    {
        // Get or create render execution context
        auto& context = get_or_create_render_context();
        
        // Update scene systems (transforms, etc.)
        scene().update();
        
        // Execute frame graph
        frame_graph_.execute(context);
        
        // Presentation backend handles swapping
    }

private:
    engine::rendering::FrameGraph frame_graph_;
    engine::rendering::ResearchBaselineResources resources_;
    // Possibly: engine::rendering::PresentationBackend* presentation_;
};
```

---

## Task Assignment

**For completing geometry_viewer, work on:**

### Primary Task: None Exists - CREATE IT!

There's no specific task for geometry_viewer completion. It's an **example** in progress.

**Options:**

1. **Just finish it directly** - It's an example, not a tracked deliverable
2. **Create a task** - If you want to track it formally:
   - `TL-315: Complete geometry_viewer rendering example`
   - Size: S (small)
   - Owner: You
   - Blocked by: Nothing (RT-410 done, everything ready)

### Related Tasks You Should Work On Instead

Since geometry_viewer is just a reference example, consider these higher-priority tasks:

#### **Option 1: TL-310 (Recommended Priority)**

**Task:** Editor Foundations & Tooling Enablement  
**Status:** in_progress  
**Remaining work:**
- [ ] Restore unit tests for configuration loader
- [ ] Add editor smoke scenario
- [ ] Documentation refresh
- [ ] PM-510 demo coordination

**Why this matters:** Unlocks TL-311-314 (editor panels) and provides infrastructure for more sophisticated tools than geometry_viewer.

**Impact:** High - enables entire editor ecosystem

#### **Option 2: Complete geometry_viewer First (Quick Win)**

**Task:** Finish geometry_viewer example (untracked)  
**Effort:** 1-2 hours  
**Impact:** Medium - provides complete reference for others

**Steps:**
1. Investigate Application framework (30 min)
2. Store frame graph as member (10 min)
3. Implement on_render() with execution (30 min)
4. Test and verify rendering works (30 min)
5. Document the pattern (20 min)

**Benefit:** Once working, you have a complete reference implementation to guide TL-310+ work.

---

## My Recommendation

### 🎯 **Finish geometry_viewer FIRST, then tackle TL-310**

**Rationale:**

1. **Quick win** - 1-2 hours to complete vs weeks for TL-310
2. **Learn by doing** - Understanding the full rendering pipeline helps with editor work
3. **Reference implementation** - TL-311+ will need similar patterns
4. **Unblocks understanding** - You asked about geometry_viewer specifically, finish what you started!
5. **Confidence builder** - See actual geometry render before tackling editor complexity

### Immediate Actions (Next 2 Hours)

```bash
# 1. Investigate Application class (15 min)
cat engine/runtime/include/engine/runtime/application.hpp
cat engine/runtime/src/application.cpp

# 2. Find render context pattern (15 min)
grep -r "RenderExecutionContext" engine/runtime/ --include="*.hpp" --include="*.cpp"

# 3. Check for existing complete examples (15 min)
find engine/ -name "*.cpp" -type f -exec grep -l "frame_graph.execute" {} \;

# 4. Implement the fix (45 min)
# Edit geometry_viewer.cpp based on findings

# 5. Build and test (20 min)
cmake --build out/build/linux-gcc-debug --target geometry_viewer -j$(nproc)
./out/build/linux-gcc-debug/engine/tools/examples/geometry_viewer

# 6. Document the pattern (10 min)
# Update QUICK_START_RENDERING.md with complete example
```

---

## Success Criteria

### geometry_viewer is "done" when:

- [ ] Renders actual 3D geometry (cube visible on screen)
- [ ] Camera controls work and affect rendered view
- [ ] No stub comments in code
- [ ] Frame graph executes every frame
- [ ] Presentation backend displays output
- [ ] No crashes or warnings
- [ ] FPS counter shows reasonable values (60-144 FPS in release)
- [ ] Code is documented as reference implementation

---

## Final Answer to Your Question

### "Which task should I work on to progress to finish the geometry viewer?"

**Answer:** There is **no formal task** for geometry_viewer - it's an example in the codebase.

**What you should do:**

1. **Quick path (1-2 hours):** Finish geometry_viewer directly
   - Investigate `Application` rendering infrastructure
   - Store `FrameGraph` as member variable  
   - Implement `on_render()` to call `frame_graph_.execute()`
   - Test and verify actual rendering works

2. **After geometry_viewer works, move to TL-310**
   - Complete remaining checklist items
   - This unlocks TL-311-314 (editor panels)
   - Higher business value than a single example

**Bottom line:** Finish the viewer first (quick win), then focus on tracked tasks (TL-310). The viewer completion will teach you patterns needed for editor work anyway.

---

## Next Command to Run

```bash
# Start your investigation
cd /home/alex/Documents/Test
cat engine/runtime/include/engine/runtime/application.hpp | grep -A100 "class Application"
```

This will show you the Application API and likely reveal the rendering hook you need!

