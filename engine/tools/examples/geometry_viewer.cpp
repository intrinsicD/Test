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

#include "engine/core/log.hpp"
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
#include "engine/geometry/api.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"
#include "engine/assets/validation.hpp"
#include <unordered_map>
#include <memory>

namespace
{
    /// Storage for procedural meshes accessible to the rendering backend
    struct ProceduralMeshStorage
    {
        std::unordered_map<std::string, engine::geometry::SurfaceMesh> meshes;

        void store(const std::string& name, engine::geometry::SurfaceMesh mesh)
        {
            meshes[name] = std::move(mesh);
        }

        std::optional<engine::geometry::SurfaceMesh> get(const std::string& name) const
        {
            auto it = meshes.find(name);
            if (it != meshes.end())
            {
                return it->second;
            }
            return std::nullopt;
        }
    };

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
            : mesh_storage_(std::make_shared<ProceduralMeshStorage>()),
              Application({
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
                .target_fps = 0.0, // Unlimited
#if ENGINE_ENABLE_RENDERING
                .rendering = {
                    .enable = true,
                    .backend = engine::runtime::ApplicationConfig::RenderingConfig::Backend::OpenGL,
                    .backend_factory = [this]() -> std::shared_ptr<engine::rendering::PresentationBackend> {
                        // Create custom MeshResolver that accesses our procedural mesh storage
                        auto storage = this->mesh_storage_;
                        auto mesh_resolver = [storage](const engine::assets::MeshHandle& handle)
                            -> std::optional<engine::geometry::SurfaceMesh>
                        {
                            ENGINE_INFO("→ Mesh resolver called for: '{}'", handle.id());
                            auto mesh = storage->get(handle.id());
                            if (mesh.has_value()) {
                                ENGINE_INFO("  ✓ Mesh found: {} vertices, {} indices",
                                           mesh->positions.size(), mesh->indices.size());
                            } else {
                                ENGINE_WARN("  ✗ Mesh '{}' not found in storage!", handle.id());
                            }
                            return mesh;
                        };

                        return std::make_shared<engine::rendering::backend::opengl::OpenGLPresentationBackend>(
                            mesh_resolver);
                    }
                }
#endif
            })
        {
        }

    protected:
        void on_initialize() override
        {
            ENGINE_INFO("=== Initializing Geometry Viewer ===");

            // Create and store procedural mesh FIRST (before validator registration)
            auto cube_mesh = engine::geometry::make_unit_cube();
            ENGINE_INFO("  Cube mesh created: {} vertices, {} indices",
                       cube_mesh.positions.size(), cube_mesh.indices.size());
            mesh_storage_->store("procedural_cube", std::move(cube_mesh));
            ENGINE_INFO("  ✓ Created and stored procedural cube");

            // Now register validator for procedural meshes
            [[maybe_unused]] auto validator_registered =
                engine::assets::HandleValidatorRegistry::instance().register_mesh_validator(
                    [storage = mesh_storage_](const engine::assets::MeshHandle& handle) -> bool {
                        ENGINE_INFO("→ Validating mesh handle: '{}'", handle.id());
                        bool valid = storage->get(handle.id()).has_value();
                        ENGINE_INFO("  Validation result: {}", valid ? "VALID" : "INVALID");
                        return valid;
                    });

            // Setup scene with cube
            setup_scene();

            // Setup camera
            setup_camera();

            // Configure frame graph
            setup_frame_graph();

            ENGINE_INFO("=== Initialization Complete ===");
            ENGINE_INFO("Controls:");
            ENGINE_INFO("  - Left mouse drag: Rotate camera");
            ENGINE_INFO("  - Mouse scroll: Zoom in/out");
            ENGINE_INFO("  - ESC: Exit");
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
            // The backend's present() will be called automatically by the Application framework
            // after this method returns. It will initialize OpenGL context, execute the frame graph,
            // and swap buffers.

            render_frame_count_++;
            if (render_frame_count_ <= 5)
            {
                ENGINE_INFO("═══ FRAME {} ═══", render_frame_count_);
            }
        }

        void on_shutdown() override
        {
            ENGINE_INFO("=== Shutting down ===");
        }

    private:
        void setup_scene()
        {
            ENGINE_DEBUG("Creating scene...");


            auto& registry = scene().registry();

            // Create a cube entity
            auto cube = registry.create();

            // Add transform component
            auto& transform = registry.emplace<engine::scene::components::WorldTransform>(cube);
            transform.value = engine::math::Transform<float>::Identity();

            // Add RenderGeometry component with our procedural mesh
            auto mesh_handle = engine::assets::MeshHandle{std::string{"procedural_cube"}};
            ENGINE_DEBUG("  Created mesh handle with id: '{}'", mesh_handle.id());

            auto material_handle = engine::assets::MaterialHandle{}; // Empty material for now

            registry.emplace<engine::rendering::components::RenderGeometry>(
                cube,
                engine::rendering::components::RenderGeometry::from_mesh(mesh_handle, material_handle));

            // Verify entity and components
            ENGINE_INFO("  ✓ Scene created with 1 renderable cube entity");
            ENGINE_INFO("  Entity ID: {}", static_cast<std::uint32_t>(cube));
            ENGINE_INFO("  Has WorldTransform: {}", registry.all_of<engine::scene::components::WorldTransform>(cube));
            ENGINE_INFO("  Has RenderGeometry: {}", registry.all_of<engine::rendering::components::RenderGeometry>(cube));

            // Verify geometry component
            if (registry.all_of<engine::rendering::components::RenderGeometry>(cube))
            {
                const auto& geom = registry.get<engine::rendering::components::RenderGeometry>(cube);
                ENGINE_INFO("  Geometry component:");
                ENGINE_INFO("    empty(): {}", geom.empty());
                ENGINE_INFO("    has_mesh(): {}", geom.has_mesh());
                ENGINE_INFO("    has_graph(): {}", geom.has_graph());
                ENGINE_INFO("    has_point_cloud(): {}", geom.has_point_cloud());

                if (const auto* mesh = geom.mesh())
                {
                    ENGINE_INFO("  Mesh handle: '{}', empty: {}, bound: {}",
                               mesh->id(), mesh->empty(), mesh->is_bound());
                }
                else
                {
                    ENGINE_WARN("  mesh() returned nullptr!");
                }
            }

            // Test the same query that the render pass uses
            using engine::rendering::components::RenderGeometry;
            using engine::scene::components::WorldTransform;
            auto test_view = registry.view<WorldTransform, RenderGeometry>();
            int count = 0;
            for (auto [entity, world, geom] : test_view.each())
            {
                (void)entity;
                (void)world;
                (void)geom;
                count++;
            }
            ENGINE_INFO("  Entities matching render query: {}", count);
        }

        void setup_camera()
        {
            ENGINE_DEBUG("Setting up camera...");

            auto& registry = scene().registry();

            // Create camera entity
            camera_entity_ = registry.create();

            // Add camera component
            auto& camera = registry.emplace<engine::rendering::Camera>(camera_entity_);

            // Set up perspective projection
            const float aspect_ratio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
            camera.set_perspective(
                1.047f, // ~60 degrees FOV
                aspect_ratio,
                0.1f, // Near plane
                100.0f // Far plane
            );

            // Position camera
            update_camera_position(camera);

            ENGINE_INFO("  ✓ Camera created with orbit controller");
        }

        void setup_frame_graph()
        {
            ENGINE_DEBUG("Configuring research baseline rendering preset...");

#if ENGINE_ENABLE_RENDERING
            // Get the OpenGL backend's frame graph
            auto* opengl_backend = dynamic_cast<engine::rendering::backend::opengl::OpenGLPresentationBackend*>(
                rendering_backend().get());

            if (opengl_backend)
            {
                engine::rendering::ResearchBaselineOptions options{};
                options.shading_mode = engine::rendering::ResearchShadingMode::Forward;
                options.width = WINDOW_WIDTH;
                options.height = WINDOW_HEIGHT;
                options.enable_normals_overlay = false;

                // Configure the backend's frame graph
                baseline_resources_ = engine::rendering::configure_research_baseline(
                    opengl_backend->frame_graph(), options);

                ENGINE_DEBUG("Compiling frame graph...");
                opengl_backend->frame_graph().compile();

                ENGINE_INFO("  ✓ Final color: {}", (baseline_resources_.lighting_output.valid() ? "✓" : "✗"));
                ENGINE_INFO("  ✓ Depth buffer: {}", (baseline_resources_.depth.valid() ? "✓" : "✗"));
            }
            else
            {
                ENGINE_WARN("  ✗ Backend is not OpenGL, skipping frame graph configuration");
            }
#endif
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


            auto result = camera.look_at(camera_pos, target, up);
            if (!result) {
                ENGINE_WARN("Camera look_at failed!");
            }
        }

        void print_fps(double delta_time)
        {
            fps_frame_count_++;
            fps_time_accumulator_ += delta_time;

            if (fps_time_accumulator_ >= 2.0)
            {
                const float fps = static_cast<float>(fps_frame_count_) / static_cast<float>(fps_time_accumulator_);
                ENGINE_DEBUG("FPS: {:.1f} (Camera: yaw={:.2f}, pitch={:.2f}, radius={:.2f})",
                    fps, camera_yaw_, camera_pitch_, camera_radius_);
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
        int render_frame_count_{0};

        // Rendering resources
        engine::rendering::ResearchBaselineResources baseline_resources_{};

        // Procedural mesh storage
        std::shared_ptr<ProceduralMeshStorage> mesh_storage_;
    };
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // Initialize logging system
    engine::core::Log::init();

    ENGINE_INFO("=== Test Engine Geometry Viewer ===");
    ENGINE_INFO("Interactive 3D Viewer with Orbit Camera");

    try
    {
        GeometryViewerApp app;
        int result = app.run();

        engine::core::Log::shutdown();
        return result;
    }
    catch (const std::exception& e)
    {
        ENGINE_CRITICAL("Error: {}", e.what());
        engine::core::Log::shutdown();
        return EXIT_FAILURE;
    }
}
