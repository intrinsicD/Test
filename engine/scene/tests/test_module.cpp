#include <gtest/gtest.h>

#include "engine/math/vector.hpp"

#include "engine/scene/api.hpp"
#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"

TEST(SceneModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::scene::module_name(), "scene");
    EXPECT_STREQ(engine_scene_module_name(), "scene");
}

namespace {

struct Position {
    float x{};
    float y{};
    float z{};
};

}  // namespace


TEST(Scene, CreateAndManipulateEntity) {
    engine::scene::Scene scene{"test"};
    EXPECT_EQ(scene.name(), "test");

    auto entity = scene.create_entity();
    EXPECT_TRUE(entity.valid());
    EXPECT_TRUE(scene.valid(entity.id()));

    auto& position = entity.emplace<Position>(1.0F, 2.0F, 3.0F);
    EXPECT_FLOAT_EQ(position.x, 1.0F);
    EXPECT_FLOAT_EQ(position.y, 2.0F);
    EXPECT_FLOAT_EQ(position.z, 3.0F);

    const auto& const_position = entity.get<Position>();
    EXPECT_FLOAT_EQ(const_position.x, 1.0F);

    EXPECT_TRUE(entity.has<Position>());

    auto view = scene.view<Position>();
    ASSERT_EQ(view.size(), 1);

    std::size_t count = 0;
    for (auto [id, pos] : view.each()) {
        EXPECT_EQ(id, entity.id());
        EXPECT_FLOAT_EQ(pos.y, 2.0F);
        ++count;
    }
    EXPECT_EQ(count, 1U);

    entity.remove<Position>();
    EXPECT_FALSE(entity.has<Position>());

    entity.destroy();
    EXPECT_FALSE(entity.valid());
}

TEST(Scene, CreateNamedEntityAndUpdateHierarchy) {
    engine::scene::Scene scene;

    auto parent = scene.create_entity("parent");
    auto child = scene.create_entity("child");

    EXPECT_TRUE(parent.has<engine::scene::components::Name>());
    EXPECT_EQ(parent.get<engine::scene::components::Name>().value, "parent");

    auto& registry = scene.registry();
    auto& parent_local = registry.emplace<engine::scene::components::LocalTransform>(parent.id());
    parent_local.value.translation = engine::math::Vector<float, 3>{1.0F, 0.0F, 0.0F};
    auto& child_local = registry.emplace<engine::scene::components::LocalTransform>(child.id());
    child_local.value.translation = engine::math::Vector<float, 3>{0.0F, 1.0F, 0.0F};

    engine::scene::systems::mark_transform_dirty(registry, parent.id());
    engine::scene::systems::mark_transform_dirty(registry, child.id());

    scene.update();

    const auto baseline_world = registry.get<engine::scene::components::WorldTransform>(child.id()).value;
    EXPECT_FLOAT_EQ(baseline_world.translation[0], 0.0F);
    EXPECT_FLOAT_EQ(baseline_world.translation[1], 1.0F);
    EXPECT_FLOAT_EQ(baseline_world.translation[2], 0.0F);

    child.set_parent(parent);

    const auto& child_local_after_reparent = registry.get<engine::scene::components::LocalTransform>(child.id());
    EXPECT_FLOAT_EQ(child_local_after_reparent.value.translation[0], -1.0F);
    EXPECT_FLOAT_EQ(child_local_after_reparent.value.translation[1], 1.0F);
    EXPECT_FLOAT_EQ(child_local_after_reparent.value.translation[2], 0.0F);

    scene.update();

    auto parent_of_child = child.parent();
    ASSERT_TRUE(parent_of_child.valid());
    EXPECT_EQ(parent_of_child.id(), parent.id());

    const auto& child_world = registry.get<engine::scene::components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world.value.translation[0], baseline_world.translation[0]);
    EXPECT_FLOAT_EQ(child_world.value.translation[1], baseline_world.translation[1]);
    EXPECT_FLOAT_EQ(child_world.value.translation[2], baseline_world.translation[2]);

    child.detach_from_parent();

    const auto& child_local_after_detach = registry.get<engine::scene::components::LocalTransform>(child.id());
    EXPECT_FLOAT_EQ(child_local_after_detach.value.translation[0], baseline_world.translation[0]);
    EXPECT_FLOAT_EQ(child_local_after_detach.value.translation[1], baseline_world.translation[1]);
    EXPECT_FLOAT_EQ(child_local_after_detach.value.translation[2], baseline_world.translation[2]);

    scene.update();

    parent_of_child = child.parent();
    EXPECT_FALSE(parent_of_child.valid());

    const auto& child_world_after_detach = registry.get<engine::scene::components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world_after_detach.value.translation[0], baseline_world.translation[0]);
    EXPECT_FLOAT_EQ(child_world_after_detach.value.translation[1], baseline_world.translation[1]);
    EXPECT_FLOAT_EQ(child_world_after_detach.value.translation[2], baseline_world.translation[2]);
}
