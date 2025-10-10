#include "engine/scene/systems/hierarchy_system.hpp"
#include "engine/scene/systems/transform_system.hpp"
#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/math/transform.hpp"

#include <vector>

namespace engine::scene::systems
{
    namespace
    {
        using Transform = math::Transform<float>;

        components::Hierarchy& assure_hierarchy(entt::registry& registry, entt::entity entity)
        {
            if (auto* existing = registry.try_get<components::Hierarchy>(entity); existing != nullptr)
            {
                return *existing;
            }

            return registry.emplace<components::Hierarchy>(entity);
        }

        // Resolve the current world transform for an entity by walking up the hierarchy and
        // composing the local transforms. This is used to preserve world-space placement during
        // reparenting operations without requiring a prior propagation pass.
        [[nodiscard]] Transform compute_world_transform(entt::registry& registry, entt::entity entity)
        {
            if (!registry.valid(entity))
            {
                return Transform::Identity();
            }

            std::vector<entt::entity> chain;
            auto current = entity;
            while (current != entt::null && registry.valid(current))
            {
                chain.push_back(current);
                const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
                current = (hierarchy != nullptr) ? hierarchy->parent : entt::null;
            }

            Transform world = Transform::Identity();
            for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            {
                if (const auto* local = registry.try_get<components::LocalTransform>(*it); local != nullptr)
                {
                    world = math::combine(world, local->value);
                }
            }

            return world;
        }

        void detach_internal(entt::registry& registry, entt::entity child)
        {
            auto* hierarchy = registry.try_get<components::Hierarchy>(child);
            if (hierarchy == nullptr)
            {
                return;
            }

            // Check if the parent is valid before trying to get its component.
            if (hierarchy->parent != entt::null && registry.valid(hierarchy->parent))
            {
                if (auto* parent_hierarchy = registry.try_get<components::Hierarchy>(hierarchy->parent);
                    parent_hierarchy != nullptr)
                {
                    if (parent_hierarchy->first_child == child)
                    {
                        parent_hierarchy->first_child = hierarchy->next_sibling;
                    }
                }
            }

            if (hierarchy->previous_sibling != entt::null)
            {
                if (auto* previous = registry.try_get<components::Hierarchy>(hierarchy->previous_sibling); previous !=
                    nullptr)
                {
                    previous->next_sibling = hierarchy->next_sibling;
                }
            }

            if (hierarchy->next_sibling != entt::null)
            {
                if (auto* next = registry.try_get<components::Hierarchy>(hierarchy->next_sibling); next != nullptr)
                {
                    next->previous_sibling = hierarchy->previous_sibling;
                }
            }

            hierarchy->previous_sibling = entt::null;
            hierarchy->next_sibling = entt::null;
        }

        void attach_internal(entt::registry& registry, entt::entity child, components::Hierarchy& hierarchy)
        {
            if (hierarchy.parent == entt::null || !registry.valid(hierarchy.parent))
            {
                hierarchy.parent = entt::null;
                return;
            }

            auto& parent_hierarchy = assure_hierarchy(registry, hierarchy.parent);
            hierarchy.next_sibling = parent_hierarchy.first_child;
            hierarchy.previous_sibling = entt::null;

            if (parent_hierarchy.first_child != entt::null)
            {
                if (auto* first = registry.try_get<components::Hierarchy>(parent_hierarchy.first_child); first !=
                    nullptr)
                {
                    first->previous_sibling = child;
                }
            }

            parent_hierarchy.first_child = child;
        }
    } // namespace

    void register_hierarchy_systems(entt::registry&)
    {
    }

    void set_parent(entt::registry& registry, entt::entity child, entt::entity parent)
    {
        // A child cannot be its own parent.
        if (child == parent)
        {
            return;
        }

        if (detect_hierarchy_cycle(registry, child, parent))
        {
            return;
        }

        auto& hierarchy = assure_hierarchy(registry, child);
        if (hierarchy.parent == parent)
        {
            return;
        }

        const bool preserve_world = registry.any_of<components::LocalTransform>(child);
        Transform child_world = Transform::Identity();
        if (preserve_world)
        {
            child_world = compute_world_transform(registry, child);
        }

        const bool parent_valid = parent != entt::null;
        Transform parent_world = Transform::Identity();
        if (preserve_world && parent_valid)
        {
            parent_world = compute_world_transform(registry, parent);
        }

        detach_internal(registry, child);

        hierarchy.parent = parent;

        // If the new parent is entt::null, we don't need to attach.
        if (parent_valid)
        {
            attach_internal(registry, child, hierarchy);
        }

        if (preserve_world)
        {
            // Preserve the child's world transform by redefining its local frame relative to the new parent.
            auto& local = registry.get<components::LocalTransform>(child);
            if (parent_valid)
            {
                local.value = math::combine(math::inverse(parent_world), child_world);
            }
            else
            {
                local.value = child_world;
            }
        }

        mark_subtree_dirty(registry, child);
    }

    void detach_from_parent(entt::registry& registry, entt::entity child)
    {
        if (auto* hierarchy = registry.try_get<components::Hierarchy>(child); hierarchy != nullptr)
        {
            const bool preserve_world = registry.any_of<components::LocalTransform>(child);
            Transform child_world = Transform::Identity();
            if (preserve_world)
            {
                child_world = compute_world_transform(registry, child);
            }

            detach_internal(registry, child);
            hierarchy->parent = entt::null;

            if (preserve_world)
            {
                // When detaching, bake the world transform back into the local component so the entity doesn't jump.
                auto& local = registry.get<components::LocalTransform>(child);
                local.value = child_world;
            }

            mark_subtree_dirty(registry, child);
        }
    }

    bool detect_hierarchy_cycle(entt::registry& registry, entt::entity child, entt::entity parent)
    {
        // Walk up the new parent's hierarchy to ensure the child is not an ancestor.
        auto current = parent;
        while (current != entt::null)
        {
            if (current == child)
            {
                // Creating a cycle is forbidden. You could log an error here.
                return true;
            }
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = (hierarchy != nullptr) ? hierarchy->parent : entt::null;
        }
        return false;
    }
} // namespace engine::scene::systems
