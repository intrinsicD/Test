#include "engine/physics/api.hpp"

#include "engine/geometry/utils/shape_interactions.hpp"

namespace engine::physics {
namespace {

[[nodiscard]] bool body_has_collider(const RigidBody& body) noexcept {
    return body.collider.type != Collider::Type::None;
}

[[nodiscard]] math::vec3 collider_translation(const RigidBody& body) noexcept {
    return body.position + body.collider.offset;
}

[[nodiscard]] engine::geometry::Sphere world_space_sphere(const RigidBody& body) noexcept {
    auto sphere = body.collider.sphere;
    sphere.center += collider_translation(body);
    return sphere;
}

[[nodiscard]] engine::geometry::Aabb world_space_aabb(const RigidBody& body) noexcept {
    auto box = body.collider.aabb;
    const auto translation = collider_translation(body);
    box.min += translation;
    box.max += translation;
    return box;
}

[[nodiscard]] bool colliders_intersect(const RigidBody& lhs, const RigidBody& rhs) noexcept {
    using ColliderType = Collider::Type;

    if (!body_has_collider(lhs) || !body_has_collider(rhs)) {
        return false;
    }

    switch (lhs.collider.type) {
    case ColliderType::Sphere: {
        const auto lhs_sphere = world_space_sphere(lhs);
        switch (rhs.collider.type) {
        case ColliderType::Sphere:
            return engine::geometry::Intersects(lhs_sphere, world_space_sphere(rhs));
        case ColliderType::Aabb:
            return engine::geometry::Intersects(lhs_sphere, world_space_aabb(rhs));
        case ColliderType::None:
            return false;
        }
        break;
    }
    case ColliderType::Aabb: {
        const auto lhs_aabb = world_space_aabb(lhs);
        switch (rhs.collider.type) {
        case ColliderType::Sphere:
            return engine::geometry::Intersects(lhs_aabb, world_space_sphere(rhs));
        case ColliderType::Aabb:
            return engine::geometry::Intersects(lhs_aabb, world_space_aabb(rhs));
        case ColliderType::None:
            return false;
        }
        break;
    }
    case ColliderType::None:
        return false;
    }

    return false;
}

}  // namespace

std::vector<CollisionPair> detect_collisions(const PhysicsWorld& world) {
    std::vector<CollisionPair> collisions;
    const auto count = world.bodies.size();
    for (std::size_t i = 0; i < count; ++i) {
        if (!engine::physics::has_collider(world, i)) {
            continue;
        }
        for (std::size_t j = i + 1; j < count; ++j) {
            if (!engine::physics::has_collider(world, j)) {
                continue;
            }
            if (colliders_intersect(world.bodies[i], world.bodies[j])) {
                collisions.push_back(CollisionPair{i, j});
            }
        }
    }
    return collisions;
}

}  // namespace engine::physics
