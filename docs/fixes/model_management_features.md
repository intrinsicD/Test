# Geometry Viewer - Model Management & Toggle Features

**Date:** November 16, 2025  
**Feature:** Toggle cube, drag-drop models, delete models  
**Status:** ✅ IMPLEMENTED

## New Features

### 1. Toggle Default Cube (T Key)

Press **'T'** to show/hide the default procedural cube.

**Implementation:**
- Tracks cube visibility state with `cube_visible_` boolean
- Removes/adds RenderGeometry component to hide/show
- Entity persists but becomes invisible when hidden
- Independent of loaded models

**Usage:**
```
Press 'T' → Cube disappears
Press 'T' again → Cube reappears
```

### 2. Drag & Drop Model Loading

Drag and drop mesh or point cloud files into the window to load them.

**Supported Formats:**
- **Meshes:** .obj, .off, .ply, .stl
- **Point Clouds:** .ply, .pcd, .xyz

**Features:**
- Automatically detects file format
- Focuses camera on loaded model bounds
- Tracks loading order for deletion
- Multiple models can be loaded simultaneously
- Each model gets unique entity and tracking ID

**Implementation:**
- Uses existing file drop event handling
- Tracks models in `loaded_models_order_` vector
- Excludes procedural cube from deletion tracking

### 3. Delete Last Loaded Model (Backspace Key)

Press **Backspace** to delete the most recently loaded model (LIFO stack order).

**Behavior:**
- Deletes models in reverse creation order (last in, first out)
- Only affects drag-dropped models, NOT the default cube
- Destroys entity completely (not just hide)
- Refocuses camera on cube if all models deleted
- Shows message if no models to delete

**Implementation:**
```cpp
void delete_last_model()
{
    if (loaded_models_order_.empty())
    {
        ENGINE_INFO("No models to delete");
        return;
    }

    // Get the last loaded model (LIFO)
    const std::string model_id = loaded_models_order_.back();
    loaded_models_order_.pop_back();

    // Destroy entity and remove from tracking
    auto& registry = scene().registry();
    if (const auto it = render_entities_.find(model_id); it != render_entities_.end())
    {
        if (registry.valid(it->second))
        {
            registry.destroy(it->second);
        }
        render_entities_.erase(it);
    }

    // Refocus on cube if no models left
    if (loaded_models_order_.empty() && cube_visible_)
    {
        if (const auto* mesh = mesh_storage_->find(kProceduralCubeId))
        {
            focus_camera_on_bounds(mesh->bounds);
        }
    }
}
```

## Usage Examples

### Example 1: Basic Workflow

```
1. Start geometry_viewer
   → Default cube is visible

2. Press 'T'
   → Cube hides

3. Drag 'bunny.obj' into window
   → Bunny loads and camera focuses on it

4. Drag 'teapot.stl' into window
   → Teapot loads and camera focuses on it

5. Press Backspace
   → Teapot deleted (last loaded)

6. Press Backspace
   → Bunny deleted

7. Press 'T'
   → Cube reappears
```

### Example 2: Multiple Models

```
1. Drag 'model1.obj' → Loads
2. Drag 'model2.off' → Loads
3. Drag 'model3.ply' → Loads

Press Backspace → Deletes model3.ply
Press Backspace → Deletes model2.off
Press Backspace → Deletes model1.obj
```

### Example 3: Cube Independent of Models

```
1. Hide cube with 'T'
2. Load several models
3. Delete all models with Backspace
4. Show cube with 'T' → Cube still exists!
```

## Key Bindings Summary

| Key | Action |
|-----|--------|
| **T** | Toggle default cube visibility |
| **Backspace** | Delete last loaded model |
| **Left Mouse Drag** | Trackball rotation |
| **Scroll Wheel** | Zoom in/out |
| **ESC** | Quit application |

## Implementation Details

### Data Structures

```cpp
// Track render entities
std::unordered_map<std::string, entt::entity> render_entities_{};

// Track loaded models in creation order (excludes cube)
std::vector<std::string> loaded_models_order_{};

// Cube visibility toggle state
bool cube_visible_{true};
```

### Model Loading Flow

```
1. File dropped into window
   ↓
2. detect_geometry_file() determines format
   ↓
3. load_mesh_asset() or load_point_cloud_asset()
   ↓
4. Asset loaded from cache
   ↓
5. focus_camera_on_bounds()
   ↓
6. attach_render_geometry()
   ↓
7. Add to loaded_models_order_ vector
```

### Deletion Flow

```
1. User presses Backspace
   ↓
2. Pop last model ID from loaded_models_order_
   ↓
3. Find entity in render_entities_
   ↓
4. registry.destroy(entity)
   ↓
5. Remove from render_entities_ map
   ↓
6. If no models left, refocus on cube
```

### Cube Toggle Flow

```
1. User presses 'T'
   ↓
2. Toggle cube_visible_ boolean
   ↓
3. If showing: attach_render_geometry() with cube
   ↓
4. If hiding: registry.remove<RenderGeometry>()
```

## Design Decisions

### Why Backspace Instead of Delete?

- **Cross-platform compatibility:** Delete key behavior varies
- **Keyboard accessibility:** Backspace is standard on all keyboards
- **Semantic meaning:** "Undo" last action (loading a model)

### Why LIFO (Stack) Order?

- **Intuitive:** Most recent action is undone first
- **Visual feedback:** The model you just loaded is the one deleted
- **Common pattern:** Matches undo/redo behavior in most applications

### Why Separate Cube from Models?

- **Design intent:** Cube is a reference object, not user content
- **Persistence:** Cube should survive model operations
- **Toggle vs Delete:** Hiding is temporary, deletion is permanent

### Why Entity Destruction vs Component Removal?

- **Cube:** Component removal (keeps entity, just invisible)
  - Allows quick toggle without recreation
  - Preserves transform and hierarchy
  
- **Models:** Entity destruction (complete removal)
  - Frees all resources
  - Removes from scene graph
  - Clean slate for new loads

## Future Enhancements

Possible improvements:

1. **Named deletion:** Delete by clicking on model
2. **Delete all:** Clear all loaded models at once
3. **Model list UI:** Show loaded models with checkboxes
4. **Undo/redo:** Full undo stack for all operations
5. **Save/load session:** Persist loaded models and cube state
6. **Model inspector:** Show properties of selected model
7. **Ctrl+Z:** Standard undo keybinding in addition to Backspace

## Technical Notes

### Memory Management

- Models are loaded into asset caches
- Caches maintain reference counting
- Entity destruction removes scene references
- Asset cache GC cleans up unused assets

### Thread Safety

- All operations on main thread
- Asset loading happens on main thread
- File drop events queued and processed serially

### Error Handling

- File format detection errors logged and skip loading
- Missing files show error message
- Invalid formats gracefully ignored
- Cache load failures caught and reported

## Files Modified

1. `/engine/tools/examples/geometry_viewer.cpp`
   - Added `loaded_models_order_` vector
   - Added `cube_visible_` boolean
   - Added `toggle_cube()` method
   - Added `delete_last_model()` method
   - Added 'T' and Backspace key handlers
   - Updated model loading to track order

## Testing

To verify the implementation:

1. **Cube Toggle:**
   ```
   Press 'T' multiple times
   → Cube should appear/disappear
   ```

2. **Model Loading:**
   ```
   Drag 3 different mesh files
   → All should load and display
   ```

3. **Model Deletion:**
   ```
   Press Backspace 3 times
   → Models deleted in reverse order
   ```

4. **Empty State:**
   ```
   Delete all models
   Press Backspace again
   → Message: "No models to delete"
   ```

5. **Cube Independent:**
   ```
   Hide cube, load models, delete models, show cube
   → Cube unaffected by model operations
   ```

---

**Status:** ✅ FULLY IMPLEMENTED  
**User Experience:** Intuitive model management  
**Code Quality:** Clean, well-documented implementation

