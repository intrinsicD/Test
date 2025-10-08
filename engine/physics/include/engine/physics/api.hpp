#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "engine/math/math.hpp"

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

struct RigidBody {
    float mass{1.0F};
    float inverse_mass{1.0F};
    math::vec3 position{0.0F, 0.0F, 0.0F};
    math::vec3 velocity{0.0F, 0.0F, 0.0F};
    math::vec3 accumulated_force{0.0F, 0.0F, 0.0F};
};

struct PhysicsWorld {
    math::vec3 gravity{0.0F, -9.81F, 0.0F};
    std::vector<RigidBody> bodies;
};

[[nodiscard]] ENGINE_PHYSICS_API std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_PHYSICS_API std::size_t add_body(PhysicsWorld& world, const RigidBody& body);

ENGINE_PHYSICS_API void clear_forces(PhysicsWorld& world) noexcept;

ENGINE_PHYSICS_API void apply_force(PhysicsWorld& world, std::size_t index, const math::vec3& force);

ENGINE_PHYSICS_API void integrate(PhysicsWorld& world, double dt);

[[nodiscard]] ENGINE_PHYSICS_API std::size_t body_count(const PhysicsWorld& world) noexcept;

[[nodiscard]] ENGINE_PHYSICS_API const RigidBody& body_at(const PhysicsWorld& world, std::size_t index);

[[nodiscard]] ENGINE_PHYSICS_API RigidBody& body_at(PhysicsWorld& world, std::size_t index);

}  // namespace engine::physics

extern "C" ENGINE_PHYSICS_API const char* engine_physics_module_name() noexcept;
