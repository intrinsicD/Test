#include "engine/runtime/application.hpp"
#include "engine/platform/api.hpp"
#include "engine/scene/scene.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

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

        running_ = true;
        elapsed_time_ = 0.0;
        frame_count_ = 0;
    }

    void Application::shutdown_subsystems()
    {
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

} // namespace engine::runtime

