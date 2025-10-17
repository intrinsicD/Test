#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine/animation/api.hpp"
#include "engine/core/plugin/isubsystem_interface.hpp"
#include "engine/compute/api.hpp"
#include "engine/geometry/api.hpp"
#include "engine/math/math.hpp"
#include "engine/physics/api.hpp"
#include "engine/runtime/subsystem_registry.hpp"

#if ENGINE_ENABLE_RENDERING
#    include "engine/rendering/components.hpp"
#    include "engine/rendering/gpu_scheduler.hpp"
#    include "engine/rendering/resources/resource_provider.hpp"
#endif

#if ENGINE_ENABLE_RENDERING
namespace engine::rendering
{
    class FrameGraph;
    class ForwardPipeline;
    class MaterialSystem;
    class RenderResourceProvider;
    class CommandEncoderProvider;
} // namespace engine::rendering

namespace engine::rendering::resources
{
    class IGpuResourceProvider;
} // namespace engine::rendering::resources
#endif

#if defined(_WIN32)
#  if defined(ENGINE_RUNTIME_EXPORTS)
#    define ENGINE_RUNTIME_API __declspec(dllexport)
#  else
#    define ENGINE_RUNTIME_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_RUNTIME_API
#endif

namespace engine::runtime {

[[nodiscard]] ENGINE_RUNTIME_API std::string_view module_name() noexcept;
[[nodiscard]] ENGINE_RUNTIME_API std::size_t module_count() noexcept;
[[nodiscard]] ENGINE_RUNTIME_API std::string_view module_name_at(std::size_t index) noexcept;

struct ENGINE_RUNTIME_API runtime_frame_state {
    double simulation_time{0.0};
    animation::AnimationRigPose pose{};
    geometry::Aabb bounds{};
    std::vector<math::vec3> body_positions{};
    compute::ExecutionReport dispatch_report{};
    struct ENGINE_RUNTIME_API scene_node_state {
        std::string name{};
        math::Transform<float> transform{};
    };
    std::vector<scene_node_state> scene_nodes{};
};

struct ENGINE_RUNTIME_API RuntimeHostDependencies {
    animation::AnimationController controller{
        animation::make_linear_controller(animation::make_default_clip())};
    geometry::SurfaceMesh mesh{geometry::make_unit_quad()};
    physics::PhysicsWorld world{};
    std::string scene_name{"runtime.scene"};
    std::vector<std::shared_ptr<core::plugin::ISubsystemInterface>> subsystem_plugins{};
    std::shared_ptr<SubsystemRegistry> subsystem_registry{};
    std::vector<std::string> enabled_subsystems{};
#if ENGINE_ENABLE_RENDERING
    rendering::components::RenderGeometry render_geometry{};
    std::string renderable_name{"runtime.renderable"};
#endif
};

class ENGINE_RUNTIME_API RuntimeHost {
public:
    RuntimeHost();
    explicit RuntimeHost(RuntimeHostDependencies dependencies);
    RuntimeHost(RuntimeHost&&) noexcept;
    RuntimeHost& operator=(RuntimeHost&&) noexcept;
    RuntimeHost(const RuntimeHost&) = delete;
    RuntimeHost& operator=(const RuntimeHost&) = delete;
    ~RuntimeHost();

    void initialize();
    void shutdown() noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;
    runtime_frame_state tick(double dt);
    [[nodiscard]] const geometry::SurfaceMesh& current_mesh() const;
    [[nodiscard]] const animation::AnimationRigPose& current_pose() const;
    [[nodiscard]] const std::vector<math::vec3>& body_positions() const;
    [[nodiscard]] const std::vector<std::string>& joint_names() const;
    [[nodiscard]] const compute::ExecutionReport& last_dispatch_report() const;
    [[nodiscard]] const std::vector<runtime_frame_state::scene_node_state>& scene_nodes() const;
    [[nodiscard]] double simulation_time() const noexcept;
    [[nodiscard]] std::span<const std::string_view> subsystem_names() const noexcept;

    void configure(RuntimeHostDependencies dependencies);

#if ENGINE_ENABLE_RENDERING
    struct RenderSubmissionContext {
        rendering::RenderResourceProvider& resources;
        rendering::MaterialSystem& materials;
        rendering::resources::IGpuResourceProvider& device_resources;
        rendering::IGpuScheduler& scheduler;
        rendering::CommandEncoderProvider& encoders;
        rendering::FrameGraph& frame_graph;
        rendering::ForwardPipeline* pipeline{nullptr};
    };

    void submit_render_graph(RenderSubmissionContext& context);
#endif

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

ENGINE_RUNTIME_API void initialize();
ENGINE_RUNTIME_API void shutdown();
ENGINE_RUNTIME_API void configure(RuntimeHostDependencies dependencies);
ENGINE_RUNTIME_API void configure_with_default_subsystems();
ENGINE_RUNTIME_API void configure_with_default_subsystems(std::span<const std::string_view> enabled_subsystems);
ENGINE_RUNTIME_API runtime_frame_state tick(double dt);
[[nodiscard]] ENGINE_RUNTIME_API const geometry::SurfaceMesh& current_mesh();
[[nodiscard]] ENGINE_RUNTIME_API bool is_initialized() noexcept;
[[nodiscard]] ENGINE_RUNTIME_API const animation::AnimationRigPose& current_pose();
[[nodiscard]] ENGINE_RUNTIME_API const std::vector<math::vec3>& body_positions();
[[nodiscard]] ENGINE_RUNTIME_API const std::vector<std::string>& joint_names();
[[nodiscard]] ENGINE_RUNTIME_API const compute::ExecutionReport& last_dispatch_report();
[[nodiscard]] ENGINE_RUNTIME_API const std::vector<runtime_frame_state::scene_node_state>& scene_nodes();
[[nodiscard]] ENGINE_RUNTIME_API double simulation_time() noexcept;
[[nodiscard]] ENGINE_RUNTIME_API std::vector<std::string> default_subsystem_names();

#if ENGINE_ENABLE_RENDERING
ENGINE_RUNTIME_API void submit_render_graph(RuntimeHost::RenderSubmissionContext& context);
#endif

}  // namespace engine::runtime

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_name() noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_module_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_at(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_with_default_modules() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_with_modules(
    const char* const* module_names,
    std::size_t count) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_initialize() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_shutdown() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_tick(double dt) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_body_count() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_body_position(std::size_t index, float* out_position) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_joint_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_joint_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_joint_translation(std::size_t index, float* out_translation) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_mesh_bounds(float* out_min, float* out_max) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_dispatch_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_dispatch_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_dispatch_duration(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_scene_node_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_scene_node_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_scene_node_transform(
    std::size_t index,
    float* out_scale,
    float* out_rotation,
    float* out_translation) noexcept;

