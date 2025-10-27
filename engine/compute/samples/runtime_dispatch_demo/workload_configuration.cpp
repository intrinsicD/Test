#include "workload_configuration.hpp"

#include "engine/animation/api.hpp"
#include "engine/animation/rigging/rig_binding.hpp"
#include "engine/compute/api.hpp"
#include "engine/geometry/api.hpp"
#include "engine/physics/api.hpp"
#include "engine/runtime/api.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    namespace geometry = engine::geometry;

    using WorkloadProfile = engine::compute::samples::WorkloadProfile;

    [[nodiscard]] geometry::SurfaceMesh make_grid_mesh(std::size_t subdivisions)
    {
        const std::size_t segments = std::max<std::size_t>(1U, subdivisions);
        const std::size_t vertex_dim = segments + 1U;
        const float step = 1.0F / static_cast<float>(segments);

        geometry::SurfaceMesh mesh{};
        mesh.rest_positions.reserve(vertex_dim * vertex_dim);
        for (std::size_t z = 0; z < vertex_dim; ++z)
        {
            const float fz = static_cast<float>(z) * step - 0.5F;
            for (std::size_t x = 0; x < vertex_dim; ++x)
            {
                const float fx = static_cast<float>(x) * step - 0.5F;
                mesh.rest_positions.emplace_back(fx, 0.0F, fz);
            }
        }

        mesh.positions = mesh.rest_positions;
        mesh.normals.assign(mesh.positions.size(), engine::math::vec3{0.0F, 1.0F, 0.0F});
        mesh.indices.reserve(segments * segments * 6U);
        for (std::size_t z = 0; z < segments; ++z)
        {
            for (std::size_t x = 0; x < segments; ++x)
            {
                const std::uint32_t top_left = static_cast<std::uint32_t>(z * vertex_dim + x);
                const std::uint32_t top_right = top_left + 1U;
                const std::uint32_t bottom_left = static_cast<std::uint32_t>((z + 1U) * vertex_dim + x);
                const std::uint32_t bottom_right = bottom_left + 1U;
                mesh.indices.insert(
                    mesh.indices.end(),
                    {top_left, top_right, bottom_right, top_left, bottom_right, bottom_left});
            }
        }

        geometry::update_bounds(mesh);
        return mesh;
    }

    [[nodiscard]] engine::animation::RigBinding make_uniform_binding(std::size_t vertex_count)
    {
        engine::animation::RigBinding binding{};
        engine::animation::RigJoint root{};
        root.name = "root";
        root.parent = engine::animation::RigBinding::kInvalidIndex;
        root.inverse_bind_pose = engine::math::Transform<float>::Identity();
        binding.joints.push_back(root);
        binding.resize_vertices(vertex_count);
        for (auto& vertex : binding.vertices)
        {
            vertex.clear();
            [[maybe_unused]] const bool added = vertex.add_influence(0U, 1.0F);
            (void)added;
            vertex.normalize_weights();
        }
        return binding;
    }

    [[nodiscard]] engine::physics::PhysicsWorld make_physics_world(std::size_t body_count)
    {
        engine::physics::PhysicsWorld world{};
        world.linear_damping = 0.05F;
        const std::size_t count = std::max<std::size_t>(1U, body_count);
        const std::size_t grid = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(count))));
        const float spacing = 0.6F;

        for (std::size_t index = 0; index < count; ++index)
        {
            engine::physics::RigidBody body{};
            body.mass = 2.0F;
            body.inverse_mass = 1.0F / body.mass;
            const std::size_t gx = index % grid;
            const std::size_t gz = index / grid;
            body.position = engine::math::vec3{
                (static_cast<float>(gx) - static_cast<float>(grid - 1U) * 0.5F) * spacing,
                0.5F + static_cast<float>(gz) * spacing * 0.5F,
                0.0F
            };
            body.collider = engine::physics::Collider::make_sphere(0.25F);
            (void)engine::physics::add_body(world, body);
        }

        engine::physics::RigidBody ground{};
        ground.mass = 0.0F;
        ground.inverse_mass = 0.0F;
        ground.position = engine::math::vec3{0.0F, -0.5F, 0.0F};
        ground.collider = engine::physics::Collider::make_aabb(
            geometry::Aabb{engine::math::vec3{-5.0F, -0.1F, -5.0F}, engine::math::vec3{5.0F, 0.1F, 5.0F}});
        (void)engine::physics::add_body(world, ground);

        return world;
    }
}

namespace engine::compute::samples
{
    std::string_view workload_to_string(WorkloadProfile workload) noexcept
    {
        switch (workload)
        {
        case WorkloadProfile::Light:
            return "light";
        case WorkloadProfile::Balanced:
            return "balanced";
        case WorkloadProfile::Heavy:
        default:
            return "heavy";
        }
    }

    WorkloadProfileDefinition workload_definition(WorkloadProfile profile) noexcept
    {
        switch (profile)
        {
        case WorkloadProfile::Light:
            return WorkloadProfileDefinition{
                .profile = WorkloadProfile::Light,
                .mesh_subdivisions = 16U,
                .physics_bodies = 16U,
            };
        case WorkloadProfile::Balanced:
            return WorkloadProfileDefinition{
                .profile = WorkloadProfile::Balanced,
                .mesh_subdivisions = 32U,
                .physics_bodies = 48U,
            };
        case WorkloadProfile::Heavy:
        default:
            return WorkloadProfileDefinition{
                .profile = WorkloadProfile::Heavy,
                .mesh_subdivisions = 64U,
                .physics_bodies = 128U,
            };
        }
    }

    void configure_runtime_host(
        engine::runtime::RuntimeHost& host,
        WorkloadProfile workload,
        DispatcherFactory dispatcher_factory)
    {
        engine::runtime::RuntimeHostDependencies dependencies{};
        const auto definition = workload_definition(workload);
        dependencies.scene_name = std::string{"runtime.scene."} + std::string{workload_to_string(workload)};
        dependencies.mesh = make_grid_mesh(definition.mesh_subdivisions);
        dependencies.binding = make_uniform_binding(dependencies.mesh.rest_positions.size());
        dependencies.world = make_physics_world(definition.physics_bodies);
        if (dispatcher_factory)
        {
            dependencies.dispatcher_factory = std::move(dispatcher_factory);
        }
        host.configure(std::move(dependencies));
    }
} // namespace engine::compute::samples