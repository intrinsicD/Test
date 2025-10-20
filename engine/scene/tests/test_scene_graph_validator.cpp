#include <gtest/gtest.h>

#include "engine/scene/graph/scene_graph_validator.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"
#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/components/transform.hpp"

#include <string>

namespace scene = engine::scene;
namespace graph = engine::scene::graph;
namespace systems = engine::scene::systems;
namespace components = engine::scene::components;

TEST(SceneGraphValidator, ReportsAcyclicGraphAsValid)
{
    scene::Scene scene_instance{};
    auto root = scene_instance.create_entity("root");
    auto child = scene_instance.create_entity("child");

    auto& registry = scene_instance.registry();
    registry.emplace<components::Hierarchy>(root.id());
    registry.emplace<components::Hierarchy>(child.id());
    systems::set_parent(registry, child.id(), root.id());

    const auto result = graph::validate(scene_instance);
    EXPECT_TRUE(result) << "Expected validator to accept acyclic hierarchy";
}

TEST(SceneGraphValidator, DetectsCycleBetweenEntities)
{
    scene::Scene scene_instance{};
    auto a = scene_instance.create_entity("A");
    auto b = scene_instance.create_entity("B");

    auto& registry = scene_instance.registry();
    registry.emplace<components::Hierarchy>(a.id());
    registry.emplace<components::Hierarchy>(b.id());

    auto& a_hierarchy = registry.get<components::Hierarchy>(a.id());
    auto& b_hierarchy = registry.get<components::Hierarchy>(b.id());
    a_hierarchy.parent = b.id();
    b_hierarchy.parent = a.id();

    const auto result = graph::validate(scene_instance);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scene::SceneGraphError::cycle_detected);
    EXPECT_NE(result.error().message().find("Cycle detected"), std::string::npos);
}

TEST(SceneGraphValidator, DetectsSelfCycle)
{
    scene::Scene scene_instance{};
    auto entity = scene_instance.create_entity("self");

    auto& registry = scene_instance.registry();
    registry.emplace<components::Hierarchy>(entity.id());
    auto& hierarchy = registry.get<components::Hierarchy>(entity.id());
    hierarchy.parent = entity.id();

    const auto result = graph::validate(scene_instance);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scene::SceneGraphError::cycle_detected);
}
