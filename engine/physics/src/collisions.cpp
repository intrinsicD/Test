#include "engine/physics/api.hpp"

#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/shapes/segment.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

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

[[nodiscard]] Collider::Capsule world_space_capsule(const RigidBody& body) noexcept {
    auto capsule = body.collider.capsule;
    const auto translation = collider_translation(body);
    capsule.point_a += translation;
    capsule.point_b += translation;
    return capsule;
}

[[nodiscard]] float squared_distance_point_segment(const math::vec3& point,
                                                   const math::vec3& a,
                                                   const math::vec3& b) noexcept {
    const auto ab = b - a;
    const auto ap = point - a;
    const float denom = math::dot(ab, ab);
    float t = 0.0F;
    if (denom > 0.0F) {
        t = std::clamp(math::dot(ap, ab) / denom, 0.0F, 1.0F);
    }
    const auto closest = a + ab * t;
    const auto diff = point - closest;
    return math::dot(diff, diff);
}

[[nodiscard]] float squared_distance_segment_segment(const math::vec3& p0,
                                                     const math::vec3& p1,
                                                     const math::vec3& q0,
                                                     const math::vec3& q1) noexcept {
    const auto u = p1 - p0;
    const auto v = q1 - q0;
    const auto w0 = p0 - q0;
    const float a = math::dot(u, u);
    const float b = math::dot(u, v);
    const float c = math::dot(v, v);
    const float d = math::dot(u, w0);
    const float e = math::dot(v, w0);
    const float denom = a * c - b * b;
    float s = 0.0F;
    float t = 0.0F;

    if (denom > 0.0F) {
        s = std::clamp((b * e - c * d) / denom, 0.0F, 1.0F);
    }

    const float t_nom = b * s + e;
    if (t_nom <= 0.0F) {
        t = 0.0F;
        s = std::clamp(-d / a, 0.0F, 1.0F);
    } else if (t_nom >= c) {
        t = 1.0F;
        s = std::clamp((b - d) / a, 0.0F, 1.0F);
    } else {
        t = t_nom / c;
    }

    const auto closest_p = p0 + u * s;
    const auto closest_q = q0 + v * t;
    const auto diff = closest_p - closest_q;
    return math::dot(diff, diff);
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
        case ColliderType::Capsule: {
            const auto capsule = world_space_capsule(rhs);
            const float distance_sq = squared_distance_point_segment(lhs_sphere.center, capsule.point_a, capsule.point_b);
            const float radius = capsule.radius + lhs_sphere.radius;
            return distance_sq <= radius * radius;
        }
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
        case ColliderType::Capsule: {
            const auto capsule = world_space_capsule(rhs);
            engine::geometry::Aabb expanded = lhs_aabb;
            const math::vec3 radius{rhs.collider.capsule.radius, rhs.collider.capsule.radius, rhs.collider.capsule.radius};
            expanded.min -= radius;
            expanded.max += radius;
            engine::geometry::Segment segment{capsule.point_a, capsule.point_b};
            return engine::geometry::Intersects(expanded, segment);
        }
        case ColliderType::None:
            return false;
        }
        break;
    }
    case ColliderType::Capsule: {
        const auto lhs_capsule = world_space_capsule(lhs);
        switch (rhs.collider.type) {
        case ColliderType::Sphere: {
            const float distance_sq = squared_distance_point_segment(world_space_sphere(rhs).center,
                                                                     lhs_capsule.point_a,
                                                                     lhs_capsule.point_b);
            const float radius = lhs_capsule.radius + rhs.collider.sphere.radius;
            return distance_sq <= radius * radius;
        }
        case ColliderType::Aabb: {
            return colliders_intersect(rhs, lhs);
        }
        case ColliderType::Capsule: {
        const auto rhs_capsule = world_space_capsule(rhs);
        const float distance_sq = squared_distance_segment_segment(lhs_capsule.point_a,
                                                                  lhs_capsule.point_b,
                                                                  rhs_capsule.point_a,
                                                                  rhs_capsule.point_b);
        const float radius = lhs_capsule.radius + rhs_capsule.radius;
        return distance_sq <= radius * radius;
        }
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

[[nodiscard]] engine::geometry::Aabb broadphase_aabb(const RigidBody& body) noexcept {
    engine::geometry::Aabb bounds{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}};
    switch (body.collider.type) {
    case Collider::Type::Sphere: {
        const auto sphere = world_space_sphere(body);
        const math::vec3 radius{sphere.radius, sphere.radius, sphere.radius};
        bounds.min = sphere.center - radius;
        bounds.max = sphere.center + radius;
        break;
    }
    case Collider::Type::Aabb: {
        bounds = world_space_aabb(body);
        break;
    }
    case Collider::Type::Capsule: {
        const auto capsule = world_space_capsule(body);
        math::vec3 min_point{std::min(capsule.point_a[0], capsule.point_b[0]),
                             std::min(capsule.point_a[1], capsule.point_b[1]),
                             std::min(capsule.point_a[2], capsule.point_b[2])};
        math::vec3 max_point{std::max(capsule.point_a[0], capsule.point_b[0]),
                             std::max(capsule.point_a[1], capsule.point_b[1]),
                             std::max(capsule.point_a[2], capsule.point_b[2])};
        const math::vec3 radius{capsule.radius, capsule.radius, capsule.radius};
        bounds.min = min_point - radius;
        bounds.max = max_point + radius;
        break;
    }
    case Collider::Type::None:
        break;
    }
    return bounds;
}

[[nodiscard]] bool aabb_overlap(const engine::geometry::Aabb& lhs, const engine::geometry::Aabb& rhs) noexcept {
    return (lhs.min[0] <= rhs.max[0] && lhs.max[0] >= rhs.min[0]) &&
           (lhs.min[1] <= rhs.max[1] && lhs.max[1] >= rhs.min[1]) &&
           (lhs.min[2] <= rhs.max[2] && lhs.max[2] >= rhs.min[2]);
}

}  // namespace

std::vector<CollisionPair> detect_collisions(const PhysicsWorld& world) {
    std::vector<CollisionPair> collisions;
    struct BroadPhaseEntry {
        std::size_t index{0U};
        float min_x{0.0F};
        float max_x{0.0F};
        engine::geometry::Aabb bounds{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}};
    };

    std::vector<BroadPhaseEntry> entries;
    entries.reserve(world.bodies.size());
    for (std::size_t i = 0; i < world.bodies.size(); ++i) {
        if (!engine::physics::has_collider(world, i)) {
            continue;
        }
        const auto bounds = broadphase_aabb(world.bodies[i]);
        BroadPhaseEntry entry;
        entry.index = i;
        entry.min_x = bounds.min[0];
        entry.max_x = bounds.max[0];
        entry.bounds = bounds;
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.min_x < rhs.min_x;
    });

    std::vector<BroadPhaseEntry> active;
    for (const auto& entry : entries) {
        active.erase(std::remove_if(active.begin(), active.end(), [&](const BroadPhaseEntry& candidate) {
                          return candidate.max_x < entry.min_x;
                      }),
                      active.end());

        for (const auto& candidate : active) {
            if (!aabb_overlap(entry.bounds, candidate.bounds)) {
                continue;
            }
            if (colliders_intersect(world.bodies[entry.index], world.bodies[candidate.index])) {
                collisions.push_back(CollisionPair{candidate.index, entry.index});
            }
        }

        active.push_back(entry);
    }

    return collisions;
}

}  // namespace engine::physics
