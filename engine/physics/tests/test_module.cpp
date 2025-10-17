#include <cstddef>
#include <stdexcept>

#include <gtest/gtest.h>

#include "engine/physics/api.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/sphere.hpp"

TEST(PhysicsModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::physics::module_name(), "physics");
    EXPECT_STREQ(engine_physics_module_name(), "physics");
}

TEST(PhysicsModule, NegativeMassClampsToStaticBody) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody body;
    body.mass = -5.0F;
    body.velocity = engine::math::vec3{3.0F, -2.0F, 1.0F};
    body.accumulated_force = engine::math::vec3{4.0F, 0.0F, 0.0F};

    const auto index = engine::physics::add_body(world, body);
    ASSERT_EQ(0U, index);

    const auto& stored = engine::physics::body_at(world, index);
    EXPECT_FLOAT_EQ(0.0F, stored.mass);
    EXPECT_FLOAT_EQ(0.0F, stored.inverse_mass);
    EXPECT_EQ((engine::math::vec3{0.0F, 0.0F, 0.0F}), stored.velocity);
    EXPECT_EQ((engine::math::vec3{0.0F, 0.0F, 0.0F}), stored.accumulated_force);
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

TEST(PhysicsModule, StaticBodiesIgnoreForcesAndGravity) {
    engine::physics::PhysicsWorld world;
    world.gravity = engine::math::vec3{0.0F, -9.81F, 0.0F};

    engine::physics::RigidBody body;
    body.mass = 0.0F;
    body.position = engine::math::vec3{1.0F, 2.0F, 3.0F};

    const auto index = engine::physics::add_body(world, body);
    engine::physics::apply_force(world, index, engine::math::vec3{15.0F, 0.0F, 0.0F});
    engine::physics::integrate(world, 0.5);

    const auto& simulated = engine::physics::body_at(world, index);
    EXPECT_EQ((engine::math::vec3{1.0F, 2.0F, 3.0F}), simulated.position);
    EXPECT_EQ((engine::math::vec3{0.0F, 0.0F, 0.0F}), simulated.velocity);
    EXPECT_EQ((engine::math::vec3{0.0F, 0.0F, 0.0F}), simulated.accumulated_force);
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

TEST(PhysicsWorldColliders, SetAndQueryColliderState) {
    engine::physics::PhysicsWorld world;
    engine::physics::RigidBody body;
    const auto index = engine::physics::add_body(world, body);

    ASSERT_EQ(1U, engine::physics::body_count(world));
    EXPECT_FALSE(engine::physics::has_collider(world, index));
    EXPECT_EQ(nullptr, engine::physics::collider_at(world, index));

    const auto collider = engine::physics::Collider::make_sphere(1.0F);
    engine::physics::set_collider(world, index, collider);

    EXPECT_TRUE(engine::physics::has_collider(world, index));
    const auto* stored = engine::physics::collider_at(world, index);
    ASSERT_TRUE(stored != nullptr);
    EXPECT_TRUE(stored->type == engine::physics::Collider::Type::Sphere);
    EXPECT_FLOAT_EQ(1.0F, stored->sphere.radius);

    engine::physics::clear_collider(world, index);
    EXPECT_FALSE(engine::physics::has_collider(world, index));
    EXPECT_EQ(nullptr, engine::physics::collider_at(world, index));
}

TEST(PhysicsWorldColliders, DetectsSphereSphereCollision) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody first;
    first.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto first_index = engine::physics::add_body(world, first);
    engine::physics::set_collider(world, first_index, engine::physics::Collider::make_sphere(1.0F));

    engine::physics::RigidBody second;
    second.position = engine::math::vec3{1.5F, 0.0F, 0.0F};
    const auto second_index = engine::physics::add_body(world, second);
    engine::physics::set_collider(world, second_index, engine::physics::Collider::make_sphere(1.0F));

    const auto collisions = engine::physics::detect_collisions(world);
    ASSERT_EQ(1U, collisions.size());
    EXPECT_EQ(first_index, collisions[0].first);
    EXPECT_EQ(second_index, collisions[0].second);
}

TEST(PhysicsWorldColliders, DetectsSphereAabbCollisionAndIgnoresSeparatedBodies) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody sphere_body;
    sphere_body.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto sphere_index = engine::physics::add_body(world, sphere_body);
    engine::physics::set_collider(world, sphere_index, engine::physics::Collider::make_sphere(0.5F));

    engine::physics::RigidBody box_body;
    box_body.position = engine::math::vec3{0.75F, 0.0F, 0.0F};
    const auto box_index = engine::physics::add_body(world, box_body);
    const engine::geometry::Aabb local_box = engine::geometry::MakeAabbFromCenterExtent({0.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    engine::physics::set_collider(world, box_index, engine::physics::Collider::make_aabb(local_box));

    engine::physics::RigidBody distant_body;
    distant_body.position = engine::math::vec3{5.0F, 0.0F, 0.0F};
    const auto distant_index = engine::physics::add_body(world, distant_body);
    engine::physics::set_collider(world, distant_index, engine::physics::Collider::make_sphere(0.25F));

    const auto collisions = engine::physics::detect_collisions(world);
    ASSERT_EQ(1U, collisions.size());
    EXPECT_EQ(sphere_index, collisions[0].first);
    EXPECT_EQ(box_index, collisions[0].second);

    EXPECT_FALSE(engine::physics::has_collider(world, static_cast<std::size_t>(42U)));
    EXPECT_EQ(nullptr, engine::physics::collider_at(world, static_cast<std::size_t>(42U)));
}

TEST(PhysicsWorld, SubsteppingAndDampingStabiliseIntegration) {
    engine::physics::PhysicsWorld world;
    world.gravity = engine::math::vec3{0.0F, 0.0F, 0.0F};
    engine::physics::set_linear_damping(world, 2.0F);
    engine::physics::set_substepping(world, 0.1F, 3U);

    engine::physics::RigidBody body;
    body.mass = 1.0F;
    body.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    body.velocity = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto index = engine::physics::add_body(world, body);

    engine::physics::apply_force(world, index, engine::math::vec3{2.0F, 0.0F, 0.0F});
    engine::physics::integrate(world, 0.5);

    const auto& simulated = engine::physics::body_at(world, index);
    EXPECT_LT(simulated.velocity[0], 1.0F);
    EXPECT_GT(simulated.velocity[0], 0.0F);
    EXPECT_NEAR(simulated.position[0], 0.2F, 0.1F);
    EXPECT_EQ((engine::math::vec3{0.0F, 0.0F, 0.0F}), simulated.accumulated_force);
}

TEST(PhysicsWorldColliders, CapsuleIntersectionsAreDetected) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody capsule_body;
    capsule_body.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto capsule_index = engine::physics::add_body(world, capsule_body);
    engine::physics::Collider::Capsule capsule_shape;
    capsule_shape.point_a = engine::math::vec3{0.0F, -0.5F, 0.0F};
    capsule_shape.point_b = engine::math::vec3{0.0F, 0.5F, 0.0F};
    capsule_shape.radius = 0.25F;
    engine::physics::set_collider(world, capsule_index, engine::physics::Collider::make_capsule(capsule_shape));

    engine::physics::RigidBody sphere_body;
    sphere_body.position = engine::math::vec3{0.2F, 0.5F, 0.0F};
    const auto sphere_index = engine::physics::add_body(world, sphere_body);
    engine::physics::set_collider(world, sphere_index, engine::physics::Collider::make_sphere(0.5F));

    auto collisions = engine::physics::detect_collisions(world);
    ASSERT_EQ(1U, collisions.size());
    const auto first_pair = collisions[0];
    EXPECT_TRUE((first_pair.first == capsule_index && first_pair.second == sphere_index) ||
                (first_pair.first == sphere_index && first_pair.second == capsule_index));

    engine::physics::RigidBody second_capsule_body;
    second_capsule_body.position = engine::math::vec3{0.0F, 2.0F, 0.0F};
    const auto second_capsule_index = engine::physics::add_body(world, second_capsule_body);
    engine::physics::set_collider(world, second_capsule_index, engine::physics::Collider::make_capsule(capsule_shape));

    collisions = engine::physics::detect_collisions(world);
    ASSERT_EQ(1U, collisions.size());
    const auto second_pair = collisions[0];
    EXPECT_TRUE((second_pair.first == capsule_index && second_pair.second == sphere_index) ||
                (second_pair.first == sphere_index && second_pair.second == capsule_index));
}

TEST(PhysicsWorldContacts, GeneratesPersistentSphereSphereManifold) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody first;
    first.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto first_index = engine::physics::add_body(world, first);
    engine::physics::set_collider(world, first_index, engine::physics::Collider::make_sphere(1.0F));

    engine::physics::RigidBody second;
    second.position = engine::math::vec3{1.5F, 0.0F, 0.0F};
    const auto second_index = engine::physics::add_body(world, second);
    engine::physics::set_collider(world, second_index, engine::physics::Collider::make_sphere(1.0F));

    engine::physics::update_contact_manifolds(world);

    const auto& manifolds = engine::physics::contact_manifolds(world);
    ASSERT_EQ(1U, manifolds.size());
    const auto& manifold = manifolds[0];
    EXPECT_EQ(first_index, manifold.first);
    EXPECT_EQ(second_index, manifold.second);
    ASSERT_EQ(1U, manifold.contact_count);
    const auto& contact = manifold.contacts[0];
    EXPECT_NEAR(0.5F, contact.penetration, 1e-4F);
    EXPECT_GT(contact.normal[0], 0.0F);

    const auto& telemetry = engine::physics::collision_telemetry(world);
    EXPECT_EQ(1U, telemetry.manifold_count);
    EXPECT_EQ(1U, telemetry.contact_count);
    EXPECT_NEAR(0.5F, telemetry.max_penetration, 1e-4F);

    // Move the second body slightly while maintaining overlap.
    engine::physics::body_at(world, second_index).position = engine::math::vec3{1.4F, 0.0F, 0.0F};
    engine::physics::update_contact_manifolds(world);
    ASSERT_EQ(1U, engine::physics::contact_manifolds(world).size());
    EXPECT_GT(engine::physics::contact_manifolds(world)[0].lifetime, 0U);

    // Separate the pair and ensure the manifold clears.
    engine::physics::body_at(world, second_index).position = engine::math::vec3{3.5F, 0.0F, 0.0F};
    engine::physics::update_contact_manifolds(world);
    EXPECT_TRUE(engine::physics::contact_manifolds(world).empty());
    const auto& cleared = engine::physics::collision_telemetry(world);
    EXPECT_EQ(0U, cleared.manifold_count);
    EXPECT_EQ(0U, cleared.contact_count);
    EXPECT_FLOAT_EQ(0.0F, cleared.max_penetration);
}

TEST(PhysicsWorldContacts, ConstraintCallbacksReceiveManifolds) {
    engine::physics::PhysicsWorld world;

    engine::physics::RigidBody box;
    box.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    const auto box_index = engine::physics::add_body(world, box);
    const auto local_box = engine::geometry::MakeAabbFromCenterExtent({0.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    engine::physics::set_collider(world, box_index, engine::physics::Collider::make_aabb(local_box));

    engine::physics::RigidBody capsule;
    capsule.position = engine::math::vec3{0.4F, 0.5F, 0.0F};
    const auto capsule_index = engine::physics::add_body(world, capsule);
    engine::physics::Collider::Capsule capsule_shape;
    capsule_shape.point_a = engine::math::vec3{0.0F, -0.5F, 0.0F};
    capsule_shape.point_b = engine::math::vec3{0.0F, 0.5F, 0.0F};
    capsule_shape.radius = 0.25F;
    engine::physics::set_collider(world, capsule_index, engine::physics::Collider::make_capsule(capsule_shape));

    struct CallbackState {
        std::size_t count{0U};
        float last_penetration{0.0F};
    } state;

    engine::physics::ConstraintSolverCallbacks callbacks{};
    callbacks.on_manifold = [](engine::physics::PhysicsWorld&, const engine::physics::ContactManifold& manifold, void* user) {
        auto* state = static_cast<CallbackState*>(user);
        ++state->count;
        if (manifold.contact_count > 0U) {
            state->last_penetration = manifold.contacts[0].penetration;
        }
    };
    callbacks.user_data = &state;
    engine::physics::set_constraint_callbacks(world, callbacks);

    engine::physics::update_contact_manifolds(world);

    EXPECT_EQ(1U, state.count);
    EXPECT_GT(state.last_penetration, 0.0F);
}
