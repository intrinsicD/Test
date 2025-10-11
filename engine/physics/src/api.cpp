#include "engine/physics/api.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace engine::physics {

namespace {

[[nodiscard]] float safe_inverse_mass(float mass) noexcept {
    constexpr float epsilon = 1e-6F;
    return (mass <= epsilon) ? 0.0F : 1.0F / mass;
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
    const float step = static_cast<float>(dt);
    for (auto& body : world.bodies) {
        if (body.inverse_mass == 0.0F) {
            body.accumulated_force = math::vec3{0.0F, 0.0F, 0.0F};
            continue;
        }
        const math::vec3 acceleration = body.accumulated_force * body.inverse_mass + world.gravity;
        body.velocity += acceleration * step;
        body.position += body.velocity * step;
        body.accumulated_force = math::vec3{0.0F, 0.0F, 0.0F};
    }
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
