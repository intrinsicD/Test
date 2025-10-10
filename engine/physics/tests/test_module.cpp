#include <cstddef>
#include <stdexcept>

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

TEST(PhysicsWorld, ClearForcesResetsAccumulatedForce) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody body_a;
    body_a.mass = 1.0F;
    body_a.position = engine::math::vec3{1.0F, 0.0F, 0.0F};
    const auto body_a_index = engine::physics::add_body(world, body_a);

    engine::physics::RigidBody body_b;
    body_b.mass = 2.0F;
    body_b.position = engine::math::vec3{-1.0F, 0.0F, 0.0F};
    const auto body_b_index = engine::physics::add_body(world, body_b);

    const engine::math::vec3 zero_force{0.0F, 0.0F, 0.0F};

    engine::physics::apply_force(world, body_a_index, engine::math::vec3{3.0F, 0.0F, 0.0F});
    engine::physics::apply_force(world, body_b_index, engine::math::vec3{0.0F, 4.0F, 0.0F});

    EXPECT_FALSE(zero_force == engine::physics::body_at(world, body_a_index).accumulated_force);
    EXPECT_FALSE(zero_force == engine::physics::body_at(world, body_b_index).accumulated_force);

    engine::physics::clear_forces(world);

    const auto& cleared_a = engine::physics::body_at(world, body_a_index);
    const auto& cleared_b = engine::physics::body_at(world, body_b_index);
    EXPECT_EQ(zero_force, cleared_a.accumulated_force);
    EXPECT_EQ(zero_force, cleared_b.accumulated_force);
}

TEST(PhysicsWorld, ApplyForceOutOfRangeDoesNotMutateWorld) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody body;
    body.mass = 1.0F;
    body.position = engine::math::vec3{0.0F, 1.0F, 0.0F};
    const auto body_index = engine::physics::add_body(world, body);

    ASSERT_EQ(1U, engine::physics::body_count(world));
    ASSERT_EQ(0U, body_index);
    const auto snapshot = world;

    const engine::math::vec3 zero_force{0.0F, 0.0F, 0.0F};

    engine::physics::apply_force(world, engine::physics::body_count(world), engine::math::vec3{5.0F, 0.0F, 0.0F});

    ASSERT_EQ(snapshot.bodies.size(), world.bodies.size());
    for (std::size_t i = 0; i < world.bodies.size(); ++i) {
        const auto& before = snapshot.bodies[i];
        const auto& after = world.bodies[i];
        EXPECT_EQ(before.mass, after.mass);
        EXPECT_EQ(before.inverse_mass, after.inverse_mass);
        EXPECT_EQ(before.position, after.position);
        EXPECT_EQ(before.velocity, after.velocity);
        EXPECT_EQ(zero_force, after.accumulated_force);
    }
}

TEST(PhysicsWorld, BodyAtThrowsWhenIndexIsOutOfRange) {
    engine::physics::PhysicsWorld world;
    engine::physics::RigidBody body;
    const auto body_index = engine::physics::add_body(world, body);
    ASSERT_EQ(0U, body_index);

    const auto invalid_index = engine::physics::body_count(world);

    EXPECT_THROW(engine::physics::body_at(world, invalid_index), std::out_of_range);

    const auto& const_world = static_cast<const engine::physics::PhysicsWorld&>(world);
    EXPECT_THROW(engine::physics::body_at(const_world, invalid_index), std::out_of_range);
}
