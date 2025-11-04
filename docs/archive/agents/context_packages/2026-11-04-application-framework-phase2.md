# Context Package: Application Framework Phase 2 - Application Base Class

> Owner: Knowledge Librarian (GitHub Copilot)  
> Date: November 4, 2025

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-11-04-application-framework-phase2.md`](../task_briefs/2026-11-04-application-framework-phase2.md)
- **Backlog Entry:** N/A (continuation of Phase 1, aligns with Phase 4 GPU Enablement)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../../ROADMAP.md) Phase 4
- **Workflow Phase:** Phase 2 (Context Assembly) → Phase 3 (Execution)

## 2. Problem Summary
**Current behaviour:**
- Applications must manually create windows
- Applications must manually wire RuntimeHost
- Applications must implement their own main loop
- No consistent application lifecycle pattern
- Examples duplicate initialization/shutdown boilerplate

**Desired behaviour:**
- Applications inherit from `runtime::Application` base class
- Override lifecycle callbacks (`on_initialize`, `on_update`, `on_render`, `on_shutdown`)
- Call `run()` to execute automatic main loop
- Access subsystems (window, input, runtime, scene) via protected methods
- Consistent pattern across all applications

**Constraints / invariants:**
- Must work without RT-410 (presentation backends not ready)
- Must maintain backward compatibility with manual RuntimeHost usage
- Must provide accurate frame timing
- Must handle shutdown gracefully
- Zero performance overhead over manual loop

**Quality budgets / telemetry notes:**
- Frame timing should be accurate within 1ms
- Main loop overhead should be negligible (<0.1ms per frame)
- No memory leaks in application lifecycle

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Phase 1 Complete | [`docs/reviews/APPLICATION_FRAMEWORK_PHASE1_COMPLETE.md`](../../../reviews/APPLICATION_FRAMEWORK_PHASE1_COMPLETE.md) | Input integration done | 8 |
| Design Proposal | [`docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md`](../../../reviews/APPLICATION_FRAMEWORK_PROPOSAL.md) | Complete application framework design | 6 |
| Session Summary | [`docs/reviews/SESSION_SUMMARY_2025-11-03.md`](../../../reviews/SESSION_SUMMARY_2025-11-03.md) | Phase 1 implementation details | 8 |
| ADR-0008 | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Runtime loop architecture | 7 |
| Runtime README | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Runtime module documentation | 5 |
| Platform README | [`docs/modules/platform/README.md`](../../../modules/platform/README.md) | Platform module with input_state() | 6 |
| RuntimeHost | [`engine/runtime/include/engine/runtime/runtime_host.hpp`](../../../../engine/runtime/include/engine/runtime/) | Existing runtime orchestrator | Code |
| Window | [`engine/platform/include/engine/platform/windowing/window.hpp`](../../../../engine/platform/include/engine/platform/windowing/window.hpp) | Window with input_state() | Code |
| Geometry Viewer | [`engine/tools/examples/geometry_viewer.cpp`](../../../../engine/tools/examples/geometry_viewer.cpp) | Current manual implementation | Code |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Runtime at risk, needs application framework | ✅ | Update after Phase 2 |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Documentation precedence established | ✅ | None |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 active, application readiness goal | ✅ | Update after release |
| 4 | N/A | No backlog entry, proactive improvement | N/A | Consider backlog entry |
| 5 | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | RuntimeHost exists, needs Application wrapper | ✅ | Update with Application |
| 6 | [`docs/modules/platform/README.md`](../../../modules/platform/README.md) | Platform input complete, Window stable | ✅ | Reference in examples |
| 7 | [`docs/specs/ADR-0008`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Defines runtime loop structure | ✅ | Align Application with ADR |
| 8 | Phase 1 artifacts | Input integration establishes patterns | ✅ | Build on Phase 1 work |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
**Canonical command block copied:**
```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

**Additional presets / datasets:**
- Build runtime module: `cmake --build cmake-build-debug --target engine_runtime`
- Build geometry_viewer: `cmake --build cmake-build-debug --target geometry_viewer`
- Run runtime tests: `ctest --preset linux-gcc-debug -R runtime`

**Benchmark targets & expected deltas:**
- Frame timing accuracy: ±1ms
- Main loop overhead: <0.1ms per frame
- No memory growth over 1000 frames

**Tooling updates required:**
- None for this phase

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should Application own RuntimeHost or share? | Specialist Engineer | Nov 4 | Application owns, creates internally |
| Should Application own Scene or access via RuntimeHost? | Specialist Engineer | Nov 4 | Provide convenience accessor to RuntimeHost's scene |
| Frame timing: fixed timestep or variable? | Specialist Engineer | Nov 4 | Variable with accurate delta, fixed timestep optional |
| VSync control needed in ApplicationConfig? | Specialist Engineer | Nov 4 | Defer to future phase |
| Should quit() be virtual for override? | Specialist Engineer | Nov 4 | Non-virtual, final behavior |

## 7. Implementation Plan

### Phase 1: Application Base Class Definition
1. Create `engine/runtime/include/engine/runtime/application.hpp`
2. Define `ApplicationConfig` struct with window configuration passthrough
3. Define `Application` base class with:
   - Constructor taking ApplicationConfig
   - Virtual destructor
   - Delete copy/move constructors
   - Pure virtual lifecycle callbacks
   - Public `int run()` method
   - Public `void quit(int exit_code = 0)` method
   - Protected subsystem accessors

### Phase 2: Application Implementation
1. Create `engine/runtime/src/application.cpp`
2. Implement constructor (store config, don't create subsystems yet)
3. Implement `run()`:
   - Initialize subsystems (window, runtime)
   - Call `on_initialize()`
   - Run main loop
   - Call `on_shutdown()`
   - Clean up subsystems
   - Return exit code
4. Implement main loop:
   - Frame timing with high_resolution_clock
   - Window event pumping
   - Call `on_update(delta_time)`
   - Call `on_render()`
   - Check quit flag and window close
   - Optional: frame rate limiting
5. Implement subsystem accessors

### Phase 3: CMakeLists Integration
1. Add application.cpp to runtime module sources
2. Ensure proper linking and exports
3. Add API export macros

### Phase 4: Geometry Viewer Refactoring
1. Create GeometryViewerApp class inheriting from Application
2. Move initialization logic to on_initialize()
3. Move per-frame logic to on_update()
4. Move rendering logic to on_render()
5. Simplify main() to just create and run app
6. Remove manual window creation, main loop, etc.

### Phase 5: Documentation Updates
1. Update runtime module README with Application class
2. Add lifecycle callback documentation
3. Add usage examples (minimal and complete)
4. Update geometry viewer documentation
5. Create migration guide

## 8. Key Code Locations

**Files to Create:**
- `engine/runtime/include/engine/runtime/application.hpp` - Application class definition
- `engine/runtime/src/application.cpp` - Application implementation

**Files to Modify:**
- `engine/runtime/CMakeLists.txt` - Add application.cpp to sources
- `engine/runtime/include/engine/runtime/api.hpp` - Ensure exports if needed
- `engine/tools/examples/geometry_viewer.cpp` - Refactor to use Application
- `docs/modules/runtime/README.md` - Add Application documentation

**Existing Code to Reference:**
- `engine/runtime/include/engine/runtime/runtime_host.hpp` - RuntimeHost API
- `engine/platform/include/engine/platform/windowing/window.hpp` - Window API with input_state()
- `engine/scene/include/engine/scene/scene.hpp` - Scene management
- Current geometry_viewer.cpp implementation

## 9. Testing Strategy

**Unit Tests:**
- Test Application construction with various configs
- Test lifecycle callback invocation order
- Test quit() triggers exit
- Test frame timing accuracy
- Test subsystem accessor validity

**Integration Tests:**
- Geometry viewer runs successfully
- Application initializes all subsystems
- Main loop executes at stable framerate
- Clean shutdown on quit() or window close

**Manual Verification:**
- Run geometry_viewer and verify all controls work
- Verify frame timing is stable
- Verify clean exit on ESC
- Check for memory leaks with valgrind/sanitizers

## 10. Detailed API Design

### ApplicationConfig
```cpp
namespace engine::runtime
{
    struct ApplicationConfig
    {
        /// Window configuration (passed to platform::create_window)
        platform::WindowConfig window{
            .title = "Engine Application",
            .width = 1280,
            .height = 720,
            .visible = true,
            .resizable = true
        };
        
        /// Preferred window backend
        platform::WindowBackend window_backend{platform::WindowBackend::Auto};
        
        /// Target framerate (0 = unlimited)
        double target_fps{0.0};
        
        /// Enable runtime diagnostics
        bool enable_diagnostics{false};
    };
}
```

### Application Class
```cpp
namespace engine::runtime
{
    class ENGINE_RUNTIME_API Application
    {
    public:
        explicit Application(const ApplicationConfig& config = {});
        virtual ~Application() noexcept;
        
        // Disable copy/move
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;
        
        /// Run the application main loop. Returns exit code.
        int run();
        
        /// Request application shutdown with optional exit code
        void quit(int exit_code = 0);
        
    protected:
        // Lifecycle callbacks (override in derived class)
        
        /// Called once before main loop starts
        virtual void on_initialize() {}
        
        /// Called once per frame before rendering
        /// @param delta_time Time since last frame in seconds
        virtual void on_update(double delta_time) {}
        
        /// Called once per frame for rendering (optional)
        virtual void on_render() {}
        
        /// Called once after main loop exits
        virtual void on_shutdown() {}
        
        // Subsystem accessors
        
        /// Access the platform window
        [[nodiscard]] platform::Window& window() noexcept;
        [[nodiscard]] const platform::Window& window() const noexcept;
        
        /// Access input state (convenience wrapper)
        [[nodiscard]] platform::input::InputState& input() noexcept;
        [[nodiscard]] const platform::input::InputState& input() const noexcept;
        
        /// Access runtime host
        [[nodiscard]] RuntimeHost& runtime() noexcept;
        [[nodiscard]] const RuntimeHost& runtime() const noexcept;
        
        /// Access scene (convenience, returns runtime's scene)
        [[nodiscard]] scene::Scene& scene() noexcept;
        [[nodiscard]] const scene::Scene& scene() const noexcept;
        
        /// Get elapsed time since application start
        [[nodiscard]] double elapsed_time() const noexcept;
        
        /// Get current frame number
        [[nodiscard]] std::uint64_t frame_count() const noexcept;
        
    private:
        void initialize_subsystems();
        void shutdown_subsystems();
        void run_main_loop();
        
        ApplicationConfig config_;
        std::shared_ptr<platform::Window> window_;
        std::unique_ptr<RuntimeHost> runtime_;
        
        bool running_{false};
        int exit_code_{0};
        double elapsed_time_{0.0};
        std::uint64_t frame_count_{0};
    };
}
```

### Usage Example (Geometry Viewer)
```cpp
class GeometryViewerApp : public engine::runtime::Application
{
public:
    GeometryViewerApp() : Application({
        .window = {
            .title = "Geometry Viewer - Research Baseline",
            .width = 1280,
            .height = 720,
            .visible = true,
            .resizable = true
        },
        .window_backend = engine::platform::WindowBackend::GLFW
    }) {}
    
protected:
    void on_initialize() override
    {
        // Setup scene
        camera_entity_ = scene().create_entity();
        // ... camera setup
    }
    
    void on_update(double dt) override
    {
        auto& inp = input();
        
        // Handle input
        if (inp.is_mouse_button_down(input::MouseButton::Left)) {
            auto delta = inp.cursor_delta();
            camera_yaw_ += delta.x * ROTATE_SPEED;
            camera_pitch_ -= delta.y * ROTATE_SPEED;
        }
        
        if (inp.was_key_pressed(input::Key::Escape)) {
            quit();
        }
        
        // Update camera
        update_camera();
    }
    
    void on_render() override
    {
        // Rendering will be wired when RT-410 completes
    }
    
    void on_shutdown() override
    {
        // Clean up application-specific resources
    }
    
private:
    entt::entity camera_entity_{entt::null};
    float camera_yaw_{0.0f};
    float camera_pitch_{0.3f};
    float camera_radius_{5.0f};
};

int main()
{
    GeometryViewerApp app;
    return app.run();
}
```

## 11. Migration Path

### Before (Manual Loop - Current)
```cpp
int main() {
    // Create window manually
    auto window = platform::create_window(config);
    
    // Create runtime manually
    RuntimeHost runtime;
    
    // Setup scene manually
    // ...
    
    // Manual main loop
    while (!window->close_requested()) {
        window->pump_events();
        
        auto& input = window->input_state();
        // Handle input...
        
        // Update logic...
        
        // Render...
    }
    
    return 0;
}
```

### After (Application Class)
```cpp
class MyApp : public Application {
protected:
    void on_initialize() override { /* setup */ }
    void on_update(double dt) override { /* update */ }
    void on_render() override { /* render */ }
};

int main() {
    MyApp app;
    return app.run();
}
```

## 12. Attachments

**Key Design Decisions:**
1. **Application owns subsystems** - Simplifies lifetime management
2. **Virtual callbacks, not event system** - Simpler for most use cases
3. **Protected accessors** - Encourages composition, prevents external access
4. **Variable timestep** - More flexible, fixed timestep can be added later
5. **Quit flag, not exception** - Cleaner shutdown path

**Blocked Features (for later phases):**
- Presentation backend integration (needs RT-410)
- VSync control (needs presentation backend)
- Multi-window support (future phase)
- ImGui integration (future phase)
- Fullscreen toggle (future phase)

**Success Metrics:**
- Geometry viewer LOC reduction: ~50% expected
- Main loop overhead: <0.1ms per frame
- Frame timing accuracy: ±1ms
- Zero memory leaks
- All tests passing

