#include <gtest/gtest.h>

#include <vector>

#include "engine/core/ecs/component_storage.hpp"
#include "engine/core/ecs/registry.hpp"
#include "engine/core/ecs/system.hpp"

namespace {

struct position {
    float x{};
    float y{};
    float z{};
};

struct velocity {
    float vx{};
    float vy{};
    float vz{};
};

}  // namespace

namespace ecs = engine::core::ecs;

TEST(EcsRegistry, EntityLifetime) {
    ecs::registry registry;

    const auto first = registry.create();
    ASSERT_TRUE(registry.is_alive(first));

    const auto first_index = first.index();
    registry.destroy(first);
    EXPECT_FALSE(registry.is_alive(first));

    const auto second = registry.create();
    EXPECT_TRUE(registry.is_alive(second));
    EXPECT_EQ(first_index, second.index());
    EXPECT_TRUE(first != second);
    EXPECT_GT(second.generation(), first.generation());
}

TEST(EcsRegistry, ComponentAddRemove) {
    ecs::registry registry;
    const auto entity = registry.create();

    auto& pos = registry.emplace<position>(entity, position{1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_TRUE(registry.contains<position>(entity));

    pos.y = -4.0f;
    const auto& fetched = registry.get<position>(entity);
    EXPECT_FLOAT_EQ(fetched.y, -4.0f);

    ecs::component_storage<velocity> velocities{registry};
    velocities.emplace(entity, velocity{0.5f, 0.0f, 1.0f});
    ASSERT_TRUE(velocities.try_get(entity) != nullptr);

    registry.remove<position>(entity);
    EXPECT_FALSE(registry.contains<position>(entity));
    EXPECT_EQ(registry.try_get<position>(entity), nullptr);

    velocities.remove(entity);
    EXPECT_FALSE(velocities.contains(entity));
}

TEST(EcsRegistry, MultiComponentIterationOrder) {
    ecs::registry registry;

    const auto e1 = registry.create();
    const auto e2 = registry.create();
    const auto e3 = registry.create();

    registry.emplace<position>(e1, position{1.0f, 0.0f, 0.0f});
    registry.emplace<velocity>(e1, velocity{0.0f, 0.0f, 0.0f});

    registry.emplace<position>(e2, position{2.0f, 0.0f, 0.0f});
    registry.emplace<velocity>(e2, velocity{1.0f, 0.0f, 0.0f});

    registry.emplace<position>(e3, position{3.0f, 0.0f, 0.0f});

    std::vector<ecs::entity_id> visited;
    for (auto&& [entity, pos, vel] : registry.view<position, velocity>()) {
        visited.push_back(entity);
        vel.vx += pos.x;
    }

    ASSERT_EQ(visited.size(), 2U);
    EXPECT_EQ(visited[0], e1);
    EXPECT_EQ(visited[1], e2);

    const auto& vel1 = registry.get<velocity>(e1);
    EXPECT_FLOAT_EQ(vel1.vx, 1.0f);
    const auto& vel2 = registry.get<velocity>(e2);
    EXPECT_FLOAT_EQ(vel2.vx, 3.0f);
}

TEST(EcsRegistry, SchedulerExecutesSystems) {
    ecs::registry registry;
    ecs::system_scheduler scheduler;

    int invoke_count = 0;
    scheduler.add_lambda_system("increment", [&](ecs::registry& reg, double dt) {
        ++invoke_count;
        EXPECT_NEAR(dt, 0.016, 1e-6);
        auto entity = reg.create();
        reg.emplace<position>(entity, position{0.0f, 1.0f, 0.0f});
    });

    scheduler.tick(registry, 0.016);

    EXPECT_EQ(invoke_count, 1);
    EXPECT_EQ(registry.alive_count(), 1U);

    draw_registry_debug_ui(registry, "Scheduler Debug");
}

