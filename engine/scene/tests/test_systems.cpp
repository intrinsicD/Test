#include <gtest/gtest.h>

#include "engine/math/transform.hpp"
#include "engine/math/vector.hpp"

#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"

namespace scene = engine::scene;
namespace components = engine::scene::components;
namespace systems = engine::scene::systems;

TEST(SceneSystems, PropagateTransformsCombinesHierarchy) {
    scene::Scene scene;

    auto parent = scene.create_entity();
    auto child = scene.create_entity();

    auto& registry = scene.registry();

    auto& parent_local = registry.emplace<components::LocalTransform>(parent.id());
    parent_local.value.translation = engine::math::Vector<float, 3>{1.0F, 0.0F, 0.0F};
    systems::mark_transform_dirty(registry, parent.id());

    auto& child_local = registry.emplace<components::LocalTransform>(child.id());
    child_local.value.translation = engine::math::Vector<float, 3>{0.0F, 2.0F, 0.0F};
    systems::mark_transform_dirty(registry, child.id());

    systems::set_parent(registry, child.id(), parent.id());

    systems::propagate_transforms(registry);

    const auto& parent_world = registry.get<components::WorldTransform>(parent.id());
    EXPECT_FLOAT_EQ(parent_world.value.translation[0], 1.0F);
    EXPECT_FLOAT_EQ(parent_world.value.translation[1], 0.0F);

    const auto& child_world = registry.get<components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world.value.translation[0], 1.0F);
    EXPECT_FLOAT_EQ(child_world.value.translation[1], 2.0F);
}

TEST(SceneSystems, UpdatingLocalTransformPropagatesToChildren) {
    scene::Scene scene;

    auto parent = scene.create_entity();
    auto child = scene.create_entity();

    auto& registry = scene.registry();

    registry.emplace<components::LocalTransform>(parent.id());
    registry.emplace<components::LocalTransform>(child.id());
    systems::mark_transform_dirty(registry, parent.id());
    systems::mark_transform_dirty(registry, child.id());

    systems::set_parent(registry, child.id(), parent.id());
    systems::propagate_transforms(registry);

    auto& updated_parent = registry.get<components::LocalTransform>(parent.id());
    updated_parent.value.translation = engine::math::Vector<float, 3>{5.0F, -1.0F, 0.0F};
    systems::mark_subtree_dirty(registry, parent.id());

    systems::propagate_transforms(registry);

    const auto& parent_world = registry.get<components::WorldTransform>(parent.id());
    EXPECT_FLOAT_EQ(parent_world.value.translation[0], 5.0F);
    EXPECT_FLOAT_EQ(parent_world.value.translation[1], -1.0F);

    const auto& child_world = registry.get<components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world.value.translation[0], 5.0F);
    EXPECT_FLOAT_EQ(child_world.value.translation[1], -1.0F);
}

TEST(SceneSystems, ReparentingUpdatesWorldTransform) {
    scene::Scene scene;

    auto root = scene.create_entity();
    auto old_parent = scene.create_entity();
    auto new_parent = scene.create_entity();
    auto child = scene.create_entity();

    auto& registry = scene.registry();

    registry.emplace<components::LocalTransform>(root.id());
    systems::mark_transform_dirty(registry, root.id());

    auto& old_parent_local = registry.emplace<components::LocalTransform>(old_parent.id());
    old_parent_local.value.translation = engine::math::Vector<float, 3>{1.0F, 0.0F, 0.0F};
    systems::mark_transform_dirty(registry, old_parent.id());

    auto& new_parent_local = registry.emplace<components::LocalTransform>(new_parent.id());
    new_parent_local.value.translation = engine::math::Vector<float, 3>{0.0F, 3.0F, 0.0F};
    systems::mark_transform_dirty(registry, new_parent.id());

    registry.emplace<components::LocalTransform>(child.id());
    systems::mark_transform_dirty(registry, child.id());

    systems::set_parent(registry, old_parent.id(), root.id());
    systems::set_parent(registry, new_parent.id(), root.id());
    systems::set_parent(registry, child.id(), old_parent.id());

    systems::propagate_transforms(registry);

    systems::set_parent(registry, child.id(), new_parent.id());
    systems::propagate_transforms(registry);

    const auto& child_world = registry.get<components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world.value.translation[0], 0.0F);
    EXPECT_FLOAT_EQ(child_world.value.translation[1], 3.0F);
}
