/**
 * @file geometry_viewer.cpp
 * @brief Interactive 3D geometry viewer using the runtime application framework.
 *
 * Features:
 * - Orbit camera controls (mouse drag + scroll) driven by the unified input system.
 * - Drag and drop mesh / point cloud files to stream them through the asset caches.
 * - OpenGL presentation backend executing the research baseline frame graph.
 */

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>

#include "engine/assets/mesh_asset.hpp"
#include "engine/assets/point_cloud_asset.hpp"
#include "engine/assets/validation.hpp"
#include "engine/core/log.hpp"
#include "engine/geometry/api.hpp"
#include "engine/geometry/mesh/surface_mesh.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"
#include "engine/geometry/point_cloud/point_cloud.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/io/geometry_io.hpp"
#include "engine/math/transform.hpp"
#include "engine/platform/input/input_state.hpp"
#include "engine/platform/windowing/event.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/runtime/api.hpp"
#include "engine/runtime/application.hpp"
#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/components/name.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems/transform.hpp"

namespace
{
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr float CAMERA_DISTANCE = 5.0f;
    constexpr float CAMERA_ROTATE_SPEED = 0.005f;
    constexpr float CAMERA_ZOOM_SPEED = 0.1f;
    constexpr std::string_view kProceduralCubeId{"procedural_cube"};

    class ProceduralMeshStorage
    {
    public:
        void store(std::string identifier, engine::geometry::SurfaceMesh mesh)
        {
            meshes_.insert_or_assign(std::move(identifier), std::move(mesh));
        }

        [[nodiscard]] bool contains(std::string_view identifier) const
        {
            return meshes_.find(std::string{identifier}) != meshes_.end();
        }

        [[nodiscard]] std::optional<engine::geometry::SurfaceMesh> get(std::string_view identifier) const
        {
            if (const auto it = meshes_.find(std::string{identifier}); it != meshes_.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        [[nodiscard]] const engine::geometry::SurfaceMesh* find(std::string_view identifier) const
        {
            if (const auto it = meshes_.find(std::string{identifier}); it != meshes_.end())
            {
                return &it->second;
            }
            return nullptr;
        }

    private:
        std::unordered_map<std::string, engine::geometry::SurfaceMesh> meshes_{};
    };

    class GeometryViewerApp : public engine::runtime::Application
    {
    public:
        GeometryViewerApp()
            : engine::runtime::Application([this]() {
                  engine::runtime::ApplicationConfig config{};
                  config.window = {
                      .title = "Geometry Viewer - Research Baseline",
                      .width = WINDOW_WIDTH,
                      .height = WINDOW_HEIGHT,
                      .visible = true,
                      .resizable = true,
                      .capability_requirements = {.require_native_surface = true}
                  };
                  config.window_backend = engine::platform::WindowBackend::GLFW;
                  config.target_fps = 0.0;
#if ENGINE_ENABLE_RENDERING
                  config.rendering.enable = true;
                  config.rendering.backend = engine::runtime::ApplicationConfig::RenderingConfig::Backend::OpenGL;
                  config.rendering.backend_factory = [this]() {
                      auto mesh_resolver = [this](const engine::assets::MeshHandle& handle)
                          -> std::optional<engine::geometry::SurfaceMesh> {
                          if (handle.empty())
                          {
                              return std::nullopt;
                          }

#    if ENGINE_ENABLE_ASSETS
                          if (mesh_cache_.contains(handle))
                          {
                              try
                              {
                                  const auto& asset = mesh_cache_.get(handle);
                                  return engine::geometry::mesh::build_surface_mesh_from_halfedge(
                                      asset.mesh.interface);
                              }
                              catch (const std::exception& ex)
                              {
                                  ENGINE_WARN("Failed to resolve mesh '{}' from cache: {}", handle.id(), ex.what());
                              }
                          }
#    endif

                          if (mesh_storage_)
                          {
                              if (auto mesh = mesh_storage_->get(handle.id()))
                              {
                                  return mesh;
                              }
                          }
                          return std::nullopt;
                      };

                      auto point_cloud_resolver = [this](const engine::assets::PointCloudHandle& handle)
                          -> std::optional<engine::geometry::PointCloud> {
                          if (handle.empty())
                          {
                              return std::nullopt;
                          }

#    if ENGINE_ENABLE_ASSETS
                          try
                          {
                              return point_cloud_cache_.get(handle).point_cloud;
                          }
                          catch (const std::exception& ex)
                          {
                              ENGINE_WARN(
                                  "Failed to resolve point cloud '{}' from cache: {}", handle.id(), ex.what());
                          }
#    endif
                          return std::nullopt;
                      };

                      auto backend = std::make_shared<engine::rendering::backend::opengl::OpenGLPresentationBackend>(
                          std::move(mesh_resolver), std::move(point_cloud_resolver));
                      backend_ = backend;
                      return backend;
                  };
#endif
                  return config;
              }())
        {
        }

    protected:
#if ENGINE_ENABLE_RENDERING
        void configure_runtime_host(engine::runtime::RuntimeHost& host) override
        {
            engine::runtime::RuntimeHostDependencies deps{};
            deps.render_geometry = {};
            deps.renderable_name = "geometry_viewer.renderable";
#    if ENGINE_ENABLE_ASSETS
            deps.asset_streaming.mesh_cache = &mesh_cache_;
            deps.asset_streaming.point_cloud_cache = &point_cloud_cache_;
#    endif
            host.configure(std::move(deps));
        }
#endif

        void on_initialize() override
        {
            ENGINE_INFO("=== Initializing Geometry Viewer ===");

            setup_backend();
            setup_camera();
            setup_procedural_assets();
            setup_scene();

#if ENGINE_ENABLE_RENDERING
            if (const auto* mesh = mesh_storage_->find(kProceduralCubeId))
            {
                focus_camera_on_bounds(mesh->bounds);
            }
#endif

            ENGINE_INFO("Drag and drop mesh (.obj/.ply/.stl) or point cloud (.ply/.pcd/.xyz) files into the window.");
        }

        void on_update(double delta_time) override
        {
            process_events();
            handle_input();
            update_camera();
#if ENGINE_ENABLE_ASSETS
            mesh_cache_.poll();
            point_cloud_cache_.poll();
#endif
            print_fps(delta_time);
        }

        void on_render() override
        {
            // Rendering handled by presentation backend.
        }

        void on_shutdown() override
        {
            ENGINE_INFO("=== Shutting down Geometry Viewer ===");
        }

    private:
        void setup_backend()
        {
#if ENGINE_ENABLE_RENDERING
            if (!backend_)
            {
                backend_ = std::dynamic_pointer_cast<engine::rendering::backend::opengl::OpenGLPresentationBackend>(
                    rendering_backend());
            }

            if (!backend_)
            {
                ENGINE_WARN("OpenGL backend unavailable; rendering disabled");
                return;
            }

            engine::rendering::ResearchBaselineOptions options{};
            options.shading_mode = engine::rendering::ResearchShadingMode::Forward;
            options.width = WINDOW_WIDTH;
            options.height = WINDOW_HEIGHT;
            baseline_resources_ = engine::rendering::configure_research_baseline(backend_->frame_graph(), options);
            backend_->frame_graph().compile();
#endif
        }

        void setup_procedural_assets()
        {
#if ENGINE_ENABLE_RENDERING
            auto cube_mesh = engine::geometry::make_unit_cube();
            mesh_storage_->store(std::string{kProceduralCubeId}, std::move(cube_mesh));

#    if ENGINE_ENABLE_ASSETS
            mesh_validator_registration_ =
                engine::assets::HandleValidatorRegistry::instance().register_mesh_validator(
                    [storage = mesh_storage_](const engine::assets::MeshHandle& handle) {
                        if (!storage || handle.empty())
                        {
                            return false;
                        }
                        return storage->contains(handle.id());
                    });
#    endif
#endif
        }

        void setup_scene()
        {
#if ENGINE_ENABLE_RENDERING && ENGINE_ENABLE_ASSETS
            attach_render_geometry(std::string{kProceduralCubeId},
                engine::rendering::components::RenderGeometry::from_mesh(
                    engine::assets::MeshHandle{std::string{kProceduralCubeId}}));
#endif
        }

        void setup_camera()
        {
#if ENGINE_ENABLE_RENDERING
            auto& registry = scene().registry();

            camera_entity_ = registry.create();
            registry.emplace<engine::scene::components::Name>(camera_entity_).value = "ViewerCamera";
            registry.emplace<engine::scene::components::Hierarchy>(camera_entity_);
            registry.emplace<engine::scene::components::LocalTransform>(camera_entity_);
            registry.emplace<engine::scene::components::WorldTransform>(camera_entity_).value =
                engine::math::Transform<float>::Identity();

            auto& camera = registry.emplace<engine::rendering::Camera>(camera_entity_);
            const float aspect_ratio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
            camera.set_perspective(1.047f, aspect_ratio, 0.1f, 100.0f);
            update_camera_position(camera);
#endif
        }

        void process_events()
        {
            auto& queue = window().event_queue();
            engine::platform::Event event;
            while (queue.poll(event))
            {
                if (event.type == engine::platform::EventType::FileDrop)
                {
                    if (const auto* payload = std::get_if<engine::platform::FileDropEvent>(&event.payload))
                    {
                        handle_file_drop(payload->paths);
                    }
                }
            }
        }

        void handle_file_drop(const std::vector<std::filesystem::path>& paths)
        {
            for (const auto& path : paths)
            {
                load_geometry_from_path(path);
            }
        }

        void load_geometry_from_path(const std::filesystem::path& raw_path)
        {
            std::filesystem::path path = raw_path;
            std::error_code ec{};
            path = std::filesystem::weakly_canonical(path, ec);
            if (ec)
            {
                ENGINE_WARN("Failed to canonicalize path '{}': {}", raw_path.string(), ec.message());
                path = raw_path;
            }

            auto detection = engine::io::detect_geometry_file(path);
            if (!detection)
            {
                ENGINE_ERROR("Unable to detect geometry type for '{}': {}", path.string(), detection.error().message());
                return;
            }

            switch (detection->kind)
            {
            case engine::io::GeometryKind::mesh:
                load_mesh_asset(path, detection->mesh_format);
                break;
            case engine::io::GeometryKind::point_cloud:
                load_point_cloud_asset(path, detection->point_cloud_format);
                break;
            default:
                ENGINE_WARN("Unsupported geometry kind for '{}'.", path.string());
                break;
            }
        }

        void load_mesh_asset(const std::filesystem::path& path, engine::io::MeshFileFormat hint)
        {
#if ENGINE_ENABLE_ASSETS
            try
            {
                auto descriptor = engine::assets::MeshAssetDescriptor::from_file(path, hint);
                const auto& asset = mesh_cache_.load(descriptor);
                auto surface_mesh = engine::geometry::mesh::build_surface_mesh_from_halfedge(asset.mesh.interface);
                focus_camera_on_bounds(surface_mesh.bounds);
                attach_render_geometry(std::string{asset.descriptor.handle.id()},
                    engine::rendering::components::RenderGeometry::from_mesh(asset.descriptor.handle));
                ENGINE_INFO("Loaded mesh '{}'", path.string());
            }
            catch (const std::exception& ex)
            {
                ENGINE_ERROR("Failed to load mesh '{}': {}", path.string(), ex.what());
            }
#else
            ENGINE_WARN("Mesh assets are not available in this build.");
            (void)path;
            (void)hint;
#endif
        }

        void load_point_cloud_asset(const std::filesystem::path& path, engine::io::PointCloudFileFormat hint)
        {
#if ENGINE_ENABLE_ASSETS
            try
            {
                auto descriptor = engine::assets::PointCloudAssetDescriptor::from_file(path, hint);
                const auto& asset = point_cloud_cache_.load(descriptor);
                const auto positions = asset.point_cloud.interface.positions();
                focus_camera_on_bounds(engine::geometry::BoundingAabb(positions));
                attach_render_geometry(std::string{asset.descriptor.handle.id()},
                    engine::rendering::components::RenderGeometry::from_point_cloud(asset.descriptor.handle));
                ENGINE_INFO("Loaded point cloud '{}'", path.string());
            }
            catch (const std::exception& ex)
            {
                ENGINE_ERROR("Failed to load point cloud '{}': {}", path.string(), ex.what());
            }
#else
            ENGINE_WARN("Point cloud assets are not available in this build.");
            (void)path;
            (void)hint;
#endif
        }

        void attach_render_geometry(const std::string& identifier,
            engine::rendering::components::RenderGeometry geometry)
        {
#if ENGINE_ENABLE_RENDERING
            auto& registry = scene().registry();

            entt::entity entity{entt::null};
            if (const auto it = render_entities_.find(identifier); it == render_entities_.end())
            {
                entity = registry.create();
                render_entities_.emplace(identifier, entity);
                registry.emplace<engine::scene::components::Name>(entity).value = identifier;
                registry.emplace<engine::scene::components::Hierarchy>(entity);
                registry.emplace<engine::scene::components::LocalTransform>(entity);
                registry.emplace<engine::scene::components::WorldTransform>(entity).value =
                    engine::math::Transform<float>::Identity();
            }
            else
            {
                entity = it->second;
            }

            registry.emplace_or_replace<engine::rendering::components::RenderGeometry>(entity, std::move(geometry));
            engine::scene::systems::mark_transform_dirty(registry, entity);
#else
            (void)identifier;
            (void)geometry;
#endif
        }

        void focus_camera_on_bounds(const engine::geometry::Aabb& bounds)
        {
            const auto size = engine::geometry::Size(bounds);
            const float max_extent = std::max({size.x, size.y, size.z, 1.0f});
            camera_radius_ = std::clamp(max_extent * 1.5f, 1.0f, 50.0f);

            update_camera();
        }

        void handle_input()
        {
            auto& input_state = input();

            if (input_state.is_mouse_button_down(engine::platform::input::MouseButton::Left))
            {
                if (was_dragging_)
                {
                    const auto delta = input_state.cursor_delta();
                    camera_yaw_ += delta.x * CAMERA_ROTATE_SPEED;
                    camera_pitch_ -= delta.y * CAMERA_ROTATE_SPEED;
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

            auto scroll = input_state.scroll_delta();
            camera_radius_ -= scroll.y * CAMERA_ZOOM_SPEED;
            camera_radius_ = std::clamp(camera_radius_, 1.0f, 50.0f);

            if (input_state.was_key_pressed(engine::platform::input::Key::Escape))
            {
                quit();
            }
        }

        void update_camera()
        {
#if ENGINE_ENABLE_RENDERING
            auto& registry = scene().registry();
            if (registry.valid(camera_entity_) && registry.any_of<engine::rendering::Camera>(camera_entity_))
            {
                auto& camera = registry.get<engine::rendering::Camera>(camera_entity_);
                update_camera_position(camera);
            }
#endif
        }

        void update_camera_position(engine::rendering::Camera& camera)
        {
            const float cos_pitch = std::cos(camera_pitch_);
            const float sin_pitch = std::sin(camera_pitch_);
            const float cos_yaw = std::cos(camera_yaw_);
            const float sin_yaw = std::sin(camera_yaw_);

            const engine::math::vec3 camera_pos{
                camera_radius_ * cos_pitch * sin_yaw,
                camera_radius_ * sin_pitch,
                camera_radius_ * cos_pitch * cos_yaw};

            const engine::math::vec3 target{0.0f, 0.0f, 0.0f};
            const engine::math::vec3 up{0.0f, 1.0f, 0.0f};

            if (auto result = camera.look_at(camera_pos, target, up); !result)
            {
                ENGINE_WARN("Camera look_at failed: {}", result.error().message());
            }
        }

        void print_fps(double delta_time)
        {
            ++fps_frame_count_;
            fps_time_accumulator_ += delta_time;

            if (fps_time_accumulator_ >= 2.0)
            {
                const double fps = static_cast<double>(fps_frame_count_) / fps_time_accumulator_;
                ENGINE_DEBUG("FPS: {:.1f} (Camera: yaw={:.2f}, pitch={:.2f}, radius={:.2f})",
                    fps, camera_yaw_, camera_pitch_, camera_radius_);
                fps_frame_count_ = 0;
                fps_time_accumulator_ = 0.0;
            }
        }

        std::shared_ptr<ProceduralMeshStorage> mesh_storage_{std::make_shared<ProceduralMeshStorage>()};
        std::shared_ptr<void> mesh_validator_registration_{};
#if ENGINE_ENABLE_ASSETS
        engine::assets::MeshCache mesh_cache_{};
        engine::assets::PointCloudCache point_cloud_cache_{};
#endif
#if ENGINE_ENABLE_RENDERING
        std::shared_ptr<engine::rendering::backend::opengl::OpenGLPresentationBackend> backend_{};
        engine::rendering::ResearchBaselineResources baseline_resources_{};
        entt::entity camera_entity_{entt::null};
#endif
        std::unordered_map<std::string, entt::entity> render_entities_{};
        float camera_yaw_{0.0f};
        float camera_pitch_{0.3f};
        float camera_radius_{CAMERA_DISTANCE};
        bool was_dragging_{false};
        int fps_frame_count_{0};
        double fps_time_accumulator_{0.0};
    };
} // namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    engine::core::Log::init();
    ENGINE_INFO("=== Test Engine Geometry Viewer ===");

    try
    {
        GeometryViewerApp app;
        const int result = app.run();
        engine::core::Log::shutdown();
        return result;
    }
    catch (const std::exception& ex)
    {
        ENGINE_CRITICAL("Error: {}", ex.what());
        engine::core::Log::shutdown();
        return EXIT_FAILURE;
    }
}

