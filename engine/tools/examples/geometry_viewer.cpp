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
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>

#include "engine/assets/handles.hpp"
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
#include "engine/math/math.hpp"
#include "engine/math/transform.hpp"
#include "engine/platform/input/input_state.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/rendering/camera_controllers.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/runtime/api.hpp"
#include "engine/runtime/application.hpp"
#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/components/name.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems/transform_system.hpp"
#if ENGINE_ENABLE_RENDERING
#    include <imgui.h>
#    include "engine/tools/editor/runtime_panel_bridge.hpp"
#    include "engine/tools/imgui/panel_registry.hpp"
#    include "engine/tools/imgui_helpers.hpp"
#    if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
#        include "backends/imgui_impl_glfw.h"
#        include "backends/imgui_impl_opengl3.h"
#    endif
#endif

#ifndef ENGINE_PLATFORM_HAS_GLFW
#    define ENGINE_PLATFORM_HAS_GLFW 0
#endif

#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

namespace
{
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr float CAMERA_DISTANCE = 5.0f;
    constexpr float CAMERA_FOV = 1.047f; // ~60 degrees
    constexpr float CAMERA_NEAR_PLANE = 0.1f;
    constexpr float CAMERA_FAR_PLANE = 100.0f;
    constexpr float CAMERA_ROTATE_SPEED = 0.005f;
    constexpr float CAMERA_ZOOM_SPEED = 0.1f;
    constexpr float MIN_GUI_SCALE = 0.5f;
    constexpr float MAX_GUI_SCALE = 3.0f;
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
                  const bool opengl_available = (ENGINE_PLATFORM_HAS_GLFW != 0)
                      && (ENGINE_RENDERING_HAS_GLAD != 0);
                  opengl_supported_ = opengl_available;

                  if (!opengl_available)
                  {
                      config.window_backend = engine::platform::WindowBackend::Mock;
#if ENGINE_ENABLE_RENDERING
                      config.rendering.enable = false;
                      config.rendering.backend =
                          engine::runtime::ApplicationConfig::RenderingConfig::Backend::Mock;
                      config.rendering.backend_factory = {};
#endif
                      ENGINE_WARN(
                          "OpenGL presentation disabled — GLFW target or GLAD loader not available. "
                          "Viewer will run in headless mock mode. Install system X11/Xrandr headers "
                          "and reconfigure to enable full rendering.");
                      return config;
                  }

                  config.enable_diagnostics = true;
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

        ~GeometryViewerApp() override
        {
#if ENGINE_ENABLE_RENDERING
            destroy_imgui_context();
#endif
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

            setup_panels();
#endif

            ENGINE_INFO("Drag and drop mesh (.obj/.ply/.stl) or point cloud (.ply/.pcd/.xyz) files into the window.");
        }

        void on_update(double delta_time) override
        {
            process_events();
            handle_input();
#if ENGINE_ENABLE_ASSETS
            mesh_cache_.poll();
            point_cloud_cache_.poll();
#endif
            print_fps(delta_time);

#if ENGINE_ENABLE_RENDERING
            if (backend_ && panel_bridge_ && imgui_context_ != nullptr)
            {
                backend_->request_imgui_render(delta_time);
            }
#endif
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
        struct WindowExtent
        {
            std::uint32_t width{static_cast<std::uint32_t>(WINDOW_WIDTH)};
            std::uint32_t height{static_cast<std::uint32_t>(WINDOW_HEIGHT)};
        };

        [[nodiscard]] float current_window_width() const noexcept
        {
            return static_cast<float>(std::max<std::uint32_t>(window_extent_.width, 1U));
        }

        [[nodiscard]] float current_window_height() const noexcept
        {
            return static_cast<float>(std::max<std::uint32_t>(window_extent_.height, 1U));
        }

        [[nodiscard]] float current_aspect_ratio() const noexcept
        {
            const float height = current_window_height();
            if (height <= std::numeric_limits<float>::epsilon())
            {
                return 1.0f;
            }
            return current_window_width() / height;
        }

        void setup_backend()
        {
#if ENGINE_ENABLE_RENDERING
            if (!opengl_supported_)
            {
                ENGINE_WARN("Skipping OpenGL backend setup — viewer running without GLFW/GLAD support.");
                return;
            }
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
            options.width = window_extent_.width;
            options.height = window_extent_.height;
            baseline_resources_ = engine::rendering::configure_research_baseline(backend_->frame_graph(), options);
            backend_->frame_graph().compile();
            register_default_material();
#endif
        }

        void setup_procedural_assets()
        {
#if ENGINE_ENABLE_RENDERING
            auto cube_mesh = engine::geometry::make_unit_cube();
            mesh_storage_->store(std::string{kProceduralCubeId}, std::move(cube_mesh));

#    if ENGINE_ENABLE_ASSETS
            material_validator_registration_ =
                engine::assets::HandleValidatorRegistry::instance().register_material_validator(
                    [handle = default_material_](const engine::assets::MaterialHandle& candidate) {
                        return !candidate.empty() && candidate.id() == handle.id();
                    });
#    endif

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
            ENGINE_INFO("setup_scene() called");
#if ENGINE_ENABLE_RENDERING && ENGINE_ENABLE_ASSETS
            ENGINE_INFO("  ENGINE_ENABLE_RENDERING=1, ENGINE_ENABLE_ASSETS=1");
            attach_render_geometry(std::string{kProceduralCubeId},
                engine::rendering::components::RenderGeometry::from_mesh(
                    engine::assets::MeshHandle{std::string{kProceduralCubeId}}, default_material_));
            ENGINE_INFO("  Attached cube render geometry");
#else
            ENGINE_WARN("  ENGINE_ENABLE_RENDERING={}, ENGINE_ENABLE_ASSETS={}",
                       ENGINE_ENABLE_RENDERING, ENGINE_ENABLE_ASSETS);
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
            update_camera_projection();

            // Create trackball controller for intuitive rotation
            const engine::math::vec3 center{0.0f, 0.0f, 0.0f};
            trackball_controller_ = std::make_unique<engine::rendering::TrackballCameraController>(
                camera, center, CAMERA_DISTANCE);

            ENGINE_INFO("Camera setup with trackball controller - center=({}, {}, {}), distance={}",
                       center[0], center[1], center[2], CAMERA_DISTANCE);
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
                else if (event.type == engine::platform::EventType::Resized)
                {
                    if (const auto* payload = std::get_if<engine::platform::ResizeEvent>(&event.payload))
                    {
                        handle_resize_event(*payload);
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

            switch (detection.value().kind)
            {
            case engine::io::GeometryKind::mesh:
                load_mesh_asset(path, detection.value().mesh_format);
                break;
            case engine::io::GeometryKind::point_cloud:
                load_point_cloud_asset(path, detection.value().point_cloud_format);
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
                const std::string model_id = std::string{asset.descriptor.handle.id()};
                attach_render_geometry(model_id,
                    engine::rendering::components::RenderGeometry::from_mesh(
                        asset.descriptor.handle, default_material_));
                focus_camera_on_entity_bounds(model_id, surface_mesh.bounds);

                // Track this model for deletion (if not already tracked)
                if (std::find(loaded_models_order_.begin(), loaded_models_order_.end(), model_id) == loaded_models_order_.end())
                {
                    loaded_models_order_.push_back(model_id);
                }

                ENGINE_INFO("Loaded mesh '{}' (total models: {})", path.string(), loaded_models_order_.size());
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
                const auto local_bounds = engine::geometry::BoundingAabb(positions);

                const std::string model_id = std::string{asset.descriptor.handle.id()};
                attach_render_geometry(model_id,
                    engine::rendering::components::RenderGeometry::from_point_cloud(
                        asset.descriptor.handle, default_material_));
                focus_camera_on_entity_bounds(model_id, local_bounds);

                // Track this model for deletion (if not already tracked)
                if (std::find(loaded_models_order_.begin(), loaded_models_order_.end(), model_id) == loaded_models_order_.end())
                {
                    loaded_models_order_.push_back(model_id);
                }

                ENGINE_INFO("Loaded point cloud '{}' (total models: {})", path.string(), loaded_models_order_.size());
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

        void register_default_material()
        {
#if ENGINE_ENABLE_RENDERING
            if (!backend_)
            {
                return;
            }

            backend_->material_system().register_material(
                engine::rendering::MaterialSystem::MaterialRecord{default_material_, {}});
#endif
        }

        void focus_camera_on_bounds(const engine::geometry::Aabb& bounds)
        {
#if ENGINE_ENABLE_RENDERING
            if (!trackball_controller_)
            {
                return;
            }

            const auto center = engine::geometry::Center(bounds);
            const float distance = compute_focus_distance(bounds);
            ensure_camera_depth_range(bounds, distance);
            trackball_controller_->set_center(center);
            trackball_controller_->set_distance(distance);

            ENGINE_INFO("Focused camera on bounds - center=({}, {}, {}), distance={}",
                       center[0], center[1], center[2], distance);
#endif
        }

        void focus_camera_on_entity_bounds(const std::string& identifier,
            const engine::geometry::Aabb& local_bounds)
        {
#if ENGINE_ENABLE_RENDERING
            if (!trackball_controller_)
            {
                return;
            }

            const auto world_bounds = [&]() {
                if (const auto it = render_entities_.find(identifier); it != render_entities_.end())
                {
                    return compute_world_bounds(it->second, local_bounds);
                }
                return local_bounds;
            }();

            focus_camera_on_bounds(world_bounds);
#else
            (void)identifier;
            (void)local_bounds;
#endif
        }

        void handle_resize_event(const engine::platform::ResizeEvent& payload)
        {
            const std::uint32_t clamped_width = std::max<std::uint32_t>(payload.width, 1U);
            const std::uint32_t clamped_height = std::max<std::uint32_t>(payload.height, 1U);

            if (clamped_width == window_extent_.width && clamped_height == window_extent_.height)
            {
                return;
            }

            window_extent_.width = clamped_width;
            window_extent_.height = clamped_height;
            ENGINE_INFO("Window resized to {}x{}", window_extent_.width, window_extent_.height);
            update_camera_projection();
        }

        void update_camera_projection()
        {
#if ENGINE_ENABLE_RENDERING
            auto& registry = scene().registry();
            if (!registry.valid(camera_entity_) || !registry.any_of<engine::rendering::Camera>(camera_entity_))
            {
                return;
            }

            auto& camera = registry.get<engine::rendering::Camera>(camera_entity_);
            camera.set_perspective(CAMERA_FOV, current_aspect_ratio(), CAMERA_NEAR_PLANE, camera_far_plane_);
#else
            (void)current_aspect_ratio();
#endif
        }

        void handle_input()
        {
#if ENGINE_ENABLE_RENDERING
            if (!trackball_controller_)
            {
                return;
            }

            auto& input_state = input();
            const auto imgui_capture = query_imgui_capture_state();
            const bool allow_mouse_input = !imgui_capture.mouse;
            const bool allow_keyboard_input = !imgui_capture.keyboard;

            const bool left_mouse_down =
                input_state.is_mouse_button_down(engine::platform::input::MouseButton::Left);
            if (allow_mouse_input && left_mouse_down)
            {
                const auto cursor = input_state.cursor_position();
                const float width = current_window_width();
                const float height = current_window_height();
                const float min_dimension = std::min(width, height);
                const float half = std::max(min_dimension * 0.5f, 1.0f);
                const engine::math::vec2 abs_cursor{
                    (cursor.x - width * 0.5f) / half,
                    (height * 0.5f - cursor.y) / half
                };
                const engine::math::vec2 projected_center =
                    project_world_point_to_trackball_coords(trackball_controller_->center());
                const engine::math::vec2 relative_cursor = abs_cursor - projected_center;

                if (was_dragging_)
                {
                    const engine::math::vec2 prev_abs = last_cursor_pos_;
                    const engine::math::vec2 prev_rel = prev_abs - projected_center;
                    const engine::math::vec2 curr_rel = relative_cursor;
                    trackball_controller_->rotate_from(prev_rel, curr_rel);
                    last_cursor_pos_ = abs_cursor;
                }
                else
                {
                    was_dragging_ = true;
                    last_cursor_pos_ = abs_cursor;
                }
            }
            else
            {
                was_dragging_ = false;
            }

            if (allow_mouse_input)
            {
                auto scroll = input_state.scroll_delta();
                if (scroll.y != 0.0f)
                {
                    trackball_controller_->zoom(-scroll.y * CAMERA_ZOOM_SPEED);
                }
            }

            if (allow_keyboard_input && input_state.was_key_pressed(engine::platform::input::Key::T))
            {
                toggle_cube();
            }

            if (allow_keyboard_input && input_state.was_key_pressed(engine::platform::input::Key::Delete))
            {
                delete_last_model();
            }

            if (input_state.was_key_pressed(engine::platform::input::Key::G) && panel_bridge_)
            {
                panels_visible_ = !panels_visible_;
            }

            if (allow_keyboard_input && input_state.was_key_pressed(engine::platform::input::Key::Escape))
            {
                quit();
            }
#endif
        }

        void toggle_cube()
        {
#if ENGINE_ENABLE_RENDERING
            cube_visible_ = !cube_visible_;

            auto& registry = scene().registry();
            const auto cube_id = std::string{kProceduralCubeId};

            if (const auto it = render_entities_.find(cube_id); it != render_entities_.end())
            {
                const auto entity = it->second;
                if (cube_visible_)
                {
                    // Re-attach the cube geometry
                    attach_render_geometry(cube_id,
                        engine::rendering::components::RenderGeometry::from_mesh(
                            engine::assets::MeshHandle{cube_id}, default_material_));
                    ENGINE_INFO("Cube visible");
                }
                else
                {
                    // Remove the render geometry component (keeps entity but makes it invisible)
                    if (registry.valid(entity) && registry.any_of<engine::rendering::components::RenderGeometry>(entity))
                    {
                        registry.remove<engine::rendering::components::RenderGeometry>(entity);
                        ENGINE_INFO("Cube hidden");
                    }
                }
            }
#endif
        }

        void delete_last_model()
        {
#if ENGINE_ENABLE_RENDERING
            if (loaded_models_order_.empty())
            {
                ENGINE_INFO("No models to delete");
                return;
            }

            // Get the last loaded model (LIFO - stack order)
            const std::string model_id = loaded_models_order_.back();
            loaded_models_order_.pop_back();

            auto& registry = scene().registry();
            if (const auto it = render_entities_.find(model_id); it != render_entities_.end())
            {
                const auto entity = it->second;
                if (registry.valid(entity))
                {
                    registry.destroy(entity);
                }
                render_entities_.erase(it);
                ENGINE_INFO("Deleted model: {}", model_id);
            }

            // If no models left, refocus on cube if visible
            if (loaded_models_order_.empty() && cube_visible_)
            {
                if (const auto* mesh = mesh_storage_->find(kProceduralCubeId))
                {
                    focus_camera_on_bounds(mesh->bounds);
                }
            }
#endif
        }

#if ENGINE_ENABLE_RENDERING
        void setup_panels()
        {
            if (!opengl_supported_)
            {
                return;
            }

            if (!imgui_context_)
            {
                imgui_context_ = ImGui::CreateContext();
                apply_imgui_scale();
            }

            // Register ImGui context with the presentation backend so it can render UI into the GL context.
            if (backend_)
            {
                backend_->set_imgui_context_for_rendering(imgui_context_);
                backend_->set_imgui_render_callback([this](double delta_time) {
                    render_panels(delta_time);
                });
            }

            if (panel_bridge_)
            {
                return;
            }

            using engine::tools::editor::RuntimePanelBridge;

            RuntimePanelBridge::HierarchyPanelHooks hierarchy_hooks{};
            hierarchy_hooks.scene_provider = [this]() -> engine::scene::Scene* {
                return &scene();
            };

            RuntimePanelBridge::AssetPanelHooks asset_hooks{};
#    if ENGINE_ENABLE_ASSETS
            asset_hooks.row_provider = [this]() {
                engine::tools::editor::AssetRegistryFacade facade{};
                facade.mesh_cache = &mesh_cache_;
                facade.point_cloud_cache = &point_cloud_cache_;
                return engine::tools::editor::collect_asset_rows(facade);
            };
#    endif

            panel_bridge_ = std::make_unique<RuntimePanelBridge>(
                panel_registry_,
                [this]() -> const engine::runtime::RuntimeDiagnostics& {
                    return runtime_host().diagnostics();
                },
                [this]() -> const engine::scene::validation::HierarchyValidationReport* {
                    return &runtime_host().diagnostics().scene_validation;
                },
                RuntimePanelBridge::Renderers{},
                std::move(hierarchy_hooks),
                std::move(asset_hooks),
                RuntimePanelBridge::PerformancePanelHooks{},
                RuntimePanelBridge::TelemetryPanelHooks{});

            refresh_widget_toggles();
        }

        void render_panels(double delta_time)
        {
            if (!panel_bridge_ || imgui_context_ == nullptr)
            {
                return;
            }

            ImGuiContext* previous_context = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(imgui_context_);
            engine::tools::imgui::begin_frame();
            render_widget_menu();
            if (panels_visible_)
            {
                render_active_widgets(delta_time);
            }
            engine::tools::imgui::end_frame();
            ImGui::SetCurrentContext(previous_context);
        }

        struct ImGuiCaptureState
        {
            bool mouse{false};
            bool keyboard{false};
        };

        [[nodiscard]] ImGuiCaptureState query_imgui_capture_state() const
        {
            ImGuiCaptureState state{};
            if (!panel_bridge_ || imgui_context_ == nullptr)
            {
                return state;
            }

            ImGuiContext* previous_context = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(imgui_context_);
            const ImGuiIO& io = ImGui::GetIO();
            state.mouse = io.WantCaptureMouse;
            state.keyboard = io.WantCaptureKeyboard;
            ImGui::SetCurrentContext(previous_context);
            return state;
        }

        void destroy_imgui_context()
        {
            panel_bridge_.reset();
            widget_toggles_.clear();
            if (backend_)
            {
                backend_->set_imgui_render_callback({});
                backend_->set_imgui_context_for_rendering(nullptr);
            }
            if (imgui_context_ != nullptr)
            {
                // Ensure the ImGui context we created is current so backend shutdown reads correct IO data.
                ImGuiContext* previous_context = ImGui::GetCurrentContext();
                ImGui::SetCurrentContext(imgui_context_);

                // Only shutdown backends if they were initialized. Check IO backend userdata to be safe.
                ImGuiIO& io = ImGui::GetIO();

                // Log backend userdata pointers to help debug shutdown ordering issues.
                ENGINE_INFO("ImGui shutdown: BackendPlatformUserData={}, BackendRendererUserData={}",
                            (void*)io.BackendPlatformUserData, (void*)io.BackendRendererUserData);
#if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
                // If a renderer backend is registered, shut it down first as per examples.
                if (io.BackendRendererUserData != nullptr)
                {
                    ENGINE_INFO("Calling ImGui_ImplOpenGL3_Shutdown()...");
                    ImGui_ImplOpenGL3_Shutdown();
                    ENGINE_INFO("After ImGui_ImplOpenGL3_Shutdown: BackendRendererUserData={}", (void*)io.BackendRendererUserData);
                }

                // Then shutdown the platform backend
                if (io.BackendPlatformUserData != nullptr)
                {
                    ENGINE_INFO("Calling ImGui_ImplGlfw_Shutdown()...");
                    ImGui_ImplGlfw_Shutdown();
                    ENGINE_INFO("After ImGui_ImplGlfw_Shutdown: BackendPlatformUserData={}", (void*)io.BackendPlatformUserData);
                }

                // If the platform userdata is still set, log and clear to avoid assertion during ImGui::Shutdown.
                if (io.BackendPlatformUserData != nullptr || io.BackendRendererUserData != nullptr)
                {
                    ENGINE_WARN("ImGui backend userdata not cleared by shutdown functions; forcing cleanup (platform={}, renderer={})",
                                (void*)io.BackendPlatformUserData, (void*)io.BackendRendererUserData);
                    io.BackendPlatformUserData = nullptr;
                    io.BackendRendererUserData = nullptr;
                    io.BackendPlatformName = nullptr;
                    // Clear backend flags related to platform/renderer capabilities to be safe.
                    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_HasMouseHoveredViewport);
                }
#endif
                // Destroy the context we created for panels
                ImGui::DestroyContext(imgui_context_);
                ImGui::SetCurrentContext(previous_context);
                imgui_context_ = nullptr;
                ENGINE_INFO("ImGui context destroyed cleanly");
            }
        }

        void refresh_widget_toggles()
        {
            widget_toggles_.clear();
            if (!panel_bridge_)
            {
                return;
            }

            const auto identifiers = panel_bridge_->panel_identifiers();
            widget_toggles_.reserve(identifiers.size());
            for (const auto& identifier : identifiers)
            {
                WidgetToggle widget{};
                widget.identifier = identifier;
                widget.label = std::string(widget_label_for_identifier(widget.identifier));
                widget.visible = widget_enabled_by_default(widget.identifier);
                widget_toggles_.push_back(std::move(widget));
            }
        }

        void render_widget_menu()
        {
            ImGui::SetNextWindowBgAlpha(0.9f);
            ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);
            const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
            if (!ImGui::Begin("Geometry Viewer Menu", nullptr, window_flags))
            {
                ImGui::End();
                return;
            }

            ImGui::Text("Window: %.0f x %.0f", current_window_width(), current_window_height());
            bool show_panels = panels_visible_;
            if (ImGui::Checkbox("Show diagnostics panels", &show_panels))
            {
                panels_visible_ = show_panels;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Press 'G' to toggle");

            if (!panels_visible_)
            {
                ImGui::TextDisabled("Diagnostics panels are hidden; enable them above to render overlays.");
            }

            ImGui::Separator();

            if (widget_toggles_.empty())
            {
                ImGui::TextUnformatted("No diagnostics widgets were registered.");
            }
            else
            {
                if (ImGui::Button("Show All"))
                {
                    set_all_widget_visibility(true);
                    panels_visible_ = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Hide All"))
                {
                    set_all_widget_visibility(false);
                }

                ImGui::Separator();

                for (auto& widget : widget_toggles_)
                {
                    bool visible = widget.visible;
                    if (ImGui::Checkbox(widget.label.c_str(), &visible))
                    {
                        widget.visible = visible;
                    }
                }

                const bool any_visible = std::any_of(
                    widget_toggles_.begin(),
                    widget_toggles_.end(),
                    [](const WidgetToggle& widget) { return widget.visible; }
                );

                if (!any_visible)
                {
                    ImGui::Spacing();
                    ImGui::TextDisabled("No widgets are active. Enable at least one above to show it.");
                }
            }

            ImGui::Separator();
            if (ImGui::SliderFloat("UI Scale",
                    &gui_scale_,
                    MIN_GUI_SCALE,
                    MAX_GUI_SCALE,
                    "%.2fx",
                    ImGuiSliderFlags_AlwaysClamp))
            {
                gui_scale_ = std::clamp(gui_scale_, MIN_GUI_SCALE, MAX_GUI_SCALE);
                apply_imgui_scale();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Applies to menu and panels");

            ImGui::End();
        }

        void render_active_widgets(double delta_time)
        {
            if (!panel_bridge_ || !panels_visible_)
            {
                return;
            }

            std::vector<std::string_view> active_identifiers;
            active_identifiers.reserve(widget_toggles_.size());
            for (const auto& widget : widget_toggles_)
            {
                if (widget.visible)
                {
                    active_identifiers.push_back(widget.identifier);
                }
            }

            if (active_identifiers.empty())
            {
                return;
            }

            panel_bridge_->render_panels(delta_time, active_identifiers);
        }

        static std::string_view widget_label_for_identifier(std::string_view identifier)
        {
            using namespace std::string_view_literals;
            if (identifier == "editor.scene_hierarchy")
            {
                return "Scene Hierarchy"sv;
            }
            if (identifier == "editor.asset_browser")
            {
                return "Asset Browser"sv;
            }
            if (identifier == "runtime.diagnostics")
            {
                return "Runtime Diagnostics"sv;
            }
            if (identifier == "runtime.performance_metrics")
            {
                return "Performance Metrics"sv;
            }
            if (identifier == "runtime.profiler")
            {
                return "Profiler"sv;
            }
            if (identifier == "runtime.telemetry")
            {
                return "Telemetry"sv;
            }
            if (identifier == "runtime.scene_validation")
            {
                return "Scene Validation"sv;
            }
            return identifier;
        }

        static bool widget_enabled_by_default(std::string_view identifier)
        {
            return identifier == "editor.scene_hierarchy"
                || identifier == "editor.asset_browser"
                || identifier == "runtime.diagnostics";
        }

        void set_all_widget_visibility(bool visible)
        {
            for (auto& widget : widget_toggles_)
            {
                widget.visible = visible;
            }
        }

        void apply_imgui_scale()
        {
            if (!imgui_context_)
            {
                return;
            }

            gui_scale_ = std::clamp(gui_scale_, MIN_GUI_SCALE, MAX_GUI_SCALE);

            ImGuiContext* previous_context = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(imgui_context_);
            ImGuiIO& io = ImGui::GetIO();
            ImGuiStyle& style = ImGui::GetStyle();
            const float previous_scale = std::max(last_applied_gui_scale_, 0.01f);
            const float ratio = gui_scale_ / previous_scale;
            if (std::abs(ratio - 1.0f) > std::numeric_limits<float>::epsilon())
            {
                style.ScaleAllSizes(ratio);
            }
            last_applied_gui_scale_ = gui_scale_;
            io.FontGlobalScale = gui_scale_;
            ImGui::SetCurrentContext(previous_context);
        }
#endif

        void print_fps(double delta_time)
        {
            ++fps_frame_count_;
            fps_time_accumulator_ += delta_time;

            if (fps_time_accumulator_ >= 2.0)
            {
                const double fps = static_cast<double>(fps_frame_count_) / fps_time_accumulator_;
                ENGINE_DEBUG("FPS: {:.1f}", fps);
                fps_frame_count_ = 0;
                fps_time_accumulator_ = 0.0;
            }
        }

        // Project a world-space point to normalized screen coordinates centered at 0: (-1..1)
        [[nodiscard]] engine::math::vec2 project_world_point_to_trackball_coords(const engine::math::vec3& world_point) const
        {
#if ENGINE_ENABLE_RENDERING
            if (!trackball_controller_)
            {
                return engine::math::vec2{0.0f, 0.0f};
            }
            auto& registry = scene().registry();
            if (!registry.valid(camera_entity_) || !registry.any_of<engine::rendering::Camera>(camera_entity_))
            {
                return engine::math::vec2{0.0f, 0.0f};
            }
            const auto& cam = registry.get<engine::rendering::Camera>(camera_entity_);
            const auto vp = cam.view_projection();

            // Convert world_point to clip space
            const engine::math::vec4 clip = vp * engine::math::vec4{world_point[0], world_point[1], world_point[2], 1.0f};
            if (clip[3] == 0.0f)
            {
                return engine::math::vec2{0.0f, 0.0f};
            }
            const engine::math::vec3 ndc = engine::math::vec3{clip[0] / clip[3], clip[1] / clip[3], clip[2] / clip[3]};

            // Convert NDC (-1..1) to pixel coordinates
            const float width = current_window_width();
            const float height = current_window_height();
            const float px = (ndc[0] * 0.5f + 0.5f) * width;
            const float py = (1.0f - (ndc[1] * 0.5f + 0.5f)) * height;

            // Convert pixel coords to normalized trackball coords centered at the projected center.
            // We return coordinates in [-1,1] relative to the smaller window dimension.
            const float min_dimension = std::min(width, height);
            const float half = std::max(min_dimension * 0.5f, 1.0f);
            const float cx = (px - width * 0.5f) / half;
            const float cy = (height * 0.5f - py) / half;
            return engine::math::vec2{cx, cy};
#else
            (void)world_point;
            return engine::math::vec2{0.0f, 0.0f};
#endif
        }

        [[nodiscard]] engine::geometry::Aabb compute_world_bounds(entt::entity entity,
            const engine::geometry::Aabb& local_bounds) const
        {
#if ENGINE_ENABLE_RENDERING
            auto& registry = scene().registry();
            if (!registry.valid(entity)
                || !registry.any_of<engine::scene::components::WorldTransform>(entity))
            {
                return local_bounds;
            }

            const auto& world_transform = registry.get<engine::scene::components::WorldTransform>(entity).value;
            const auto corners = engine::geometry::GetCorners(local_bounds);

            engine::geometry::Aabb world_bounds{};
            const auto first_point = engine::math::transform_point(world_transform, corners.front());
            world_bounds.min = first_point;
            world_bounds.max = first_point;
            for (std::size_t i = 1; i < corners.size(); ++i)
            {
                const auto point = engine::math::transform_point(world_transform, corners[i]);
                engine::geometry::Merge(world_bounds, point);
            }
            return world_bounds;
#else
            (void)entity;
            return local_bounds;
#endif
        }

        void ensure_camera_depth_range(const engine::geometry::Aabb& bounds, float focus_distance)
        {
#if ENGINE_ENABLE_RENDERING
            const float radius = std::max(engine::math::length(engine::geometry::Size(bounds)) * 0.5f, 0.0f);
            constexpr float kRadiusPadding = 1.05f;
            constexpr float kDistancePadding = 1.0f;
            const float far_from_focus = focus_distance + radius * kRadiusPadding + kDistancePadding;
            const float required_far = std::max(far_from_focus, CAMERA_FAR_PLANE);
            if (std::abs(required_far - camera_far_plane_) > 1e-3f)
            {
                camera_far_plane_ = required_far;
                update_camera_projection();
            }
#else
            (void)bounds;
            (void)focus_distance;
#endif
        }

        [[nodiscard]] float compute_focus_distance(const engine::geometry::Aabb& bounds) const
        {
            const auto size = engine::geometry::Size(bounds);
            const engine::math::vec3 half_size = size * 0.5f;

            const float aspect = std::max(current_aspect_ratio(), 1e-3f);
            const float vertical_fov = CAMERA_FOV;
            const float horizontal_fov = 2.0f * std::atan(std::tan(vertical_fov * 0.5f) * aspect);

            const auto axis_distance = [](float half_extent, float fov) {
                if (half_extent <= std::numeric_limits<float>::epsilon())
                {
                    return 0.0f;
                }
                const float half_fov = std::max(fov * 0.5f, 1e-3f);
                return half_extent / std::tan(half_fov);
            };

            float distance = std::max(axis_distance(half_size[1], vertical_fov),
                axis_distance(half_size[0], horizontal_fov));

            const float diagonal_radius = std::max(engine::math::length(size) * 0.5f,
                std::max(half_size[2], 0.0f));
            distance = std::max(distance, diagonal_radius);

            constexpr float kMargin = 1.15f;
            constexpr float kMinDistance = 1.0f;
            distance = std::max(distance * kMargin, kMinDistance);
            return distance;
        }

        WindowExtent window_extent_{};
        std::shared_ptr<ProceduralMeshStorage> mesh_storage_{std::make_shared<ProceduralMeshStorage>()};
        std::shared_ptr<void> mesh_validator_registration_{};
        std::shared_ptr<void> material_validator_registration_{};
#if ENGINE_ENABLE_ASSETS
        engine::assets::MeshCache mesh_cache_{};
        engine::assets::PointCloudCache point_cloud_cache_{};
#endif
#if ENGINE_ENABLE_RENDERING
        std::shared_ptr<engine::rendering::backend::opengl::OpenGLPresentationBackend> backend_{};
        engine::rendering::ResearchBaselineResources baseline_resources_{};
        entt::entity camera_entity_{entt::null};
        std::unique_ptr<engine::rendering::TrackballCameraController> trackball_controller_{};
        float camera_far_plane_{CAMERA_FAR_PLANE};
#endif
        std::unordered_map<std::string, entt::entity> render_entities_{};
        std::vector<std::string> loaded_models_order_{}; // Track loaded models in creation order (excluding cube)
        bool cube_visible_{true}; // Toggle state for default cube
        engine::assets::MaterialHandle default_material_{std::string{"geometry_viewer.material.default"}};
        bool opengl_supported_{(ENGINE_PLATFORM_HAS_GLFW != 0) && (ENGINE_RENDERING_HAS_GLAD != 0)};
#if ENGINE_ENABLE_RENDERING
        engine::tools::imgui::PanelRegistry panel_registry_{};
        std::unique_ptr<engine::tools::editor::RuntimePanelBridge> panel_bridge_{};
        bool panels_visible_{true};
        ImGuiContext* imgui_context_{nullptr};
        struct WidgetToggle
        {
            std::string identifier;
            std::string label;
            bool visible{false};
        };
        std::vector<WidgetToggle> widget_toggles_{};
        float gui_scale_{1.25f};
        float last_applied_gui_scale_{1.0f};
#endif
        bool was_dragging_{false};
        engine::math::vec2 last_cursor_pos_{0.0f, 0.0f};
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
        ENGINE_INFO("Creating GeometryViewerApp...");
        GeometryViewerApp app;
        ENGINE_INFO("Running app...");
        const int result = app.run();
        ENGINE_INFO("App finished with result: {}", result);
        engine::core::Log::shutdown();
        return result;
    }
    catch (const std::exception& ex)
    {
        ENGINE_CRITICAL("Error: {}", ex.what());
        engine::core::Log::shutdown();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        ENGINE_CRITICAL("Unknown error occurred");
        engine::core::Log::shutdown();
        return EXIT_FAILURE;
    }
}

