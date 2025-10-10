#include "engine/runtime/api.hpp"

#include <array>
#include <string>
#include <utility>

#include "engine/assets/api.hpp"
#include "engine/compute/cuda/api.hpp"
#include "engine/core/api.hpp"
#include "engine/io/api.hpp"
#include "engine/platform/api.hpp"
#include "engine/rendering/api.hpp"
#include "engine/scene/api.hpp"
#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"

namespace {

struct runtime_state {
    bool initialized{false};
    double simulation_time{0.0};
    engine::animation::AnimationController controller{engine::animation::make_linear_controller(
        engine::animation::make_default_clip())};
    engine::animation::AnimationRigPose pose{engine::animation::evaluate_controller(controller)};
    engine::geometry::SurfaceMesh mesh{engine::geometry::make_unit_quad()};
    engine::physics::PhysicsWorld world{};
    engine::compute::KernelDispatcher dispatcher{};
    engine::compute::ExecutionReport last_report{};
    std::vector<engine::math::vec3> body_positions{};
    std::vector<std::string> joint_names{};
    engine::scene::Scene scene{};
    std::vector<engine::scene::Entity> joint_entities{};
    std::vector<engine::runtime::runtime_frame_state::scene_node_state> scene_nodes{};

    runtime_state() {
        engine::geometry::recompute_vertex_normals(mesh);
        engine::geometry::update_bounds(mesh);
    }
};

runtime_state& global_state() {
    static runtime_state state;
    return state;
}

const std::array<std::string_view, 11>& module_names() noexcept {
    static const std::array<std::string_view, 11> names = {
        engine::animation::module_name(),
        engine::assets::module_name(),
        engine::compute::module_name(),
        engine::compute::cuda::module_name(),
        engine::core::module_name(),
        engine::geometry::module_name(),
        engine::io::module_name(),
        engine::physics::module_name(),
        engine::platform::module_name(),
        engine::rendering::module_name(),
        engine::scene::module_name(),
    };
    return names;
}

// Rebuild the runtime scene hierarchy so that a scene entity exists for every joint in the pose.
void rebuild_scene_entities(runtime_state& state) {
    state.scene = engine::scene::Scene{"runtime.scene"};
    state.joint_entities.clear();
    state.joint_entities.reserve(state.pose.joints.size());

    for (const auto& entry : state.pose.joints) {
        auto entity = state.scene.create_entity();
        auto& name_component = entity.emplace<engine::scene::components::Name>();
        name_component.value = entry.first;
        entity.emplace<engine::scene::components::LocalTransform>();
        entity.emplace<engine::scene::components::WorldTransform>();
        entity.emplace<engine::scene::components::Hierarchy>();
        engine::scene::systems::mark_transform_dirty(state.scene.registry(), entity.id());
        state.joint_entities.push_back(entity);
    }
}

// Update scene transforms and cache the resulting world-space nodes for external inspection.
void synchronize_scene_graph(runtime_state& state, const engine::math::vec3& body_translation) {
    if (state.joint_entities.size() != state.pose.joints.size()) {
        rebuild_scene_entities(state);
    }

    auto& registry = state.scene.registry();
    state.scene_nodes.clear();

    for (std::size_t index = 0; index < state.joint_entities.size() && index < state.pose.joints.size(); ++index) {
        auto entity = state.joint_entities[index];
        if (!entity.valid()) {
            continue;
        }

        const auto entt_entity = entity.id();
        auto& local = registry.get<engine::scene::components::LocalTransform>(entt_entity);
        const auto& pose_entry = state.pose.joints[index];
        local.value.scale = pose_entry.second.scale;
        local.value.rotation = pose_entry.second.rotation;
        local.value.translation = pose_entry.second.translation;
        if (pose_entry.first == "root") {
            local.value.translation += body_translation;
        }

        auto* name_component = registry.try_get<engine::scene::components::Name>(entt_entity);
        if (name_component == nullptr) {
            name_component = &registry.emplace<engine::scene::components::Name>(entt_entity);
        }
        name_component->value = pose_entry.first;

        engine::scene::systems::mark_transform_dirty(registry, entt_entity);
    }

    engine::scene::systems::propagate_transforms(registry);

    for (const auto& entity : state.joint_entities) {
        if (!entity.valid()) {
            continue;
        }

        const auto entt_entity = entity.id();
        const auto* name_component = registry.try_get<engine::scene::components::Name>(entt_entity);
        const auto* world = registry.try_get<engine::scene::components::WorldTransform>(entt_entity);
        if (!name_component || !world) {
            continue;
        }

        engine::runtime::runtime_frame_state::scene_node_state node{};
        node.name = name_component->value;
        node.transform = world->value;
        state.scene_nodes.push_back(std::move(node));
    }
}

void ensure_initialized(runtime_state& state) {
    if (state.initialized) {
        return;
    }

    state.world = engine::physics::PhysicsWorld{};
    engine::physics::RigidBody body;
    body.mass = 2.0F;
    body.position = engine::math::vec3{0.0F, 0.25F, 0.0F};
    MAYBE_UNUSED_CONST_AUTO id = engine::physics::add_body(state.world, body);
    state.body_positions.clear();
    state.body_positions.push_back(body.position);
    state.pose = engine::animation::evaluate_controller(state.controller);
    state.joint_names.clear();
    for (const auto& entry : state.pose.joints) {
        state.joint_names.push_back(entry.first);
    }
    state.last_report.execution_order.clear();
    const engine::math::vec3 body_translation = !state.body_positions.empty() ? state.body_positions.front() :
                                                                                 engine::math::vec3{0.0F, 0.0F, 0.0F};
    rebuild_scene_entities(state);
    synchronize_scene_graph(state, body_translation);
    state.initialized = true;
}

}  // namespace

namespace engine::runtime {

std::string_view module_name() noexcept {
    return "runtime";
}

std::size_t module_count() noexcept {
    return module_names().size();
}

std::string_view module_name_at(std::size_t index) noexcept {
    const auto& names = module_names();
    if (index >= names.size()) {
        return {};
    }
    return names[index];
}

void initialize() {
    auto& state = global_state();
    ensure_initialized(state);
}

void shutdown() {
    auto& state = global_state();
    state.initialized = false;
    state.simulation_time = 0.0;
    state.world = engine::physics::PhysicsWorld{};
    state.pose = engine::animation::evaluate_controller(state.controller);
    state.body_positions.clear();
    state.joint_names.clear();
    state.last_report.execution_order.clear();
    state.scene = engine::scene::Scene{};
    state.joint_entities.clear();
    state.scene_nodes.clear();
    engine::geometry::apply_uniform_translation(state.mesh, engine::math::vec3{0.0F, 0.0F, 0.0F});
    engine::geometry::recompute_vertex_normals(state.mesh);
}

runtime_frame_state tick(double dt) {
    auto& state = global_state();
    ensure_initialized(state);

    state.dispatcher.clear();

    const auto animation_kernel = state.dispatcher.add_kernel(
        "animation.evaluate",
        [&]() {
            engine::animation::advance_controller(state.controller, dt);
            state.pose = engine::animation::evaluate_controller(state.controller);
        });

    const auto physics_forces = state.dispatcher.add_kernel(
        "physics.accumulate",
        [&]() {
            engine::physics::clear_forces(state.world);
            if (!state.pose.joints.empty() && engine::physics::body_count(state.world) > 0) {
                if (const auto* root = state.pose.find("root")) {
                    const engine::math::vec3 drive = root->translation * 4.0F;
                    engine::physics::apply_force(state.world, 0, drive);
                }
            }
        },
        {animation_kernel});

    const auto physics_integrate = state.dispatcher.add_kernel(
        "physics.integrate",
        [&]() {
            engine::physics::integrate(state.world, dt);
            state.body_positions.clear();
            const auto count = engine::physics::body_count(state.world);
            state.body_positions.reserve(count);
            for (std::size_t index = 0; index < count; ++index) {
                state.body_positions.push_back(engine::physics::body_at(state.world, index).position);
            }
        },
        {physics_forces});

    const auto deform = state.dispatcher.add_kernel(
        "geometry.deform",
        [&]() {
            engine::math::vec3 translation{0.0F, 0.0F, 0.0F};
            if (!state.body_positions.empty()) {
                translation = state.body_positions.front();
            }
            if (const auto* root = state.pose.find("root")) {
                translation += root->translation;
            }
            engine::geometry::apply_uniform_translation(state.mesh, translation);
            engine::geometry::recompute_vertex_normals(state.mesh);
        },
        {physics_integrate});

    MAYBE_UNUSED_CONST_AUTO a = state.dispatcher.add_kernel(
        "geometry.finalize",
        [&]() {
            engine::geometry::update_bounds(state.mesh);
            state.joint_names.clear();
            state.joint_names.reserve(state.pose.joints.size());
            for (const auto& entry : state.pose.joints) {
                state.joint_names.push_back(entry.first);
            }
            const engine::math::vec3 body_translation = !state.body_positions.empty() ? state.body_positions.front() :
                                                                                         engine::math::vec3{0.0F, 0.0F, 0.0F};
            synchronize_scene_graph(state, body_translation);
        },
        {deform});

    state.last_report = state.dispatcher.dispatch();
    state.simulation_time += dt;

    runtime_frame_state frame{};
    frame.simulation_time = state.simulation_time;
    frame.pose = state.pose;
    frame.bounds = state.mesh.bounds;
    frame.body_positions = state.body_positions;
    frame.dispatch_report = state.last_report;
    frame.scene_nodes = state.scene_nodes;
    return frame;
}

const geometry::SurfaceMesh& current_mesh() {
    auto& state = global_state();
    ensure_initialized(state);
    return state.mesh;
}

}  // namespace engine::runtime

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_name() noexcept {
    return engine::runtime::module_name().data();
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_module_count() noexcept {
    return engine::runtime::module_count();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_at(std::size_t index) noexcept {
    const auto name = engine::runtime::module_name_at(index);
    return name.empty() ? nullptr : name.data();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_initialize() noexcept {
    engine::runtime::initialize();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_shutdown() noexcept {
    engine::runtime::shutdown();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_tick(double dt) noexcept {
    try {
        static_cast<void>(engine::runtime::tick(dt));
    } catch (...) {
        // swallow exceptions to keep C ABI stable
    }
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_body_count() noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    return state.body_positions.size();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_body_position(std::size_t index, float* out_position) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (!out_position || index >= state.body_positions.size()) {
        return;
    }
    const auto& value = state.body_positions[index];
    out_position[0] = value[0];
    out_position[1] = value[1];
    out_position[2] = value[2];
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_joint_count() noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    return state.joint_names.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_joint_name(std::size_t index) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (index >= state.joint_names.size()) {
        return nullptr;
    }
    return state.joint_names[index].c_str();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_joint_translation(std::size_t index, float* out_translation) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (!out_translation || index >= state.pose.joints.size()) {
        return;
    }
    const auto& pose_entry = state.pose.joints[index];
    out_translation[0] = pose_entry.second.translation[0];
    out_translation[1] = pose_entry.second.translation[1];
    out_translation[2] = pose_entry.second.translation[2];
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_mesh_bounds(float* out_min, float* out_max) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (!out_min || !out_max) {
        return;
    }
    const auto& bounds = state.mesh.bounds;
    out_min[0] = bounds.min[0];
    out_min[1] = bounds.min[1];
    out_min[2] = bounds.min[2];
    out_max[0] = bounds.max[0];
    out_max[1] = bounds.max[1];
    out_max[2] = bounds.max[2];
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_dispatch_count() noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    return state.last_report.execution_order.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_dispatch_name(std::size_t index) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (index >= state.last_report.execution_order.size()) {
        return nullptr;
    }
    return state.last_report.execution_order[index].c_str();
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_scene_node_count() noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    return state.scene_nodes.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_scene_node_name(std::size_t index) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (index >= state.scene_nodes.size()) {
        return nullptr;
    }
    return state.scene_nodes[index].name.c_str();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_scene_node_transform(
    std::size_t index,
    float* out_scale,
    float* out_rotation,
    float* out_translation) noexcept {
    auto& state = global_state();
    ensure_initialized(state);
    if (index >= state.scene_nodes.size()) {
        return;
    }
    const auto& node = state.scene_nodes[index];
    if (out_scale != nullptr) {
        out_scale[0] = node.transform.scale[0];
        out_scale[1] = node.transform.scale[1];
        out_scale[2] = node.transform.scale[2];
    }
    if (out_rotation != nullptr) {
        out_rotation[0] = node.transform.rotation.w;
        out_rotation[1] = node.transform.rotation.x;
        out_rotation[2] = node.transform.rotation.y;
        out_rotation[3] = node.transform.rotation.z;
    }
    if (out_translation != nullptr) {
        out_translation[0] = node.transform.translation[0];
        out_translation[1] = node.transform.translation[1];
        out_translation[2] = node.transform.translation[2];
    }
}

