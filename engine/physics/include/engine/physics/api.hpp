#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "engine/math/math.hpp"
#include "engine/geometry/shapes.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_PHYSICS_EXPORTS)
#    define ENGINE_PHYSICS_API __declspec(dllexport)
#  else
#    define ENGINE_PHYSICS_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_PHYSICS_API
#endif

namespace engine::physics {

struct Collider {
    enum class Type {
        None,
        Sphere,
        Aabb,
    };

    Type type{Type::None};
    math::vec3 offset{0.0F, 0.0F, 0.0F};
    engine::geometry::Sphere sphere{{0.0F, 0.0F, 0.0F}, 0.0F};
    engine::geometry::Aabb aabb{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}};

    [[nodiscard]] static Collider none() noexcept {
        return Collider{};
    }

    [[nodiscard]] static Collider make_sphere(float radius,
                                              const math::vec3& local_center = math::vec3{0.0F, 0.0F, 0.0F},
                                              const math::vec3& offset = math::vec3{0.0F, 0.0F, 0.0F}) noexcept {
        Collider collider;
        collider.type = Type::Sphere;
        collider.offset = offset;
        collider.sphere = engine::geometry::Sphere{local_center, radius};
        return collider;
    }

    [[nodiscard]] static Collider make_sphere(const engine::geometry::Sphere& shape,
                                              const math::vec3& offset = math::vec3{0.0F, 0.0F, 0.0F}) noexcept {
        Collider collider;
        collider.type = Type::Sphere;
        collider.offset = offset;
        collider.sphere = shape;
        return collider;
    }

    [[nodiscard]] static Collider make_aabb(const engine::geometry::Aabb& shape,
                                            const math::vec3& offset = math::vec3{0.0F, 0.0F, 0.0F}) noexcept {
        Collider collider;
        collider.type = Type::Aabb;
        collider.offset = offset;
        collider.aabb = shape;
        return collider;
    }
};

struct RigidBody {
    float mass{1.0F};
    float inverse_mass{1.0F};
    math::vec3 position{0.0F, 0.0F, 0.0F};
    math::vec3 velocity{0.0F, 0.0F, 0.0F};
    math::vec3 accumulated_force{0.0F, 0.0F, 0.0F};
    Collider collider{};
};

struct PhysicsWorld {
    math::vec3 gravity{0.0F, -9.81F, 0.0F};
    std::vector<RigidBody> bodies;
};

struct CollisionPair {
    std::size_t first{0U};
    std::size_t second{0U};
};

[[nodiscard]] ENGINE_PHYSICS_API std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_PHYSICS_API std::size_t add_body(PhysicsWorld& world, const RigidBody& body);

ENGINE_PHYSICS_API void clear_forces(PhysicsWorld& world) noexcept;

ENGINE_PHYSICS_API void apply_force(PhysicsWorld& world, std::size_t index, const math::vec3& force);

ENGINE_PHYSICS_API void integrate(PhysicsWorld& world, double dt);

[[nodiscard]] ENGINE_PHYSICS_API std::size_t body_count(const PhysicsWorld& world) noexcept;

[[nodiscard]] ENGINE_PHYSICS_API const RigidBody& body_at(const PhysicsWorld& world, std::size_t index);

[[nodiscard]] ENGINE_PHYSICS_API RigidBody& body_at(PhysicsWorld& world, std::size_t index);

ENGINE_PHYSICS_API void set_collider(PhysicsWorld& world, std::size_t index, const Collider& collider) noexcept;

ENGINE_PHYSICS_API void clear_collider(PhysicsWorld& world, std::size_t index) noexcept;

[[nodiscard]] ENGINE_PHYSICS_API bool has_collider(const PhysicsWorld& world, std::size_t index) noexcept;

[[nodiscard]] ENGINE_PHYSICS_API const Collider* collider_at(const PhysicsWorld& world, std::size_t index) noexcept;

[[nodiscard]] ENGINE_PHYSICS_API std::vector<CollisionPair> detect_collisions(const PhysicsWorld& world);

}  // namespace engine::physics

extern "C" ENGINE_PHYSICS_API const char* engine_physics_module_name() noexcept;
