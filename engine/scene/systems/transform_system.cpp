#include "transform_system.hpp"

#include "../components/hierarchy.hpp"
#include "../components/transform.hpp"

#include "engine/math/transform.hpp"

#include <entt/entt.hpp>

#include <vector>

namespace engine::scene::systems
{
    namespace
    {
        components::WorldTransform& assure_world(entt::registry& registry, entt::entity entity)
        {
            if (auto* existing = registry.try_get<components::WorldTransform>(entity); existing != nullptr)
            {
                return *existing;
            }

            return registry.emplace<components::WorldTransform>(entity);
        }
    } // namespace

    void register_transform_systems(entt::registry&)
    {
    }

    void mark_transform_dirty(entt::registry& registry, entt::entity entity)
    {
        registry.emplace_or_replace<components::DirtyTransform>(entity);
    }

    void mark_subtree_dirty(entt::registry& registry, entt::entity root)
    {
        if (!registry.valid(root))
        {
            return;
        }

        std::vector<entt::entity> stack;
        stack.push_back(root);

        while (!stack.empty())
        {
            const entt::entity current = stack.back();
            stack.pop_back();

            if (!registry.valid(current))
            {
                continue;
            }

            mark_transform_dirty(registry, current);

            if (auto* hierarchy = registry.try_get<components::Hierarchy>(current); hierarchy != nullptr)
            {
                auto child = hierarchy->first_child;
                while (child != entt::null)
                {
                    stack.push_back(child);
                    const auto* child_hierarchy = registry.try_get<components::Hierarchy>(child);
                    child = (child_hierarchy != nullptr) ? child_hierarchy->next_sibling : entt::null;
                }
            }
        }
    }

    void propagate_transforms(entt::registry& registry)
    {
        struct Node
        {
            entt::entity entity{entt::null};
            engine::math::Transform<float> parent_world{};
            bool has_parent{false};
        };

        std::vector<Node> stack;

        auto view = registry.view<components::LocalTransform>();
        for (auto [entity, local] : view.each())
        {
            (void)local;
            const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
            const bool has_parent = hierarchy != nullptr && hierarchy->parent != entt::null && registry.valid(hierarchy->parent)
                                    && registry.try_get<components::LocalTransform>(hierarchy->parent) != nullptr;
            if (!has_parent)
            {
                stack.push_back(Node{entity, engine::math::Transform<float>::Identity(), false});
            }
        }

        while (!stack.empty())
        {
            const Node node = stack.back();
            stack.pop_back();

            if (!registry.valid(node.entity) || !registry.any_of<components::LocalTransform>(node.entity))
            {
                continue;
            }

            auto& local = registry.get<components::LocalTransform>(node.entity);
            auto& world = assure_world(registry, node.entity);

            if (node.has_parent)
            {
                world.value = engine::math::combine(node.parent_world, local.value);
            }
            else
            {
                world.value = local.value;
            }

            registry.remove<components::DirtyTransform>(node.entity);

            if (auto* hierarchy = registry.try_get<components::Hierarchy>(node.entity); hierarchy != nullptr)
            {
                auto child = hierarchy->first_child;
                while (child != entt::null)
                {
                    if (registry.valid(child) && registry.any_of<components::LocalTransform>(child))
                    {
                        stack.push_back(Node{child, world.value, true});
                    }

                    const auto* child_hierarchy = registry.try_get<components::Hierarchy>(child);
                    child = (child_hierarchy != nullptr) ? child_hierarchy->next_sibling : entt::null;
                }
            }
        }
    }
} // namespace engine::scene::systems
