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
        components::Hierarchy& assure_hierarchy(entt::registry& registry, entt::entity entity)
        {
            if (auto* existing = registry.try_get<components::Hierarchy>(entity); existing != nullptr)
            {
                return *existing;
            }

            return registry.emplace<components::Hierarchy>(entity);
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

        math::Transform<float> evaluate_world_transform(entt::registry& registry, entt::entity entity)
        {
            std::vector<entt::entity> chain;
            auto current = entity;
            while (current != entt::null && registry.valid(current))
            {
                chain.push_back(current);
                const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
                current = (hierarchy != nullptr) ? hierarchy->parent : entt::null;
            }

            math::Transform<float> world{math::Transform<float>::Identity()};
            for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            {
                if (const auto* local = registry.try_get<components::LocalTransform>(*it); local != nullptr)
                {
                    world = math::combine(world, local->value);
                }
            }

            return world;
        }

        void apply_preserved_world(entt::registry& registry, entt::entity child, entt::entity parent,
                                   const math::Transform<float>& desired_world)
        {
            auto* local = registry.try_get<components::LocalTransform>(child);
            if (local == nullptr)
            {
                local = &registry.emplace<components::LocalTransform>(child);
            }

            math::Transform<float> parent_world{math::Transform<float>::Identity()};
            if (parent != entt::null && registry.valid(parent))
            {
                parent_world = evaluate_world_transform(registry, parent);
            }

            const math::Transform<float> parent_inverse = math::inverse(parent_world);
            local->value = math::combine(parent_inverse, desired_world);

            components::WorldTransform world_component{};
            world_component.value = desired_world;
            registry.emplace_or_replace<components::WorldTransform>(child, world_component);
        }
    } // namespace

    void register_hierarchy_systems(entt::registry&)
    {
    }

    void set_parent(entt::registry& registry, entt::entity child, entt::entity parent,
                    bool preserve_world_transform)
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

        math::Transform<float> original_world{};
        const bool should_preserve = preserve_world_transform &&
            registry.any_of<components::LocalTransform>(child);
        if (should_preserve)
        {
            original_world = evaluate_world_transform(registry, child);
        }

        auto& hierarchy = assure_hierarchy(registry, child);
        if (hierarchy.parent == parent)
        {
            return;
        }

        detach_internal(registry, child);

        hierarchy.parent = parent;

        // If the new parent is entt::null, we don't need to attach.
        if (parent != entt::null)
        {
            attach_internal(registry, child, hierarchy);
        }

        if (should_preserve)
        {
            apply_preserved_world(registry, child, parent, original_world);
        }

        mark_subtree_dirty(registry, child);
    }

    void detach_from_parent(entt::registry& registry, entt::entity child, bool preserve_world_transform)
    {
        if (auto* hierarchy = registry.try_get<components::Hierarchy>(child); hierarchy != nullptr)
        {
            math::Transform<float> original_world{};
            const bool should_preserve = preserve_world_transform &&
                registry.any_of<components::LocalTransform>(child);
            if (should_preserve)
            {
                original_world = evaluate_world_transform(registry, child);
            }

            detach_internal(registry, child);
            hierarchy->parent = entt::null;

            if (should_preserve)
            {
                apply_preserved_world(registry, child, entt::null, original_world);
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
