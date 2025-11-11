#pragma once

#include "engine/runtime/api.hpp"
#include "engine/platform/windowing/window.hpp"
#include "engine/platform/input/input_state.hpp"
#include "engine/scene/scene.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace engine::runtime
{
    class RuntimeLoopPlan;
}

namespace engine::rendering
{
    class RenderResourceProvider;
    class MaterialSystem;
    class CommandEncoderProvider;
    struct RenderExecutionContext;
    class PresentationBackend;
    class IGpuScheduler;

    namespace resources
    {
        class IGpuResourceProvider;
    } // namespace resources
} // namespace engine::rendering

namespace engine::runtime
{
    /// \brief Configuration for application initialization
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

#if ENGINE_ENABLE_RENDERING
        /// Rendering subsystem configuration
        struct RenderingConfig
        {
            /// Available rendering backends.
            enum class Backend
            {
                Auto,
                Mock,
                OpenGL,
                //Vulkan // Future backend TODO
            };

            /// Enable rendering integration for the application.
            bool enable{false};

            /// Preferred rendering backend.
            Backend backend{Backend::Auto};

            /// Optional factory that returns a presentation backend instance.
            std::function<std::shared_ptr<engine::rendering::PresentationBackend>()>
                backend_factory{};
        };

        RenderingConfig rendering{};
#endif
    };

    /// \brief Base class for engine applications
    ///
    /// Provides automatic window creation, runtime host management, and a built-in main loop.
    /// Applications inherit from this class and override lifecycle callbacks to implement
    /// application-specific behavior.
    ///
    /// \par Example Usage:
    /// \code
    /// class MyApp : public engine::runtime::Application
    /// {
    /// protected:
    ///     void on_initialize() override {
    ///         // Setup scene, load resources
    ///     }
    ///
    ///     void on_update(double dt) override {
    ///         auto& input = this->input();
    ///         if (input.is_key_down(platform::input::Key::W)) {
    ///             // Move forward
    ///         }
    ///     }
    ///
    ///     void on_render() override {
    ///         // Rendering (when RT-410 completes)
    ///     }
    /// };
    ///
    /// int main() {
    ///     MyApp app;
    ///     return app.run();
    /// }
    /// \endcode
    class ENGINE_RUNTIME_API Application
    {
    public:
        /// \brief Construct application with configuration
        /// \param config Application and window configuration
        explicit Application(const ApplicationConfig& config = {});

        /// \brief Virtual destructor for polymorphic cleanup
        virtual ~Application() noexcept;

        // Disable copy/move semantics
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        /// \brief Run the application main loop
        ///
        /// This method handles the complete application lifecycle:
        /// 1. Initialize subsystems (window, runtime, scene)
        /// 2. Call on_initialize()
        /// 3. Run main loop (pump events, on_update, on_render)
        /// 4. Call on_shutdown()
        /// 5. Clean up subsystems
        ///
        /// \return Exit code (0 = success, non-zero = error)
        int run();

        /// \brief Request application shutdown
        ///
        /// Sets a flag that will cause the main loop to exit gracefully at the end
        /// of the current frame.
        ///
        /// \param exit_code Exit code to return from run() (default: 0)
        void quit(int exit_code = 0);

    protected:
        // Lifecycle callbacks (override in derived classes)

        /// \brief Called once before main loop starts
        ///
        /// Override to initialize application-specific resources, setup scene,
        /// load assets, etc. All subsystems (window, runtime, scene) are available.
        virtual void on_initialize() {}

        /// \brief Called once per frame before rendering
        ///
        /// Override to implement game logic, physics updates, input handling, etc.
        ///
        /// \param delta_time Time elapsed since last frame in seconds
        virtual void on_update(double delta_time) {}

        /// \brief Called once per frame for rendering
        ///
        /// Override to implement custom rendering logic. In future phases, this will
        /// integrate with the presentation backend (RT-410).
        virtual void on_render() {}

        /// \brief Called once after main loop exits
        ///
        /// Override to clean up application-specific resources. Subsystems will be
        /// automatically cleaned up after this callback returns.
        virtual void on_shutdown() {}

        // Subsystem accessors

        /// \brief Access the platform window
        /// \return Reference to the application window
        /// \pre run() has been called and subsystems are initialized
        [[nodiscard]] platform::Window& window() noexcept;

        /// \brief Access the platform window (const)
        /// \return Const reference to the application window
        /// \pre run() has been called and subsystems are initialized
        [[nodiscard]] const platform::Window& window() const noexcept;

        /// \brief Access input state (convenience wrapper for window().input_state())
        /// \return Reference to the current frame's input state
        /// \pre run() has been called and subsystems are initialized
        [[nodiscard]] platform::input::InputState& input() noexcept;

        /// \brief Access input state (const)
        /// \return Const reference to the current frame's input state
        /// \pre run() has been called and subsystems are initialized
        [[nodiscard]] const platform::input::InputState& input() const noexcept;

        /// \brief Access scene
        /// \return Reference to the scene
        /// \pre run() has been called and subsystems are initialized
        [[nodiscard]] scene::Scene& scene() noexcept;

        /// \brief Access scene (const)
        /// \return Const reference to the scene
        /// \pre run() has been called and subsystems are initialized
        [[nodiscard]] const scene::Scene& scene() const noexcept;

#if ENGINE_ENABLE_RENDERING
        /// \brief Access the render execution context.
        /// \return Reference to the current render execution context
        /// \pre Rendering has been enabled via ApplicationConfig
        [[nodiscard]] rendering::RenderExecutionContext& render_context();

        /// \brief Access the render execution context (const).
        /// \return Const reference to the current render execution context
        /// \pre Rendering has been enabled via ApplicationConfig
        [[nodiscard]] const rendering::RenderExecutionContext& render_context() const;

        /// \brief Access the underlying runtime host.
        /// \return Reference to the runtime host used by the application.
        [[nodiscard]] RuntimeHost& runtime_host() noexcept;

        /// \brief Access the underlying runtime host (const).
        /// \return Const reference to the runtime host used by the application.
        [[nodiscard]] const RuntimeHost& runtime_host() const noexcept;
#endif

        /// \brief Get elapsed time since application start
        /// \return Total elapsed time in seconds
        [[nodiscard]] double elapsed_time() const noexcept { return elapsed_time_; }

        /// \brief Get current frame number
        /// \return Number of frames executed (starts at 0)
        [[nodiscard]] std::uint64_t frame_count() const noexcept { return frame_count_; }

    private:
        void initialize_subsystems();
        void shutdown_subsystems();
        void run_main_loop();

#if ENGINE_ENABLE_RENDERING
        void initialize_rendering_subsystem();
        void shutdown_rendering_subsystem() noexcept;
#endif

#if ENGINE_ENABLE_RENDERING
        /// \brief Configure the runtime host prior to initialization.
        ///
        /// Derived classes may override to provide custom dependencies or streaming providers
        /// before the host is initialized.
        virtual void configure_runtime_host(RuntimeHost& host);
#endif

        ApplicationConfig config_;
        std::shared_ptr<platform::Window> window_;
        std::unique_ptr<scene::Scene> scene_;

        bool running_{false};
        int exit_code_{0};
        double elapsed_time_{0.0};
        std::uint64_t frame_count_{0};

#if ENGINE_ENABLE_RENDERING
        struct RenderingSubsystem
        {
            std::unique_ptr<rendering::RenderResourceProvider> resources;
            std::unique_ptr<rendering::MaterialSystem> materials;
            std::unique_ptr<rendering::resources::IGpuResourceProvider> device_resources;
            std::unique_ptr<rendering::IGpuScheduler> scheduler;
            std::unique_ptr<rendering::CommandEncoderProvider> encoders;
            std::shared_ptr<rendering::PresentationBackend> backend;
            std::optional<rendering::RenderExecutionContext> context;
        } rendering_{};

        std::unique_ptr<RuntimeHost> runtime_host_{};
#endif
    };

} // namespace engine::runtime

