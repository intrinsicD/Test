/**
 * @file geometry_viewer.cpp
 * @brief Interactive 3D geometry viewer using Application framework
 *
 * This example demonstrates the engine's Application base class:
 * - Inherit from runtime::Application for automatic lifecycle management
 * - Override lifecycle callbacks (on_initialize, on_update, on_render)
 * - Access subsystems through protected accessors (window(), input(), scene())
 * - Clean, minimal main() function
 *
 * Controls:
 * - Left mouse drag: Rotate camera around target
 * - Mouse scroll: Zoom in/out
 * - ESC: Exit application
 */

#include <cmath>
#include <cstdlib>
#include <iostream>

#include "engine/runtime/application.hpp"
#include "engine/platform/input/input_state.hpp"
#include "engine/rendering/api.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/rendering/camera_controllers.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/math/transform.hpp"

namespace
{
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr float CAMERA_DISTANCE = 5.0f;
    constexpr float CAMERA_ROTATE_SPEED = 0.005f;
    constexpr float CAMERA_ZOOM_SPEED = 0.1f;

    /// Geometry Viewer Application
    class GeometryViewerApp : public engine::runtime::Application
    {
    public:
        GeometryViewerApp()
            : Application({
                .window = {
                    .title = "Geometry Viewer - Research Baseline",
                    .width = WINDOW_WIDTH,
                    .height = WINDOW_HEIGHT,
                    .visible = true,
                    .resizable = true,
                    .capability_requirements = {
                        .require_native_surface = true
                    }
                },
                .window_backend = engine::platform::WindowBackend::GLFW,
                .target_fps = 0.0,  // Unlimited
#if ENGINE_ENABLE_RENDERING
                .rendering = {
                    .enable = true,
                    .backend = engine::runtime::ApplicationConfig::RenderingConfig::Backend::Auto,
                    .backend_factory = {}
                }
#endif
            })
        {
        }

    protected:
        void on_initialize() override
        {
            std::cout << "\n=== Initializing Geometry Viewer ===\n";

            // Setup scene with cube
            setup_scene();

            // Setup camera
            setup_camera();

            // Configure frame graph
            setup_frame_graph();

            std::cout << "=== Initialization Complete ===\n\n";
            std::cout << "Controls:\n";
            std::cout << "  - Left mouse drag: Rotate camera\n";
            std::cout << "  - Mouse scroll: Zoom in/out\n";
            std::cout << "  - ESC: Exit\n\n";
        }

        void on_update(double delta_time) override
        {
            // Process input
            handle_input();

            // Update camera
            update_camera();

            // Print FPS periodically
            print_fps(delta_time);
        }

        void on_render() override
        {
            frame_graph_.execute(render_context());
        }

        void on_shutdown() override
        {
            std::cout << "\n=== Shutting down ===\n";
        }

    private:
        void setup_scene()
        {
            std::cout << "Creating scene...\n";

            auto& registry = scene().registry();

            // Create a cube entity
            auto cube = registry.create();

            // Add transform component
            auto& transform = registry.emplace<engine::scene::components::WorldTransform>(cube);
            transform.value = engine::math::Transform<float>::Identity();

            // Add render geometry component
            auto mesh = engine::assets::MeshHandle{std::string{"examples/cube.mesh"}};
            auto material = engine::assets::MaterialHandle{std::string{"examples/default.material"}};
            registry.emplace<engine::rendering::components::RenderGeometry>(
                cube,
                engine::rendering::components::RenderGeometry::from_mesh(mesh, material));

            std::cout << "  ✓ Scene created with 1 cube entity\n";
        }

        void setup_camera()
        {
            std::cout << "Setting up camera...\n";

            auto& registry = scene().registry();

            // Create camera entity
            camera_entity_ = registry.create();

            // Add camera component
            auto& camera = registry.emplace<engine::rendering::Camera>(camera_entity_);

            // Set up perspective projection
            const float aspect_ratio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
            camera.set_perspective(
                1.047f,  // ~60 degrees FOV
                aspect_ratio,
                0.1f,    // Near plane
                100.0f   // Far plane
            );

            // Position camera
            update_camera_position(camera);

            std::cout << "  ✓ Camera created with orbit controller\n";
        }

        void setup_frame_graph()
        {
            std::cout << "Configuring research baseline rendering preset...\n";

            engine::rendering::ResearchBaselineOptions options{};
            options.shading_mode = engine::rendering::ResearchShadingMode::Forward;
            options.width = WINDOW_WIDTH;
            options.height = WINDOW_HEIGHT;
            options.enable_normals_overlay = false;

            frame_graph_ = engine::rendering::FrameGraph{};
            baseline_resources_ = engine::rendering::configure_research_baseline(frame_graph_, options);

            std::cout << "Compiling frame graph...\n";
            frame_graph_.compile();

            std::cout << "  ✓ Final color: " << (baseline_resources_.lighting_output.valid() ? "✓" : "✗") << "\n";
            std::cout << "  ✓ Depth buffer: " << (baseline_resources_.depth.valid() ? "✓" : "✗") << "\n";
        }

        void handle_input()
        {
            auto& inp = input();

            // Handle mouse drag for camera rotation
            if (inp.is_mouse_button_down(engine::platform::input::MouseButton::Left))
            {
                if (was_dragging_)
                {
                    auto delta = inp.cursor_delta();
                    camera_yaw_ += delta.x * CAMERA_ROTATE_SPEED;
                    camera_pitch_ -= delta.y * CAMERA_ROTATE_SPEED;

                    // Clamp pitch to avoid gimbal lock
                    camera_pitch_ = std::clamp(camera_pitch_, -1.5f, 1.5f);
                }
                else
                {
                    was_dragging_ = true;
                }
            }
            else
            {
                was_dragging_ = false;
            }

            // Handle scroll for zoom
            auto scroll = inp.scroll_delta();
            camera_radius_ -= scroll.y * CAMERA_ZOOM_SPEED;
            camera_radius_ = std::clamp(camera_radius_, 1.0f, 20.0f);

            // Handle ESC to exit
            if (inp.was_key_pressed(engine::platform::input::Key::Escape))
            {
                quit();
            }
        }

        void update_camera()
        {
            auto& registry = scene().registry();
            if (registry.valid(camera_entity_))
            {
                auto& camera = registry.get<engine::rendering::Camera>(camera_entity_);
                update_camera_position(camera);
            }
        }

        void update_camera_position(engine::rendering::Camera& camera)
        {
            const float cos_pitch = std::cos(camera_pitch_);
            const float sin_pitch = std::sin(camera_pitch_);
            const float cos_yaw = std::cos(camera_yaw_);
            const float sin_yaw = std::sin(camera_yaw_);

            // Calculate camera position in orbit
            const engine::math::vec3 camera_pos{
                camera_radius_ * cos_pitch * sin_yaw,
                camera_radius_ * sin_pitch,
                camera_radius_ * cos_pitch * cos_yaw
            };

            const engine::math::vec3 target{0.0f, 0.0f, 0.0f};
            const engine::math::vec3 up{0.0f, 1.0f, 0.0f};

            [[maybe_unused]] auto result = camera.look_at(camera_pos, target, up);
        }

        void print_fps(double delta_time)
        {
            fps_frame_count_++;
            fps_time_accumulator_ += delta_time;

            if (fps_time_accumulator_ >= 2.0)
            {
                const float fps = static_cast<float>(fps_frame_count_) / static_cast<float>(fps_time_accumulator_);
                std::cout << "FPS: " << fps << " (Camera: yaw=" << camera_yaw_
                          << ", pitch=" << camera_pitch_
                          << ", radius=" << camera_radius_ << ")\n";
                fps_frame_count_ = 0;
                fps_time_accumulator_ = 0.0;
            }
        }

        // Camera state
        entt::entity camera_entity_{entt::null};
        float camera_yaw_{0.0f};
        float camera_pitch_{0.3f};
        float camera_radius_{CAMERA_DISTANCE};
        bool was_dragging_{false};

        // FPS tracking
        int fps_frame_count_{0};
        double fps_time_accumulator_{0.0};

        engine::rendering::FrameGraph frame_graph_{};
        engine::rendering::ResearchBaselineResources baseline_resources_{};
    };
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "=== Test Engine Geometry Viewer ===\n";
    std::cout << "Interactive 3D Viewer with Orbit Camera\n";

    try
    {
        GeometryViewerApp app;
        return app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n❌ Error: " << e.what() << "\n";

        return EXIT_FAILURE;
    }
}