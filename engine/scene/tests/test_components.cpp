#include <gtest/gtest.h>

#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"

#include <entt/entt.hpp>

TEST(SceneComponents, NameStoresText) {
    engine::scene::components::Name name{.value = "example"};
    EXPECT_EQ(engine::scene::components::view(name), "example");
    EXPECT_TRUE(name == "example");
    EXPECT_TRUE("example" == name);
}

TEST(SceneComponents, HierarchyParentChildRelationships) {
    engine::scene::Scene scene;

    auto parent = scene.create_entity();
    auto child = scene.create_entity();

    auto& registry = scene.registry();

    engine::scene::systems::set_parent(registry, child.id(), parent.id());

    const auto& parent_hierarchy = registry.get<engine::scene::components::Hierarchy>(parent.id());
    EXPECT_TRUE(engine::scene::components::has_children(parent_hierarchy));
    EXPECT_EQ(parent_hierarchy.first_child, child.id());

    const auto& child_hierarchy = registry.get<engine::scene::components::Hierarchy>(child.id());
    EXPECT_EQ(child_hierarchy.parent, parent.id());
    EXPECT_TRUE(engine::scene::components::is_root(parent_hierarchy));

    engine::scene::systems::detach_from_parent(registry, child.id());

    const auto& detached = registry.get<engine::scene::components::Hierarchy>(child.id());
    EXPECT_EQ(detached.parent, entt::null);
}
