#include "engine/physics/api.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace engine::physics {

namespace {

constexpr float minimum_step = 1e-6F;

[[nodiscard]] float safe_inverse_mass(float mass) noexcept {
    constexpr float epsilon = 1e-6F;
    return (mass <= epsilon) ? 0.0F : 1.0F / mass;
}

void integrate_substep(PhysicsWorld& world, float step) {
    const float damping = std::max(world.linear_damping, 0.0F);
    const float damping_factor = std::exp(-damping * step);
    for (auto& body : world.bodies) {
        if (body.inverse_mass == 0.0F) {
            body.accumulated_force = math::vec3{0.0F, 0.0F, 0.0F};
            continue;
        }
        const math::vec3 acceleration = body.accumulated_force * body.inverse_mass + world.gravity;
        body.velocity += acceleration * step;
        body.velocity *= damping_factor;
        body.position += body.velocity * step;
    }
}

}  // namespace

std::string_view module_name() noexcept {
    return "physics";
}

std::size_t add_body(PhysicsWorld& world, const RigidBody& body) {
    RigidBody instance = body;
    instance.mass = std::max(body.mass, 0.0F);
    instance.inverse_mass = safe_inverse_mass(instance.mass);
    if (instance.inverse_mass == 0.0F) {
        instance.velocity = math::vec3{0.0F, 0.0F, 0.0F};
        instance.accumulated_force = math::vec3{0.0F, 0.0F, 0.0F};
    }
    world.bodies.push_back(instance);
    return world.bodies.size() - 1U;
}

void clear_forces(PhysicsWorld& world) noexcept {
    for (auto& body : world.bodies) {
        body.accumulated_force = math::vec3{0.0F, 0.0F, 0.0F};
    }
}

void apply_force(PhysicsWorld& world, std::size_t index, const math::vec3& force) {
    if (index >= world.bodies.size()) {
        return;
    }
    world.bodies[index].accumulated_force += force;
}

void integrate(PhysicsWorld& world, double dt) {
    if (dt <= static_cast<double>(minimum_step)) {
        return;
    }
    const float max_step = std::max(world.max_substep, minimum_step);
    const std::uint32_t max_substeps = std::max(world.max_substeps, 1U);

    double remaining = dt;
    std::uint32_t steps = 0U;
    while (remaining > static_cast<double>(minimum_step) && steps < max_substeps) {
        const float step = static_cast<float>(std::min(remaining, static_cast<double>(max_step)));
        integrate_substep(world, step);
        remaining -= step;
        ++steps;
    }

    if (remaining > static_cast<double>(minimum_step)) {
        integrate_substep(world, static_cast<float>(remaining));
    }

    for (auto& body : world.bodies) {
        body.accumulated_force = math::vec3{0.0F, 0.0F, 0.0F};
    }
}

void set_linear_damping(PhysicsWorld& world, float damping) noexcept {
    world.linear_damping = std::max(damping, 0.0F);
}

void set_substepping(PhysicsWorld& world, float max_step, std::uint32_t max_substeps) noexcept {
    world.max_substep = std::max(max_step, minimum_step);
    world.max_substeps = std::max(max_substeps, 1U);
}

std::size_t body_count(const PhysicsWorld& world) noexcept {
    return world.bodies.size();
}

const RigidBody& body_at(const PhysicsWorld& world, std::size_t index) {
    if (index >= world.bodies.size()) {
        throw std::out_of_range{"physics::body_at index out of range"};
    }
    return world.bodies[index];
}

RigidBody& body_at(PhysicsWorld& world, std::size_t index) {
    if (index >= world.bodies.size()) {
        throw std::out_of_range{"physics::body_at index out of range"};
    }
    return world.bodies[index];
}

void set_collider(PhysicsWorld& world, std::size_t index, const Collider& collider) noexcept {
    if (index >= world.bodies.size()) {
        return;
    }
    world.bodies[index].collider = collider;
}

void clear_collider(PhysicsWorld& world, std::size_t index) noexcept {
    if (index >= world.bodies.size()) {
        return;
    }
    world.bodies[index].collider = Collider{};
}

bool has_collider(const PhysicsWorld& world, std::size_t index) noexcept {
    return (index < world.bodies.size()) &&
           (world.bodies[index].collider.type != Collider::Type::None);
}

const Collider* collider_at(const PhysicsWorld& world, std::size_t index) noexcept {
    if (!has_collider(world, index)) {
        return nullptr;
    }
    return &world.bodies[index].collider;
}

}  // namespace engine::physics

extern "C" ENGINE_PHYSICS_API const char* engine_physics_module_name() noexcept {
    return engine::physics::module_name().data();
}
