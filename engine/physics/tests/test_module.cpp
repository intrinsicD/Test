#include <gtest/gtest.h>

#include "engine/physics/api.hpp"

TEST(PhysicsModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::physics::module_name(), "physics");
    EXPECT_STREQ(engine_physics_module_name(), "physics");
}

TEST(PhysicsModule, IntegratesBodiesUnderForce) {
    engine::physics::PhysicsWorld world;
    world.gravity = engine::math::vec3{0.0F, 0.0F, 0.0F};

    engine::physics::RigidBody body;
    body.mass = 2.0F;
    body.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    body.velocity = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto index = engine::physics::add_body(world, body);

    engine::physics::apply_force(world, index, engine::math::vec3{2.0F, 0.0F, 0.0F});
    engine::physics::integrate(world, 1.0);

    const auto& simulated = engine::physics::body_at(world, index);
    EXPECT_NEAR(simulated.velocity[0], 1.0F, 1e-4F);
    EXPECT_NEAR(simulated.position[0], 1.0F, 1e-4F);
}
