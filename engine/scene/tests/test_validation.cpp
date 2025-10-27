#include <gtest/gtest.h>

#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"
#include "engine/scene/validation.hpp"
#include "engine/scene/components/transform.hpp"

#include "engine/math/transform.hpp"

#include <limits>

namespace scene = engine::scene;
namespace components = engine::scene::components;
namespace systems = engine::scene::systems;
namespace validation = engine::scene::validation;

namespace
{
    void force_cycle(scene::Scene& scene_instance, scene::Entity a, scene::Entity b)
    {
        auto& registry = scene_instance.registry();
        registry.emplace_or_replace<components::Hierarchy>(a.id());
        registry.emplace_or_replace<components::Hierarchy>(b.id());

        auto& a_hierarchy = registry.get<components::Hierarchy>(a.id());
        auto& b_hierarchy = registry.get<components::Hierarchy>(b.id());
        a_hierarchy.parent = b.id();
        b_hierarchy.parent = a.id();
    }
}

TEST(SceneValidation, ReportsCleanHierarchyWithoutIssues)
{
    scene::Scene scene_instance{};
    auto root = scene_instance.create_entity("root");
    auto child = scene_instance.create_entity("child");

    auto& registry = scene_instance.registry();
    registry.emplace<components::LocalTransform>(root.id());
    registry.emplace<components::LocalTransform>(child.id());
    systems::set_parent(registry, child.id(), root.id());
    systems::mark_subtree_dirty(registry, root.id());
    systems::propagate_transforms(registry);

    const auto report = validation::validate_hierarchy(scene_instance);
    EXPECT_TRUE(report.ok()) << "Expected no validation issues";
    EXPECT_TRUE(report.issues.empty());
}

TEST(SceneValidation, DetectsCycleIntroducedOutOfBand)
{
    scene::Scene scene_instance{};
    auto a = scene_instance.create_entity("A");
    auto b = scene_instance.create_entity("B");

    auto& registry = scene_instance.registry();
    registry.emplace<components::LocalTransform>(a.id());
    registry.emplace<components::LocalTransform>(b.id());

    force_cycle(scene_instance, a, b);

    const auto report = validation::validate_hierarchy(scene_instance);
    EXPECT_FALSE(report.ok());
    EXPECT_GT(report.metrics.cycle_count, 0U);
}

TEST(SceneValidation, DetectsDanglingParent)
{
    scene::Scene scene_instance{};
    auto root = scene_instance.create_entity("root");
    auto child = scene_instance.create_entity("child");

    auto& registry = scene_instance.registry();
    registry.emplace<components::LocalTransform>(root.id());
    registry.emplace<components::LocalTransform>(child.id());

    systems::set_parent(registry, child.id(), root.id());
    auto& hierarchy = registry.get<components::Hierarchy>(child.id());
    hierarchy.parent = static_cast<entt::entity>(9999);

    const auto report = validation::validate_hierarchy(scene_instance);
    EXPECT_FALSE(report.ok());
    EXPECT_GT(report.metrics.dangling_parent_count, 0U);
}

TEST(SceneValidation, DetectsTransformMismatchWhenNotDirty)
{
    scene::Scene scene_instance{};
    auto root = scene_instance.create_entity("root");
    auto child = scene_instance.create_entity("child");

    auto& registry = scene_instance.registry();
    auto& root_local = registry.emplace<components::LocalTransform>(root.id());
    root_local.value.translation = engine::math::Vector < float, 3 >
    {
        1.0F, 0.0F, 0.0F
    };
    auto& child_local = registry.emplace<components::LocalTransform>(child.id());
    child_local.value.translation = engine::math::Vector < float, 3 >
    {
        0.0F, 2.0F, 0.0F
    };

    systems::set_parent(registry, child.id(), root.id());
    systems::mark_subtree_dirty(registry, root.id());
    systems::propagate_transforms(registry);

    auto& child_world = registry.get<components::WorldTransform>(child.id());
    child_world.value.translation[1] += 1.0F; // Break propagated value.

    const auto report = validation::validate_hierarchy(scene_instance);
    EXPECT_FALSE(report.ok());
    EXPECT_GT(report.metrics.transform_mismatch_count, 0U);
}

TEST(SceneValidation, SkipsDirtyEntitiesWhenConfigured)
{
    scene::Scene scene_instance{};
    auto root = scene_instance.create_entity("root");
    auto child = scene_instance.create_entity("child");

    auto& registry = scene_instance.registry();
    registry.emplace<components::LocalTransform>(root.id());
    registry.emplace<components::LocalTransform>(child.id());
    systems::set_parent(registry, child.id(), root.id());
    systems::mark_subtree_dirty(registry, root.id());

    auto& child_world = registry.emplace<components::WorldTransform>(child.id());
    child_world.value.translation[0] = 42.0F;

    validation::HierarchyValidationOptions options{};
    options.skip_dirty_entities = true;

    const auto report = validation::validate_hierarchy(scene_instance, options);
    EXPECT_TRUE(report.metrics.transform_mismatch_count == 0U);
}

TEST(SceneValidation, DetectsNonFiniteTransforms)
{
    scene::Scene scene_instance{};
    auto entity = scene_instance.create_entity("bad");

    auto& registry = scene_instance.registry();
    auto& local = registry.emplace<components::LocalTransform>(entity.id());
    local.value.translation[0] = std::numeric_limits<float>::quiet_NaN();
    auto& world = registry.emplace<components::WorldTransform>(entity.id());
    world.value.scale[0] = std::numeric_limits<float>::infinity();

    const auto report = validation::validate_hierarchy(scene_instance);
    EXPECT_FALSE(report.ok());
    EXPECT_GE(report.metrics.non_finite_transform_count, 2U);
}