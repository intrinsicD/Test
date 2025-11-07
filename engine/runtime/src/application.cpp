#include "engine/runtime/application.hpp"
#include "engine/platform/api.hpp"
#include "engine/scene/scene.hpp"

#if ENGINE_ENABLE_RENDERING
#include "engine/assets/handles.hpp"
#include "engine/rendering/backend/mock/presentation_backend.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"
#include "engine/rendering/backend/stub_gpu_scheduler_base.hpp"
#include "engine/rendering/command_encoder.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/presentation_backend.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/runtime/render_submission.hpp"
#include "engine/geometry/api.hpp"
#endif

#include <chrono>
#include <stdexcept>
#include <thread>
#include <optional>

namespace
{
#if ENGINE_ENABLE_RENDERING
    class ApplicationRenderResourceProvider final : public engine::rendering::RenderResourceProvider
    {
    public:
        void require_mesh(const engine::assets::MeshHandle&) override {}
        void require_graph(const engine::assets::GraphHandle&) override {}
        void require_point_cloud(const engine::assets::PointCloudHandle&) override {}
        void require_material(const engine::assets::MaterialHandle&) override {}
        void require_shader(const engine::assets::ShaderHandle&) override {}
    };

    class ApplicationGpuScheduler final : public engine::rendering::backend::StubGpuSchedulerBase
    {
    };

    using RenderingConfig = engine::runtime::ApplicationConfig::RenderingConfig;

    [[nodiscard]] RenderingConfig::Backend resolve_backend(
        const engine::runtime::ApplicationConfig& config) noexcept
    {
        using Backend = RenderingConfig::Backend;
        if (config.rendering.backend == Backend::Auto)
        {
            if (config.window_backend == engine::platform::WindowBackend::GLFW)
            {
                return Backend::OpenGL;
            }

            return Backend::Mock;
        }

        return config.rendering.backend;
    }

    std::shared_ptr<engine::rendering::PresentationBackend> instantiate_backend(
        RenderingConfig::Backend backend,
        engine::runtime::RuntimeHost& runtime_host)
    {
        using MockBackend = engine::rendering::backend::mock::MockPresentationBackend;
        using OpenGlBackend = engine::rendering::backend::opengl::OpenGLPresentationBackend;

        switch (backend)
        {
        case RenderingConfig::Backend::Mock:
            return std::make_shared<MockBackend>();
        case RenderingConfig::Backend::OpenGL:
            try
            {
                auto backend_instance = std::make_shared<OpenGlBackend>(
                    [&runtime_host](const engine::assets::MeshHandle&)
                        -> std::optional<engine::geometry::SurfaceMesh>
                    {
                        if (!runtime_host.is_initialized())
                        {
                            return std::nullopt;
                        }

                        try
                        {
                            return runtime_host.current_mesh();
                        }
                        catch (...) // NOLINT(bugprone-empty-catch)
                        {
                            return std::nullopt;
                        }
                    });
                return backend_instance;
            }
            catch (...) // NOLINT(bugprone-empty-catch)
            {
                return nullptr;
            }
        case RenderingConfig::Backend::Auto:
            break;
        }

        return nullptr;
    }
#endif
} // namespace

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
            // Log error (TODO: use engine logging when available)
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

    platform::Window& Application::window() noexcept
    {
        return *window_;
    }

    const platform::Window& Application::window() const noexcept
    {
        return *window_;
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
        return *scene_;
    }

    const scene::Scene& Application::scene() const noexcept
    {
        return *scene_;
    }

    void Application::initialize_subsystems()
    {
        // Create window
        window_ = platform::create_window(config_.window, config_.window_backend);
        if (!window_)
        {
            throw std::runtime_error{"Failed to create window"};
        }

        // Create scene
        scene_ = std::make_unique<scene::Scene>("Application Scene");

#if ENGINE_ENABLE_RENDERING
        initialize_rendering_subsystem();
#endif

        running_ = true;
        elapsed_time_ = 0.0;
        frame_count_ = 0;
    }

    void Application::shutdown_subsystems()
    {
#if ENGINE_ENABLE_RENDERING
        shutdown_rendering_subsystem();
#endif
        scene_.reset();
        window_.reset();
        running_ = false;
    }

    void Application::run_main_loop()
    {
        using clock = std::chrono::high_resolution_clock;
        using duration = std::chrono::duration<double>;

        auto last_time = clock::now();

        while (running_ && !window_->close_requested())
        {
            // Calculate delta time
            const auto current_time = clock::now();
            const double delta_time = duration(current_time - last_time).count();
            last_time = current_time;

            // Update frame timing
            elapsed_time_ += delta_time;
            ++frame_count_;

            // Pump window events (updates input state)
            window_->pump_events();

            // Update scene (processes hierarchy, transforms, etc.)
            scene_->update();

            // Call user update callback
            on_update(delta_time);

            // Call user render callback
            on_render();
#if ENGINE_ENABLE_RENDERING
            if (rendering_.backend)
            {
                if (!runtime_host_)
                {
                    runtime_host_ = std::make_unique<RuntimeHost>();
                    runtime_host_->initialize();
                    runtime_host_->set_presentation_backend(rendering_.backend);
                }
                rendering::RuntimePresentationContext presentation_context{
                    *runtime_host_,
                    delta_time,
                    nullptr};
                presentation_context.submit_render_graph = &submit_render_graph;
                rendering_.backend->present(presentation_context);
            }
#endif

            // Optional: frame rate limiting
            if (config_.target_fps > 0.0)
            {
                const double target_frame_time = 1.0 / config_.target_fps;
                const auto frame_end = clock::now();
                const double frame_time = duration(frame_end - current_time).count();

                if (frame_time < target_frame_time)
                {
                    const double sleep_time = target_frame_time - frame_time;
                    std::this_thread::sleep_for(
                        std::chrono::duration<double>(sleep_time));
                }
            }
        }
    }

#if ENGINE_ENABLE_RENDERING
    void Application::initialize_rendering_subsystem()
    {
        if (!config_.rendering.enable)
        {
            return;
        }

        if (!rendering_.resources)
        {
            rendering_.resources = std::make_unique<ApplicationRenderResourceProvider>();
        }
        if (!rendering_.materials)
        {
            rendering_.materials = std::make_unique<rendering::MaterialSystem>();
        }
        if (!rendering_.device_resources)
        {
            rendering_.device_resources = std::make_unique<rendering::resources::RecordingGpuResourceProvider>(
                rendering::resources::GraphicsApi::OpenGL);
        }
        if (!rendering_.scheduler)
        {
            rendering_.scheduler = std::make_unique<ApplicationGpuScheduler>();
        }
        if (!rendering_.encoders)
        {
            rendering_.encoders = std::make_unique<rendering::RecordingCommandEncoderProvider>();
        }

        if (!runtime_host_)
        {
            runtime_host_ = std::make_unique<RuntimeHost>();
            runtime_host_->initialize();
        }

        if (config_.rendering.backend_factory)
        {
            rendering_.backend = config_.rendering.backend_factory();
        }
        else
        {
            const auto backend_choice = resolve_backend(config_);
            rendering_.backend = instantiate_backend(backend_choice, *runtime_host_);
        }

        if (!rendering_.backend)
        {
            rendering_.backend = std::make_shared<rendering::backend::mock::MockPresentationBackend>();
        }

        runtime_host_->set_presentation_backend(rendering_.backend);

        rendering_.context.emplace(
            *rendering_.resources,
            *rendering_.materials,
            rendering::RenderView{*scene_},
            *rendering_.scheduler,
            *rendering_.device_resources,
            *rendering_.encoders);
    }

    void Application::shutdown_rendering_subsystem() noexcept
    {
        rendering_.context.reset();
        rendering_.encoders.reset();
        rendering_.scheduler.reset();
        rendering_.device_resources.reset();
        rendering_.materials.reset();
        rendering_.resources.reset();
        rendering_.backend.reset();

        if (runtime_host_)
        {
            runtime_host_->set_presentation_backend(nullptr);
            runtime_host_->shutdown();
            runtime_host_.reset();
        }
    }

    rendering::RenderExecutionContext& Application::render_context()
    {
        if (!rendering_.context)
        {
            throw std::runtime_error("Render context unavailable. Enable rendering in ApplicationConfig.");
        }
        return *rendering_.context;
    }

    const rendering::RenderExecutionContext& Application::render_context() const
    {
        if (!rendering_.context)
        {
            throw std::runtime_error("Render context unavailable. Enable rendering in ApplicationConfig.");
        }
        return *rendering_.context;
    }
#endif

} // namespace engine::runtime

