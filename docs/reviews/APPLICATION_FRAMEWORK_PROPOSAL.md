# Engine Application Framework - Design Proposal

**Date**: November 3, 2025  
**Status**: Proposal / Design Discussion  
**Related Tasks**: RT-410, T-0119, T-0120  

## Motivation

Currently, there is **no high-level application framework** in the engine. Developers must manually:

1. Create and manage windows
2. Wire input handling via raw callbacks
3. Implement their own main loop
4. Manually integrate runtime, rendering, and presentation
5. Handle shutdown and resource cleanup

This creates **significant friction** and leads to **inconsistent patterns** across examples and applications.

## Proposed Architecture

### Component Hierarchy

```
engine::runtime::Application (new)
├── engine::platform::Window (existing)
│   └── engine::platform::input::InputState (existing, needs integration)
├── engine::runtime::RuntimeHost (existing)
│   └── engine::runtime::RuntimeLoopPlan (existing)
└── engine::rendering::PresentationBackend (RT-410)
    └── engine::rendering::FrameGraph (existing)
```

### Core Application Class

```cpp
// engine/runtime/include/engine/runtime/application.hpp

namespace engine::runtime
{
    /// Configuration for application initialization
    struct ApplicationConfig
    {
        /// Window configuration
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
        double target_fps{60.0};
        
        /// Enable VSync
        bool vsync{true};
        
        /// Enable runtime diagnostics
        bool enable_diagnostics{false};
    };
    
    /// Base class for engine applications
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
        
        /// Run the application main loop
        /// Returns exit code (0 = success)
        int run();
        
        /// Request application shutdown
        void quit(int exit_code = 0);
        
    protected:
        // Lifecycle callbacks (override in derived class)
        
        /// Called once before main loop starts
        virtual void on_initialize() {}
        
        /// Called once per frame before rendering
        /// @param delta_time Time since last frame in seconds
        virtual void on_update(double delta_time) {}
        
        /// Called once per frame for rendering (optional)
        /// Frame graph execution happens automatically
        virtual void on_render() {}
        
        /// Called once after main loop exits
        virtual void on_shutdown() {}
        
        // Subsystem accessors
        
        /// Access the platform window
        [[nodiscard]] platform::Window& window() noexcept { return *window_; }
        [[nodiscard]] const platform::Window& window() const noexcept { return *window_; }
        
        /// Access input state
        [[nodiscard]] platform::input::InputState& input() noexcept;
        [[nodiscard]] const platform::input::InputState& input() const noexcept;
        
        /// Access runtime host
        [[nodiscard]] RuntimeHost& runtime() noexcept { return *runtime_; }
        [[nodiscard]] const RuntimeHost& runtime() const noexcept { return *runtime_; }
        
        /// Access scene (convenience)
        [[nodiscard]] scene::Scene& scene() noexcept;
        [[nodiscard]] const scene::Scene& scene() const noexcept;
        
        /// Get elapsed time since application start
        [[nodiscard]] double elapsed_time() const noexcept { return elapsed_time_; }
        
        /// Get current frame number
        [[nodiscard]] std::uint64_t frame_count() const noexcept { return frame_count_; }
        
    private:
        void initialize_subsystems();
        void shutdown_subsystems();
        void run_main_loop();
        
        ApplicationConfig config_;
        std::shared_ptr<platform::Window> window_;
        std::unique_ptr<RuntimeHost> runtime_;
        std::unique_ptr<rendering::PresentationBackend> presentation_;
        
        bool running_{false};
        int exit_code_{0};
        double elapsed_time_{0.0};
        std::uint64_t frame_count_{0};
    };
    
} // namespace engine::runtime
```

## Implementation Details

### 1. Input Integration

First, extend `Window` interface to expose input:

```cpp
// engine/platform/include/engine/platform/windowing/window.hpp

class Window
{
public:
    // ...existing methods...
    
    /// Access input state (automatically updated during pump_events)
    [[nodiscard]] virtual input::InputState& input_state() = 0;
    [[nodiscard]] virtual const input::InputState& input_state() const = 0;
};
```

Then implement in GLFW backend:

```cpp
// engine/platform/src/windowing/glfw_window.cpp

class GlfwWindow : public HeadlessWindow
{
private:
    input::InputState input_state_;
    
    static void key_callback(GLFWwindow* window, int key, int scancode, 
                            int action, int mods)
    {
        auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        auto mapped_key = map_glfw_key(key);
        bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        self->input_state_.apply_key_event(mapped_key, pressed);
    }
    
    static void mouse_button_callback(GLFWwindow* window, int button, 
                                     int action, int mods)
    {
        auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        auto mapped_button = map_glfw_mouse_button(button);
        bool pressed = (action == GLFW_PRESS);
        self->input_state_.apply_mouse_button_event(mapped_button, pressed);
    }
    
    static void cursor_position_callback(GLFWwindow* window, double x, double y)
    {
        auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        self->input_state_.apply_cursor_position(static_cast<float>(x), 
                                                 static_cast<float>(y));
    }
    
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        self->input_state_.apply_scroll_delta(static_cast<float>(xoffset), 
                                             static_cast<float>(yoffset));
    }
    
    void pump_events() override
    {
        input_state_.begin_frame();
        glfwPollEvents();
        // Callbacks update input_state_ automatically
    }
};
```

### 2. Application Implementation

```cpp
// engine/runtime/src/application.cpp

namespace engine::runtime
{
    Application::Application(const ApplicationConfig& config)
        : config_{config}
    {
    }
    
    Application::~Application() noexcept
    {
        if (running_)
        {
            shutdown_subsystems();
        }
    }
    
    int Application::run()
    {
        if (running_)
        {
            return -1; // Already running
        }
        
        try
        {
            initialize_subsystems();
            on_initialize();
            run_main_loop();
            on_shutdown();
            shutdown_subsystems();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Application error: " << e.what() << "\n";
            shutdown_subsystems();
            return -1;
        }
        
        return exit_code_;
    }
    
    void Application::quit(int exit_code)
    {
        running_ = false;
        exit_code_ = exit_code;
    }
    
    void Application::initialize_subsystems()
    {
        // Create window
        window_ = platform::create_window(config_.window, config_.window_backend);
        if (!window_)
        {
            throw std::runtime_error{"Failed to create window"};
        }
        
        // Create runtime host
        runtime_ = std::make_unique<RuntimeHost>();
        
        // Create presentation backend (when RT-410 completes)
        // presentation_ = rendering::create_presentation_backend(window_);
        
        running_ = true;
    }
    
    void Application::shutdown_subsystems()
    {
        presentation_.reset();
        runtime_.reset();
        window_.reset();
        running_ = false;
    }
    
    void Application::run_main_loop()
    {
        const double target_dt = config_.target_fps > 0.0 
            ? (1.0 / config_.target_fps) 
            : 0.0;
        
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (running_ && !window_->close_requested())
        {
            // Calculate delta time
            auto current_time = std::chrono::high_resolution_clock::now();
            double delta_time = std::chrono::duration<double>(
                current_time - last_time).count();
            last_time = current_time;
            
            // Process window events
            window_->pump_events();
            
            // Update application
            on_update(delta_time);
            
            // Execute runtime loop
            if (runtime_)
            {
                runtime_->tick(delta_time);
            }
            
            // Render
            on_render();
            
            // Present (when presentation backend exists)
            if (presentation_)
            {
                // rendering::RuntimePresentationContext ctx{
                //     .host = *runtime_,
                //     .delta_seconds = delta_time
                // };
                // presentation_->present(ctx);
            }
            
            // Update counters
            elapsed_time_ += delta_time;
            ++frame_count_;
            
            // Frame rate limiting
            if (target_dt > 0.0)
            {
                auto frame_end = std::chrono::high_resolution_clock::now();
                double frame_duration = std::chrono::duration<double>(
                    frame_end - current_time).count();
                
                if (frame_duration < target_dt)
                {
                    std::this_thread::sleep_for(
                        std::chrono::duration<double>(target_dt - frame_duration));
                }
            }
        }
    }
    
    platform::input::InputState& Application::input() noexcept
    {
        return window_->input_state();
    }
    
    const platform::input::InputState& Application::input() const noexcept
    {
        return window_->input_state();
    }
    
    scene::Scene& Application::scene() noexcept
    {
        // Access scene from runtime (needs API addition)
        // return runtime_->scene();
        static scene::Scene dummy;
        return dummy;
    }
    
} // namespace engine::runtime
```

## Usage Examples

### Minimal Application

```cpp
#include "engine/runtime/application.hpp"

class MinimalApp : public engine::runtime::Application
{
    void on_update(double dt) override
    {
        if (input().was_key_pressed(platform::input::Key::Escape))
        {
            quit();
        }
    }
};

int main()
{
    MinimalApp app;
    return app.run();
}
```

### Geometry Viewer (Refactored)

```cpp
#include "engine/runtime/application.hpp"

class GeometryViewer : public engine::runtime::Application
{
public:
    GeometryViewer()
        : Application({
            .window = {
                .title = "Geometry Viewer",
                .width = 1280,
                .height = 720
            },
            .target_fps = 60.0,
            .vsync = true
        })
    {}
    
protected:
    void on_initialize() override
    {
        setup_scene();
        setup_camera();
    }
    
    void on_update(double dt) override
    {
        update_camera_controls(dt);
        
        if (input().was_key_pressed(platform::input::Key::Escape))
        {
            quit();
        }
    }
    
private:
    void setup_scene()
    {
        auto entity = scene().create_entity();
        // Add mesh, material, etc.
    }
    
    void setup_camera()
    {
        camera_entity_ = scene().create_entity();
        // Setup camera component
    }
    
    void update_camera_controls(double dt)
    {
        auto& input_state = input();
        
        // Mouse orbit
        if (input_state.is_mouse_button_down(platform::input::Key::MouseButton::Left))
        {
            auto delta = input_state.cursor_delta();
            camera_yaw_ += delta.x * 0.005f;
            camera_pitch_ -= delta.y * 0.005f;
        }
        
        // Zoom
        auto scroll = input_state.scroll_delta();
        camera_distance_ -= scroll.y * 0.1f;
        
        // Update camera transform
        update_camera_transform();
    }
    
    void update_camera_transform()
    {
        // Calculate position and update camera component
    }
    
    entt::entity camera_entity_{entt::null};
    float camera_yaw_{0.0f};
    float camera_pitch_{0.3f};
    float camera_distance_{5.0f};
};

int main()
{
    GeometryViewer app;
    return app.run();
}
```

## Dependencies

This proposal depends on:

1. ✅ **Platform Input Integration** - Add `Window::input_state()`
2. ⚠️ **RT-410** - Presentation backend implementation
3. ⚠️ **T-0119/T-0120** - GPU execution for actual rendering

## Benefits

1. **Reduced Boilerplate**: No manual loop, input wiring, or subsystem management
2. **Consistent Patterns**: All apps follow same structure
3. **Easier Onboarding**: New developers can start quickly
4. **Better Abstraction**: Hide platform/rendering complexity
5. **Future-Proof**: Easy to add features (scripting, hot-reload, etc.)

## Migration Path

### Phase 1: Add Window::input_state()
- Wire InputState into platform backends
- Update tests

### Phase 2: Implement Application base class
- Basic lifecycle support
- Manual presentation for now

### Phase 3: Integrate RT-410
- Wire presentation backends
- Automatic swapchain management

### Phase 4: Optimize and Extend
- Add diagnostics integration
- Add scripting hooks
- Add hot-reload support

## Open Questions

1. Should `Application` own the scene, or should apps create their own?
2. Should there be multiple scene support?
3. How to handle multi-window applications?
4. Should frame rate limiting be configurable at runtime?
5. How to integrate with Dear ImGui for tools?

## Recommendation

**Implement this framework incrementally**:
1. Start with `Window::input_state()` integration (Quick win)
2. Design `Application` API (Community feedback)
3. Implement basic version without presentation (Testable)
4. Integrate RT-410 when ready (Full featured)

This provides immediate value while building toward the complete vision.

