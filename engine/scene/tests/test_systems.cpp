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

    const auto expected_child_world = child_local.value;
    const auto expected_child_local = engine::math::combine(engine::math::inverse(parent_local.value), expected_child_world);

    systems::set_parent(registry, child.id(), parent.id());

    const auto& child_local_after_reparent = registry.get<components::LocalTransform>(child.id());
    EXPECT_FLOAT_EQ(child_local_after_reparent.value.translation[0], expected_child_local.translation[0]);
    EXPECT_FLOAT_EQ(child_local_after_reparent.value.translation[1], expected_child_local.translation[1]);

    systems::propagate_transforms(registry);

    const auto& parent_world = registry.get<components::WorldTransform>(parent.id());
    EXPECT_FLOAT_EQ(parent_world.value.translation[0], 1.0F);
    EXPECT_FLOAT_EQ(parent_world.value.translation[1], 0.0F);

    const auto& child_world = registry.get<components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world.value.translation[0], expected_child_world.translation[0]);
    EXPECT_FLOAT_EQ(child_world.value.translation[1], expected_child_world.translation[1]);
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

TEST(SceneSystems, ReparentingPreservesWorldTransform) {
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

    auto& child_local = registry.emplace<components::LocalTransform>(child.id());
    child_local.value.translation = engine::math::Vector<float, 3>{2.0F, 1.0F, 0.0F};
    systems::mark_transform_dirty(registry, child.id());

    systems::set_parent(registry, old_parent.id(), root.id());
    systems::set_parent(registry, new_parent.id(), root.id());
    systems::set_parent(registry, child.id(), old_parent.id());

    systems::propagate_transforms(registry);

    const auto baseline_world = registry.get<components::WorldTransform>(child.id()).value;
    const auto new_parent_world = registry.get<components::WorldTransform>(new_parent.id()).value;
    const auto expected_local = engine::math::combine(engine::math::inverse(new_parent_world), baseline_world);

    systems::set_parent(registry, child.id(), new_parent.id());

    const auto& local_after_reparent = registry.get<components::LocalTransform>(child.id());
    EXPECT_FLOAT_EQ(local_after_reparent.value.translation[0], expected_local.translation[0]);
    EXPECT_FLOAT_EQ(local_after_reparent.value.translation[1], expected_local.translation[1]);
    EXPECT_FLOAT_EQ(local_after_reparent.value.translation[2], expected_local.translation[2]);

    systems::propagate_transforms(registry);

    const auto& child_world = registry.get<components::WorldTransform>(child.id());
    EXPECT_FLOAT_EQ(child_world.value.translation[0], baseline_world.translation[0]);
    EXPECT_FLOAT_EQ(child_world.value.translation[1], baseline_world.translation[1]);
    EXPECT_FLOAT_EQ(child_world.value.translation[2], baseline_world.translation[2]);
}

TEST(SceneSystems, PropagateTransformsUpdatesOnlyDirtyEntities) {
    scene::Scene scene;

    auto root = scene.create_entity();
    auto dirty_child = scene.create_entity();
    auto clean_child = scene.create_entity();

    auto& registry = scene.registry();

    auto& root_local = registry.emplace<components::LocalTransform>(root.id());
    root_local.value.translation = engine::math::Vector<float, 3>{2.0F, 0.0F, 0.0F};

    auto& dirty_local = registry.emplace<components::LocalTransform>(dirty_child.id());
    dirty_local.value.translation = engine::math::Vector<float, 3>{0.0F, 1.0F, 0.0F};

    auto& clean_local = registry.emplace<components::LocalTransform>(clean_child.id());
    clean_local.value.translation = engine::math::Vector<float, 3>{0.0F, -2.0F, 0.0F};

    systems::mark_transform_dirty(registry, root.id());
    systems::mark_transform_dirty(registry, dirty_child.id());
    systems::mark_transform_dirty(registry, clean_child.id());

    systems::set_parent(registry, dirty_child.id(), root.id());
    systems::set_parent(registry, clean_child.id(), root.id());

    systems::propagate_transforms(registry);

    const auto baseline_root = registry.get<components::WorldTransform>(root.id()).value;
    const auto baseline_dirty = registry.get<components::WorldTransform>(dirty_child.id()).value;
    const auto baseline_clean = registry.get<components::WorldTransform>(clean_child.id()).value;

    dirty_local.value.translation = engine::math::Vector<float, 3>{0.0F, 4.0F, 0.0F};
    systems::mark_transform_dirty(registry, dirty_child.id());

    systems::propagate_transforms(registry);

    const auto& root_world = registry.get<components::WorldTransform>(root.id());
    EXPECT_FLOAT_EQ(root_world.value.translation[0], baseline_root.translation[0]);
    EXPECT_FLOAT_EQ(root_world.value.translation[1], baseline_root.translation[1]);
    EXPECT_FLOAT_EQ(root_world.value.translation[2], baseline_root.translation[2]);
    EXPECT_FALSE(registry.any_of<components::DirtyTransform>(root.id()));

    const auto& dirty_world = registry.get<components::WorldTransform>(dirty_child.id());
    EXPECT_GT(std::fabs(dirty_world.value.translation[1] - baseline_dirty.translation[1]), 0.0f);
    EXPECT_FLOAT_EQ(dirty_world.value.translation[0], baseline_root.translation[0] + dirty_local.value.translation[0]);
    EXPECT_FLOAT_EQ(dirty_world.value.translation[1], baseline_root.translation[1] + dirty_local.value.translation[1]);
    EXPECT_FLOAT_EQ(dirty_world.value.translation[2], baseline_root.translation[2] + dirty_local.value.translation[2]);
    EXPECT_FALSE(registry.any_of<components::DirtyTransform>(dirty_child.id()));

    const auto& clean_world = registry.get<components::WorldTransform>(clean_child.id());
    EXPECT_FLOAT_EQ(clean_world.value.translation[0], baseline_clean.translation[0]);
    EXPECT_FLOAT_EQ(clean_world.value.translation[1], baseline_clean.translation[1]);
    EXPECT_FLOAT_EQ(clean_world.value.translation[2], baseline_clean.translation[2]);
    EXPECT_FALSE(registry.any_of<components::DirtyTransform>(clean_child.id()));
}
