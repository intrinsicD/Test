# Context Package: Application Framework Implementation

> Owner: Knowledge Librarian (GitHub Copilot)  
> Date: November 3, 2025

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-11-03-application-framework-implementation.md`](../task_briefs/2026-11-03-application-framework-implementation.md)
- **Backlog Entry:** N/A (new capability, aligns with Phase 4 GPU Enablement)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md) Phase 4
- **Workflow Phase:** Phase 2 (Context Assembly) → Phase 3 (Execution)

## 2. Problem Summary
**Current behaviour:**
- Applications must use raw GLFW callbacks for input handling
- Window interface does not expose InputState
- Examples like `geometry_viewer.cpp` create duplicate window management (GLFW + Engine)
- Manual callback wiring creates boilerplate and inconsistent patterns

**Desired behaviour:**
- Window interface exposes `input_state()` accessor
- GLFW backend automatically updates InputState during event processing
- Applications can query input state cleanly: `window->input_state().is_key_pressed(Key::W)`
- Unified pattern across all applications

**Constraints / invariants:**
- Must not break existing event queue functionality
- Must maintain backend abstraction (GLFW, Mock)
- InputState must be frame-coherent (updated during pump_events)
- Zero performance overhead in input processing

**Quality budgets / telemetry notes:**
- No telemetry changes required for this phase
- Input processing is already fast, must remain so

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/platform/README.md`](../../docs/modules/platform/README.md) | Documents window backends, partial input docs | 5 |
| Design Proposal | [`docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md`](../../docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md) | Complete application framework design | 6 |
| Gap Analysis | [`docs/reviews/MISSING_COMPONENTS_SUMMARY.md`](../../docs/reviews/MISSING_COMPONENTS_SUMMARY.md) | Identifies Window::input_state() as missing | 6 |
| Architecture Analysis | [`docs/reviews/GEOMETRY_VIEWER_ARCHITECTURE_ANALYSIS.md`](../../docs/reviews/GEOMETRY_VIEWER_ARCHITECTURE_ANALYSIS.md) | Explains why geometry_viewer uses GLFW directly | 6 |
| Window Interface | [`engine/platform/include/engine/platform/windowing/window.hpp`](../../engine/platform/include/engine/platform/windowing/window.hpp) | Current Window interface (no input_state) | Code |
| InputState | [`engine/platform/include/engine/platform/input/`](../../engine/platform/include/engine/platform/input/) | Existing InputState implementation | Code |
| GLFW Backend | [`engine/platform/src/windowing/glfw_window.cpp`](../../engine/platform/src/windowing/) | GLFW implementation to extend | Code |
| Geometry Viewer | [`engine/tools/examples/geometry_viewer.cpp`](../../engine/tools/examples/geometry_viewer.cpp) | Example using raw GLFW callbacks | Code |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Platform module stable, 60% complete per gap analysis | ✅ | Update after implementation |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Documentation precedence: AGENTS.md → NAVIGATION → modules | ✅ | None |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 active, RT-410 in progress, aligns with this work | ✅ | Update status after Phase 5 |
| 4 | N/A | No specific backlog entry, proactive improvement | N/A | Consider creating backlog entry |
| 5 | [`docs/modules/platform/README.md`](../../docs/modules/platform/README.md) | Input handling examples exist but not integrated with Window | ✅ | Update with new API |
| 6 | [`docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md`](../../docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md) | Proposes Window::input_state() as Phase 1 | ✅ | Mark Phase 1 complete |
| 7 | [`docs/reviews/MISSING_COMPONENTS_SUMMARY.md`](../../docs/reviews/MISSING_COMPONENTS_SUMMARY.md) | Platform module 60% complete, missing input integration | ✅ | Update percentages |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
**Canonical command block copied:**
```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

**Additional presets / datasets:**
- Build geometry_viewer target specifically: `cmake --build cmake-build-debug --target geometry_viewer`
- Run platform tests: `ctest --preset linux-gcc-debug -R platform`

**Benchmark targets & expected deltas:**
- No performance change expected in input processing
- Input latency should remain sub-millisecond

**Tooling updates required:**
- None for this phase

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should InputState be owned by Window or shared? | Specialist Engineer | Nov 3 | Window owns, returns reference |
| Mock backend needs InputState too? | Specialist Engineer | Nov 3 | Yes, for testing consistency |
| Key mapping enums already exist? | Knowledge Librarian | Nov 3 | Yes, in input module |
| GLFW callback mapping complete? | Specialist Engineer | Nov 3 | Check existing input code |

## 7. Implementation Plan

### Phase 1: Window Interface Extension
1. Add `input_state()` methods to Window interface
2. Update Window base class documentation
3. Ensure const and non-const overloads

### Phase 2: GLFW Backend Integration
1. Add InputState member to GlfwWindow class
2. Set up GLFW callbacks in constructor:
   - `glfwSetKeyCallback` → update InputState
   - `glfwSetMouseButtonCallback` → update InputState
   - `glfwSetCursorPosCallback` → update InputState
   - `glfwSetScrollCallback` → update InputState
3. Call `input_state_.begin_frame()` at start of `pump_events()`
4. Implement key/button mapping from GLFW to engine enums

### Phase 3: Mock Backend Integration
1. Add InputState member to MockWindow
2. Allow test code to inject input events
3. Update InputState during pump_events()

### Phase 4: Geometry Viewer Refactoring
1. Remove raw GLFW callbacks
2. Remove duplicate window management
3. Use `window->input_state()` for input queries
4. Simplify main loop

### Phase 5: Documentation Updates
1. Update platform module README with examples
2. Add migration guide for GLFW callbacks → input_state
3. Update geometry viewer documentation

## 8. Key Code Locations

**Files to Modify:**
- `engine/platform/include/engine/platform/windowing/window.hpp` - Add input_state() interface
- `engine/platform/src/windowing/glfw_window.cpp` - Implement GLFW integration
- `engine/platform/src/windowing/glfw_window.hpp` - Add InputState member
- `engine/platform/src/windowing/mock_window.cpp` - Implement Mock integration
- `engine/platform/src/windowing/mock_window.hpp` - Add InputState member
- `engine/tools/examples/geometry_viewer.cpp` - Refactor to use new API
- `docs/modules/platform/README.md` - Update documentation

**Existing Code to Reference:**
- `engine/platform/include/engine/platform/input/input_state.hpp` - InputState API
- `engine/platform/include/engine/platform/input/keys.hpp` - Key enum definitions
- Current GLFW callback usage in geometry_viewer.cpp

## 9. Testing Strategy

**Unit Tests:**
- Test Window::input_state() returns valid reference
- Test InputState updates during pump_events()
- Test key mapping from GLFW codes to engine::platform::Key
- Test mouse button mapping
- Test cursor position tracking
- Test scroll delta accumulation

**Integration Tests:**
- Geometry viewer runs without crashes
- Input responses match previous GLFW behavior
- Mock window input injection works

**Manual Verification:**
- Run geometry_viewer and test all controls
- Verify camera rotation with mouse drag
- Verify zoom with scroll
- Verify ESC key exits

## 10. Attachments

**Code Snippets from Design Proposal:**

```cpp
// Proposed Window interface addition
class Window {
public:
    // NEW: Expose unified input state
    [[nodiscard]] virtual input::InputState& input_state() = 0;
    [[nodiscard]] virtual const input::InputState& input_state() const = 0;
};

// Proposed GLFW integration
class GlfwWindow : public HeadlessWindow {
private:
    input::InputState input_state_;
    
    static void key_callback(GLFWwindow* window, int key, int scancode, 
                            int action, int mods) {
        auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        auto mapped_key = map_glfw_key(key);
        bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        self->input_state_.apply_key_event(mapped_key, pressed);
    }
    
    void pump_events() override {
        input_state_.begin_frame();
        glfwPollEvents();
        // Callbacks update input_state_ automatically
    }
};

// Proposed usage in applications
auto& input = window->input_state();
if (input.is_key_down(Key::W)) {
    camera.move_forward(dt);
}
```

**Migration Path:**

Before (geometry_viewer.cpp current):
```cpp
// Raw GLFW callbacks
glfwSetMouseButtonCallback(window, mouse_button_callback);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        state->mouse_dragging = true;
    }
}
```

After (proposed):
```cpp
// Clean input queries
auto& input = state.window->input_state();
if (input.was_button_pressed(MouseButton::Left)) {
    state.mouse_dragging = true;
}
```

> **Checklist:** 
> - ✅ All design documents reviewed
> - ✅ Context ladder complete
> - ✅ Implementation plan detailed
> - ✅ Testing strategy defined
> - ✅ Documentation updates planned
> - ✅ Code locations identified
> - ⏳ Ready for Phase 3 (Execution)

