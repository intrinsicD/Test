#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"

#include "gtest/gtest.h"

#include <array>
#include <sstream>
#include <string>
#include <unordered_map>

namespace engine::scene::tests
{
    namespace
    {
        using engine::scene::components::Hierarchy;
        using engine::scene::components::LocalTransform;
        using engine::scene::components::Name;
        using engine::scene::components::WorldTransform;
        using engine::scene::components::DirtyTransform;

        [[nodiscard]] engine::math::Transform<float> make_transform(const std::array<float, 3>& scale,
                                                                    const std::array<float, 4>& rotation,
                                                                    const std::array<float, 3>& translation)
        {
            engine::math::Transform<float> transform{};
            transform.scale = engine::math::Vector<float, 3>{scale[0], scale[1], scale[2]};
            transform.rotation = engine::math::Quaternion<float>{rotation[0], rotation[1], rotation[2], rotation[3]};
            transform.translation = engine::math::Vector<float, 3>{translation[0], translation[1], translation[2]};
            return transform;
        }
    } // namespace

    TEST(SceneSerialization, RoundTripScene)
    {
        Scene original{"RoundTrip"};

        auto root = original.create_entity();
        auto child = original.create_entity();

        root.emplace<Name>(Name{"Root"});
        child.emplace<Name>(Name{"Child"});

        Hierarchy root_hierarchy{};
        root_hierarchy.parent = entt::null;
        root_hierarchy.first_child = child.id();
        root_hierarchy.next_sibling = entt::null;
        root_hierarchy.previous_sibling = entt::null;
        original.registry().emplace<Hierarchy>(root.id(), root_hierarchy);

        Hierarchy child_hierarchy{};
        child_hierarchy.parent = root.id();
        child_hierarchy.first_child = entt::null;
        child_hierarchy.next_sibling = entt::null;
        child_hierarchy.previous_sibling = entt::null;
        original.registry().emplace<Hierarchy>(child.id(), child_hierarchy);

        LocalTransform root_local{};
        root_local.value = make_transform({1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.5f, 0.75f}, {10.0f, 0.0f, -5.0f});
        original.registry().emplace<LocalTransform>(root.id(), root_local);

        WorldTransform root_world{};
        root_world.value = make_transform({0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        original.registry().emplace<WorldTransform>(root.id(), root_world);

        original.registry().emplace<DirtyTransform>(root.id());

        LocalTransform child_local{};
        child_local.value = make_transform({0.25f, 0.75f, 1.25f}, {0.5f, 0.5f, 0.25f, 0.75f}, {2.0f, 3.0f, 4.0f});
        original.registry().emplace<LocalTransform>(child.id(), child_local);

        std::stringstream buffer;
        original.save(buffer);

        Scene restored{"placeholder"};
        restored.load(buffer);

        EXPECT_EQ(std::string(restored.name()), "RoundTrip");
        EXPECT_EQ(restored.registry().alive_count(), 2u);

        std::unordered_map<std::string, entt::entity> entities_by_name;
        auto name_view = restored.registry().view<Name>();
        for (auto it = name_view.begin(); it != name_view.end(); ++it)
        {
            auto [entity, name] = *it;
            entities_by_name.emplace(name.value, entity);
        }

        ASSERT_EQ(entities_by_name.size(), 2u);
        const auto root_entity = entities_by_name.at("Root");
        const auto child_entity = entities_by_name.at("Child");

        const auto& restored_root_hierarchy = restored.registry().get<Hierarchy>(root_entity);
        EXPECT_EQ(restored_root_hierarchy.parent, entt::null);
        EXPECT_EQ(restored_root_hierarchy.first_child, child_entity);
        EXPECT_EQ(restored_root_hierarchy.next_sibling, entt::null);
        EXPECT_EQ(restored_root_hierarchy.previous_sibling, entt::null);

        const auto& restored_child_hierarchy = restored.registry().get<Hierarchy>(child_entity);
        EXPECT_EQ(restored_child_hierarchy.parent, root_entity);
        EXPECT_EQ(restored_child_hierarchy.first_child, entt::null);
        EXPECT_EQ(restored_child_hierarchy.next_sibling, entt::null);
        EXPECT_EQ(restored_child_hierarchy.previous_sibling, entt::null);

        const auto& restored_root_local = restored.registry().get<LocalTransform>(root_entity);
        EXPECT_FLOAT_EQ(restored_root_local.value.scale[0], root_local.value.scale[0]);
        EXPECT_FLOAT_EQ(restored_root_local.value.scale[1], root_local.value.scale[1]);
        EXPECT_FLOAT_EQ(restored_root_local.value.scale[2], root_local.value.scale[2]);
        EXPECT_FLOAT_EQ(restored_root_local.value.rotation.w, root_local.value.rotation.w);
        EXPECT_FLOAT_EQ(restored_root_local.value.rotation.x, root_local.value.rotation.x);
        EXPECT_FLOAT_EQ(restored_root_local.value.rotation.y, root_local.value.rotation.y);
        EXPECT_FLOAT_EQ(restored_root_local.value.rotation.z, root_local.value.rotation.z);
        EXPECT_FLOAT_EQ(restored_root_local.value.translation[0], root_local.value.translation[0]);
        EXPECT_FLOAT_EQ(restored_root_local.value.translation[1], root_local.value.translation[1]);
        EXPECT_FLOAT_EQ(restored_root_local.value.translation[2], root_local.value.translation[2]);

        const auto& restored_root_world = restored.registry().get<WorldTransform>(root_entity);
        EXPECT_FLOAT_EQ(restored_root_world.value.scale[0], root_world.value.scale[0]);
        EXPECT_FLOAT_EQ(restored_root_world.value.scale[1], root_world.value.scale[1]);
        EXPECT_FLOAT_EQ(restored_root_world.value.scale[2], root_world.value.scale[2]);
        EXPECT_FLOAT_EQ(restored_root_world.value.rotation.w, root_world.value.rotation.w);
        EXPECT_FLOAT_EQ(restored_root_world.value.rotation.x, root_world.value.rotation.x);
        EXPECT_FLOAT_EQ(restored_root_world.value.rotation.y, root_world.value.rotation.y);
        EXPECT_FLOAT_EQ(restored_root_world.value.rotation.z, root_world.value.rotation.z);
        EXPECT_FLOAT_EQ(restored_root_world.value.translation[0], root_world.value.translation[0]);
        EXPECT_FLOAT_EQ(restored_root_world.value.translation[1], root_world.value.translation[1]);
        EXPECT_FLOAT_EQ(restored_root_world.value.translation[2], root_world.value.translation[2]);

        EXPECT_TRUE(restored.registry().any_of<DirtyTransform>(root_entity));
        EXPECT_FALSE(restored.registry().any_of<DirtyTransform>(child_entity));

        const auto& restored_child_local = restored.registry().get<LocalTransform>(child_entity);
        EXPECT_FLOAT_EQ(restored_child_local.value.scale[0], child_local.value.scale[0]);
        EXPECT_FLOAT_EQ(restored_child_local.value.scale[1], child_local.value.scale[1]);
        EXPECT_FLOAT_EQ(restored_child_local.value.scale[2], child_local.value.scale[2]);
        EXPECT_FLOAT_EQ(restored_child_local.value.rotation.w, child_local.value.rotation.w);
        EXPECT_FLOAT_EQ(restored_child_local.value.rotation.x, child_local.value.rotation.x);
        EXPECT_FLOAT_EQ(restored_child_local.value.rotation.y, child_local.value.rotation.y);
        EXPECT_FLOAT_EQ(restored_child_local.value.rotation.z, child_local.value.rotation.z);
        EXPECT_FLOAT_EQ(restored_child_local.value.translation[0], child_local.value.translation[0]);
        EXPECT_FLOAT_EQ(restored_child_local.value.translation[1], child_local.value.translation[1]);
        EXPECT_FLOAT_EQ(restored_child_local.value.translation[2], child_local.value.translation[2]);
    }
} // namespace engine::scene::tests
