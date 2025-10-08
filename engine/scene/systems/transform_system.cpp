#include "transform_system.hpp"

#include "components/hierarchy.hpp"
#include "components/transform.hpp"

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

    void register_transform_systems(entt::registry& registry)
    {
        registry.on_update<components::LocalTransform>().connect<&mark_subtree_dirty>();
        registry.on_update<components::Hierarchy>().connect<&mark_subtree_dirty>();
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
            math::Transform<float> parent_world{};
        };

        std::vector<Node> stack;

        // We start by finding the "roots" of all dirty sub-hierarchies for this frame.
        // A dirty entity is a "root" for propagation if it has no parent or its parent is NOT dirty.
        auto dirty_view = registry.view<components::LocalTransform, components::DirtyTransform>();
        for (auto entity : dirty_view)
        {
            const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
            const bool has_parent = hierarchy != nullptr && hierarchy->parent != entt::null && registry.valid(
                hierarchy->parent);

            // If the parent is also dirty, we can skip this entity for now.
            // It will be processed when its parent is processed.
            if (has_parent && registry.any_of<components::DirtyTransform>(hierarchy->parent))
            {
                continue;
            }

            math::Transform<float> parent_world{math::Transform<float>::Identity()};
            if (has_parent)
            {
                // We can safely get the parent's world transform because we know it's not dirty.
                if (const auto* parent_world_component = registry.try_get<
                    components::WorldTransform>(hierarchy->parent))
                {
                    parent_world = parent_world_component->value;
                }
            }

            stack.push_back(Node{entity, parent_world});
        }

        // Now, process the stack and propagate changes down the hierarchy.
        while (!stack.empty())
        {
            const Node node = stack.back();
            stack.pop_back();

            // The entity might have been processed already if it was a child of another dirty node.
            // We only process if it's still marked as dirty.
            if (!registry.valid(node.entity) || !registry.any_of<components::DirtyTransform>(node.entity))
            {
                continue;
            }

            // Calculate the new world transform
            const auto& local = registry.get<components::LocalTransform>(node.entity);
            auto& world = assure_world(registry, node.entity);
            world.value = math::combine(node.parent_world, local.value);

            // The transform is now clean.
            registry.remove<components::DirtyTransform>(node.entity);

            // Add all children to the stack for processing, regardless of whether they were marked dirty initially.
            if (auto* hierarchy = registry.try_get<components::Hierarchy>(node.entity); hierarchy != nullptr)
            {
                auto child = hierarchy->first_child;
                while (child != entt::null)
                {
                    // We only need to know that the child exists and has a LocalTransform.
                    if (registry.valid(child) && registry.any_of<components::LocalTransform>(child))
                    {
                        // The new parent_world for the child is the transform we just calculated.
                        stack.push_back(Node{child, world.value});
                    }

                    const auto* child_hierarchy = registry.try_get<components::Hierarchy>(child);
                    child = (child_hierarchy != nullptr) ? child_hierarchy->next_sibling : entt::null;
                }
            }
        }
    }
} // namespace engine::scene::systems
