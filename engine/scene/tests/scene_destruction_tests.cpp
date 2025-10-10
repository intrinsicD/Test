#include <gtest/gtest.h>

#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"

#include <entt/entt.hpp>

namespace
{
    using engine::scene::Scene;
    using engine::scene::components::DirtyTransform;
    using engine::scene::components::Hierarchy;
}

TEST(SceneDestruction, DestroyParentDetachesChildren)
{
    Scene scene;

    auto parent = scene.create_entity();
    auto child_a = scene.create_entity();
    auto child_b = scene.create_entity();

    auto& registry = scene.registry();

    engine::scene::systems::set_parent(registry, child_a.id(), parent.id());
    engine::scene::systems::set_parent(registry, child_b.id(), parent.id());

    ASSERT_EQ(scene.size(), 3);

    scene.destroy_entity(parent);

    EXPECT_FALSE(scene.valid(parent.id()));
    EXPECT_EQ(scene.size(), 2);

    ASSERT_TRUE(scene.valid(child_a.id()));
    ASSERT_TRUE(scene.valid(child_b.id()));

    const auto& hierarchy_a = registry.get<Hierarchy>(child_a.id());
    EXPECT_EQ(hierarchy_a.parent, entt::null);
    EXPECT_EQ(hierarchy_a.previous_sibling, entt::null);
    EXPECT_EQ(hierarchy_a.next_sibling, entt::null);

    const auto& hierarchy_b = registry.get<Hierarchy>(child_b.id());
    EXPECT_EQ(hierarchy_b.parent, entt::null);
    EXPECT_EQ(hierarchy_b.previous_sibling, entt::null);
    EXPECT_EQ(hierarchy_b.next_sibling, entt::null);

    EXPECT_TRUE(registry.any_of<DirtyTransform>(child_a.id()));
    EXPECT_TRUE(registry.any_of<DirtyTransform>(child_b.id()));
}

TEST(SceneDestruction, DestroyForeignEntityIsNoOp)
{
    Scene first_scene;
    Scene second_scene;

    auto foreign_entity = first_scene.create_entity();

    ASSERT_EQ(first_scene.size(), 1);
    ASSERT_EQ(second_scene.size(), 0);
    ASSERT_TRUE(foreign_entity.valid());

    second_scene.destroy_entity(foreign_entity);

    EXPECT_TRUE(foreign_entity.valid());
    EXPECT_EQ(first_scene.size(), 1);
    EXPECT_EQ(second_scene.size(), 0);
}
