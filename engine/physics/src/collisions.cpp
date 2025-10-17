#include "engine/physics/api.hpp"

#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/shapes/segment.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::physics {
namespace {

constexpr float penetration_epsilon = 1e-6F;

struct ContactKey {
    std::size_t first{0U};
    std::size_t second{0U};
};

[[nodiscard]] ContactKey make_contact_key(std::size_t first, std::size_t second) noexcept {
    if (first > second) {
        std::swap(first, second);
    }
    return ContactKey{first, second};
}

struct ContactKeyHash {
    [[nodiscard]] std::size_t operator()(const ContactKey& key) const noexcept {
        return std::hash<std::size_t>{}(key.first) ^ (std::hash<std::size_t>{}(key.second) << 1U);
    }
};

struct ContactKeyEqual {
    [[nodiscard]] bool operator()(const ContactKey& lhs, const ContactKey& rhs) const noexcept {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};

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

[[nodiscard]] math::vec3 closest_point_on_segment(const math::vec3& point,
                                                  const math::vec3& a,
                                                  const math::vec3& b) noexcept {
    const auto ab = b - a;
    const auto ap = point - a;
    const float denom = math::dot(ab, ab);
    if (denom <= 0.0F) {
        return a;
    }
    const float t = std::clamp(math::dot(ap, ab) / denom, 0.0F, 1.0F);
    return a + ab * t;
}

[[nodiscard]] math::vec3 closest_point_on_aabb(const math::vec3& point,
                                               const engine::geometry::Aabb& aabb) noexcept {
    return math::vec3{std::clamp(point[0], aabb.min[0], aabb.max[0]),
                      std::clamp(point[1], aabb.min[1], aabb.max[1]),
                      std::clamp(point[2], aabb.min[2], aabb.max[2])};
}

struct SegmentClosestPoints {
    math::vec3 point_a{0.0F, 0.0F, 0.0F};
    math::vec3 point_b{0.0F, 0.0F, 0.0F};
    float distance_sq{0.0F};
};

[[nodiscard]] SegmentClosestPoints closest_points_between_segments(const math::vec3& p0,
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
    return SegmentClosestPoints{closest_p, closest_q, math::dot(diff, diff)};
}

[[nodiscard]] float squared_distance_segment_segment(const math::vec3& p0,
                                                     const math::vec3& p1,
                                                     const math::vec3& q0,
                                                     const math::vec3& q1) noexcept {
    return closest_points_between_segments(p0, p1, q0, q1).distance_sq;
}

[[nodiscard]] float squared_distance_point_segment(const math::vec3& point,
                                                   const math::vec3& a,
                                                   const math::vec3& b) noexcept {
    const auto closest = closest_point_on_segment(point, a, b);
    const auto diff = point - closest;
    return math::dot(diff, diff);
}

[[nodiscard]] math::vec3 safe_normalized(const math::vec3& value,
                                         const math::vec3& fallback = math::vec3{0.0F, 1.0F, 0.0F}) noexcept {
    const float length_sq = math::dot(value, value);
    if (length_sq <= std::numeric_limits<float>::epsilon()) {
        return fallback;
    }
    const float inv_length = 1.0F / std::sqrt(length_sq);
    return value * inv_length;
}

[[nodiscard]] math::vec3 dominant_axis(const math::vec3& value,
                                       const math::vec3& fallback = math::vec3{0.0F, 1.0F, 0.0F}) noexcept {
    const math::vec3 abs_value{std::fabs(value[0]), std::fabs(value[1]), std::fabs(value[2])};
    std::size_t axis = 0U;
    float max_component = abs_value[0];
    for (std::size_t i = 1; i < 3; ++i) {
        if (abs_value[i] > max_component) {
            axis = i;
            max_component = abs_value[i];
        }
    }
    if (max_component <= std::numeric_limits<float>::epsilon()) {
        return fallback;
    }
    math::vec3 direction{0.0F, 0.0F, 0.0F};
    direction[axis] = (value[axis] >= 0.0F) ? 1.0F : -1.0F;
    return direction;
}

[[nodiscard]] math::vec3 aabb_center(const engine::geometry::Aabb& aabb) noexcept {
    return (aabb.min + aabb.max) * 0.5F;
}

[[nodiscard]] math::vec3 aabb_extents(const engine::geometry::Aabb& aabb) noexcept {
    return (aabb.max - aabb.min) * 0.5F;
}

[[nodiscard]] math::vec3 fallback_normal(const RigidBody& first, const RigidBody& second) noexcept {
    return dominant_axis(second.position - first.position);
}

[[nodiscard]] ContactPoint make_contact_point(const math::vec3& normal,
                                              float penetration,
                                              const math::vec3& point_a,
                                              const math::vec3& point_b) noexcept {
    ContactPoint contact;
    contact.normal = normal;
    contact.penetration = std::max(penetration, 0.0F);
    contact.position = (point_a + point_b) * 0.5F;
    return contact;
}

[[nodiscard]] ContactPoint contact_sphere_sphere(const RigidBody& first, const RigidBody& second) noexcept {
    const auto sphere_a = world_space_sphere(first);
    const auto sphere_b = world_space_sphere(second);
    const auto delta = sphere_b.center - sphere_a.center;
    const auto normal = safe_normalized(delta, fallback_normal(first, second));
    const auto distance = math::length(delta);
    const float penetration = sphere_a.radius + sphere_b.radius - distance;
    const auto point_a = sphere_a.center + normal * sphere_a.radius;
    const auto point_b = sphere_b.center - normal * sphere_b.radius;
    return make_contact_point(normal, penetration, point_a, point_b);
}

[[nodiscard]] ContactPoint contact_sphere_aabb(const RigidBody& sphere_body,
                                              const RigidBody& box_body) noexcept {
    const auto sphere = world_space_sphere(sphere_body);
    const auto box = world_space_aabb(box_body);
    const auto closest = closest_point_on_aabb(sphere.center, box);
    auto direction = closest - sphere.center;
    const float distance = math::length(direction);
    if (distance <= penetration_epsilon) {
        const auto center = aabb_center(box);
        const auto extents = aabb_extents(box);
        const auto local = sphere.center - center;
        math::vec3 abs_local{std::fabs(local[0]), std::fabs(local[1]), std::fabs(local[2])};
        math::vec3 face_distance{extents[0] - abs_local[0], extents[1] - abs_local[1], extents[2] - abs_local[2]};
        std::size_t axis = 0U;
        float min_face = face_distance[0];
        for (std::size_t i = 1; i < 3; ++i) {
            if (face_distance[i] < min_face) {
                min_face = face_distance[i];
                axis = i;
            }
        }
        math::vec3 normal{0.0F, 0.0F, 0.0F};
        normal[axis] = (local[axis] >= 0.0F) ? 1.0F : -1.0F;
        const float penetration = sphere.radius + std::max(min_face, 0.0F);
        const auto point_b = sphere.center + normal * std::max(min_face, 0.0F);
        const auto point_a = sphere.center - normal * sphere.radius;
        return make_contact_point(normal, penetration, point_a, point_b);
    }
    const auto normal = safe_normalized(direction, fallback_normal(sphere_body, box_body));
    const float penetration = sphere.radius - distance;
    const auto point_a = sphere.center + normal * sphere.radius;
    const auto point_b = closest;
    return make_contact_point(normal, penetration, point_a, point_b);
}

[[nodiscard]] ContactPoint contact_aabb_aabb(const RigidBody& first, const RigidBody& second) noexcept {
    const auto box_a = world_space_aabb(first);
    const auto box_b = world_space_aabb(second);
    const math::vec3 overlap_min{std::max(box_a.min[0], box_b.min[0]),
                                 std::max(box_a.min[1], box_b.min[1]),
                                 std::max(box_a.min[2], box_b.min[2])};
    const math::vec3 overlap_max{std::min(box_a.max[0], box_b.max[0]),
                                 std::min(box_a.max[1], box_b.max[1]),
                                 std::min(box_a.max[2], box_b.max[2])};
    const math::vec3 overlaps = overlap_max - overlap_min;
    std::size_t axis = 0U;
    float min_overlap = overlaps[0];
    for (std::size_t i = 1; i < 3; ++i) {
        if (overlaps[i] < min_overlap) {
            min_overlap = overlaps[i];
            axis = i;
        }
    }
    const auto center_a = aabb_center(box_a);
    const auto center_b = aabb_center(box_b);
    math::vec3 normal{0.0F, 0.0F, 0.0F};
    normal[axis] = (center_b[axis] >= center_a[axis]) ? 1.0F : -1.0F;
    const float penetration = std::max(min_overlap, 0.0F);
    math::vec3 point_a = overlap_min;
    math::vec3 point_b = overlap_max;
    point_a[axis] = (normal[axis] > 0.0F) ? box_a.max[axis] : box_a.min[axis];
    point_b[axis] = (normal[axis] > 0.0F) ? box_b.min[axis] : box_b.max[axis];
    point_a = math::vec3{(point_a[0] + point_b[0]) * 0.5F,
                         (point_a[1] + point_b[1]) * 0.5F,
                         (point_a[2] + point_b[2]) * 0.5F};
    point_b = point_a;
    return make_contact_point(normal, penetration, point_a, point_b);
}

[[nodiscard]] ContactPoint contact_capsule_sphere(const RigidBody& capsule_body,
                                                 const RigidBody& sphere_body) noexcept {
    const auto capsule = world_space_capsule(capsule_body);
    const auto sphere = world_space_sphere(sphere_body);
    const auto closest = closest_point_on_segment(sphere.center, capsule.point_a, capsule.point_b);
    auto direction = sphere.center - closest;
    const auto normal = safe_normalized(direction, fallback_normal(capsule_body, sphere_body));
    const float distance = math::length(direction);
    const float penetration = capsule.radius + sphere.radius - distance;
    const auto point_a = closest + normal * capsule.radius;
    const auto point_b = sphere.center - normal * sphere.radius;
    return make_contact_point(normal, penetration, point_a, point_b);
}

[[nodiscard]] ContactPoint contact_capsule_capsule(const RigidBody& first,
                                                  const RigidBody& second) noexcept {
    const auto capsule_a = world_space_capsule(first);
    const auto capsule_b = world_space_capsule(second);
    const auto closest = closest_points_between_segments(capsule_a.point_a, capsule_a.point_b,
                                                         capsule_b.point_a, capsule_b.point_b);
    const auto normal = safe_normalized(closest.point_b - closest.point_a, fallback_normal(first, second));
    const float distance = std::sqrt(closest.distance_sq);
    const float penetration = capsule_a.radius + capsule_b.radius - distance;
    const auto point_a = closest.point_a + normal * capsule_a.radius;
    const auto point_b = closest.point_b - normal * capsule_b.radius;
    return make_contact_point(normal, penetration, point_a, point_b);
}

[[nodiscard]] ContactPoint contact_capsule_aabb(const RigidBody& capsule_body,
                                               const RigidBody& box_body) noexcept {
    const auto capsule = world_space_capsule(capsule_body);
    const auto box = world_space_aabb(box_body);
    const auto midpoint = (capsule.point_a + capsule.point_b) * 0.5F;
    auto closest_on_segment = closest_point_on_segment(closest_point_on_aabb(midpoint, box),
                                                      capsule.point_a,
                                                      capsule.point_b);
    const auto closest_on_box = closest_point_on_aabb(closest_on_segment, box);
    auto direction = closest_on_box - closest_on_segment;
    const float distance = math::length(direction);
    math::vec3 normal = safe_normalized(direction, fallback_normal(capsule_body, box_body));
    float penetration = capsule.radius - distance;
    if (distance <= penetration_epsilon) {
        const auto center = aabb_center(box);
        normal = dominant_axis(center - closest_on_segment, fallback_normal(capsule_body, box_body));
        const auto point_b = closest_point_on_aabb(closest_on_segment + normal * capsule.radius, box);
        const auto point_a = closest_on_segment + normal * capsule.radius;
        return make_contact_point(normal, capsule.radius, point_a, point_b);
    }
    const auto point_a = closest_on_segment + normal * capsule.radius;
    const auto point_b = closest_on_box;
    return make_contact_point(normal, penetration, point_a, point_b);
}

[[nodiscard]] ContactPoint contact_aabb_sphere(const RigidBody& box_body,
                                              const RigidBody& sphere_body) noexcept {
    auto contact = contact_sphere_aabb(sphere_body, box_body);
    contact.normal *= -1.0F;
    return contact;
}

[[nodiscard]] ContactPoint contact_sphere_capsule(const RigidBody& sphere_body,
                                                 const RigidBody& capsule_body) noexcept {
    auto contact = contact_capsule_sphere(capsule_body, sphere_body);
    contact.normal *= -1.0F;
    return contact;
}

[[nodiscard]] ContactPoint contact_aabb_capsule(const RigidBody& box_body,
                                               const RigidBody& capsule_body) noexcept {
    auto contact = contact_capsule_aabb(capsule_body, box_body);
    contact.normal *= -1.0F;
    return contact;
}

[[nodiscard]] std::optional<ContactManifold> build_contact_manifold(const PhysicsWorld& world,
                                                                    std::size_t first,
                                                                    std::size_t second) noexcept {
    if (first >= world.bodies.size() || second >= world.bodies.size()) {
        return std::nullopt;
    }
    const auto& body_a = world.bodies[first];
    const auto& body_b = world.bodies[second];
    if (!body_has_collider(body_a) || !body_has_collider(body_b)) {
        return std::nullopt;
    }

    using ColliderType = Collider::Type;
    ContactPoint contact{};

    switch (body_a.collider.type) {
    case ColliderType::Sphere:
        switch (body_b.collider.type) {
        case ColliderType::Sphere:
            contact = contact_sphere_sphere(body_a, body_b);
            break;
        case ColliderType::Aabb:
            contact = contact_sphere_aabb(body_a, body_b);
            break;
        case ColliderType::Capsule:
            contact = contact_sphere_capsule(body_a, body_b);
            break;
        case ColliderType::None:
            return std::nullopt;
        }
        break;
    case ColliderType::Aabb:
        switch (body_b.collider.type) {
        case ColliderType::Sphere:
            contact = contact_aabb_sphere(body_a, body_b);
            break;
        case ColliderType::Aabb:
            contact = contact_aabb_aabb(body_a, body_b);
            break;
        case ColliderType::Capsule:
            contact = contact_aabb_capsule(body_a, body_b);
            break;
        case ColliderType::None:
            return std::nullopt;
        }
        break;
    case ColliderType::Capsule:
        switch (body_b.collider.type) {
        case ColliderType::Sphere:
            contact = contact_capsule_sphere(body_a, body_b);
            break;
        case ColliderType::Aabb:
            contact = contact_capsule_aabb(body_a, body_b);
            break;
        case ColliderType::Capsule:
            contact = contact_capsule_capsule(body_a, body_b);
            break;
        case ColliderType::None:
            return std::nullopt;
        }
        break;
    case ColliderType::None:
        return std::nullopt;
    }

    ContactManifold manifold{};
    manifold.first = first;
    manifold.second = second;
    manifold.contacts[0] = contact;
    manifold.contact_count = 1U;
    return manifold;
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

void update_contact_manifolds(PhysicsWorld& world) {
    std::unordered_map<ContactKey, ContactManifold, ContactKeyHash, ContactKeyEqual> previous;
    previous.reserve(world.manifolds.size());
    for (const auto& manifold : world.manifolds) {
        previous.emplace(make_contact_key(manifold.first, manifold.second), manifold);
    }

    const auto collisions = detect_collisions(world);
    std::vector<ContactManifold> next;
    next.reserve(collisions.size());

    std::size_t total_contacts = 0U;
    float max_penetration = 0.0F;

    for (const auto& pair : collisions) {
        const auto key = make_contact_key(pair.first, pair.second);
        const auto manifold_opt = build_contact_manifold(world, key.first, key.second);
        if (!manifold_opt.has_value()) {
            continue;
        }

        auto manifold = manifold_opt.value();
        if (const auto it = previous.find(key); it != previous.end()) {
            manifold.lifetime = it->second.lifetime + 1U;
        }
        next.push_back(manifold);
    }

    std::sort(next.begin(), next.end(), [](const ContactManifold& lhs, const ContactManifold& rhs) {
        return std::tie(lhs.first, lhs.second) < std::tie(rhs.first, rhs.second);
    });

    for (const auto& manifold : next) {
        total_contacts += manifold.contact_count;
        for (std::uint32_t i = 0; i < manifold.contact_count; ++i) {
            max_penetration = std::max(max_penetration, manifold.contacts[i].penetration);
        }
    }

    world.manifolds = std::move(next);
    world.collision_stats.manifold_count = world.manifolds.size();
    world.collision_stats.contact_count = total_contacts;
    world.collision_stats.max_penetration = max_penetration;

    if (world.constraint_callbacks.on_manifold != nullptr) {
        for (auto& manifold : world.manifolds) {
            world.constraint_callbacks.on_manifold(world, manifold, world.constraint_callbacks.user_data);
        }
    }
}

const std::vector<ContactManifold>& contact_manifolds(const PhysicsWorld& world) noexcept {
    return world.manifolds;
}

const CollisionTelemetry& collision_telemetry(const PhysicsWorld& world) noexcept {
    return world.collision_stats;
}

void set_constraint_callbacks(PhysicsWorld& world, ConstraintSolverCallbacks callbacks) noexcept {
    world.constraint_callbacks = callbacks;
}

}  // namespace engine::physics
