#include "hierarchy_system.hpp"

#include "../components/hierarchy.hpp"
#include "../components/transform.hpp"
#include "transform_system.hpp"

#include <entt/entt.hpp>

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

            if (hierarchy->parent != entt::null)
            {
                if (auto* parent_hierarchy = registry.try_get<components::Hierarchy>(hierarchy->parent); parent_hierarchy != nullptr)
                {
                    if (parent_hierarchy->first_child == child)
                    {
                        parent_hierarchy->first_child = hierarchy->next_sibling;
                    }
                }
            }

            if (hierarchy->previous_sibling != entt::null)
            {
                if (auto* previous = registry.try_get<components::Hierarchy>(hierarchy->previous_sibling); previous != nullptr)
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
                if (auto* first = registry.try_get<components::Hierarchy>(parent_hierarchy.first_child); first != nullptr)
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
        auto& hierarchy = assure_hierarchy(registry, child);
        if (hierarchy.parent == parent)
        {
            return;
        }

        detach_internal(registry, child);

        hierarchy.parent = parent;
        attach_internal(registry, child, hierarchy);

        mark_subtree_dirty(registry, child);
    }

    void detach_from_parent(entt::registry& registry, entt::entity child)
    {
        if (auto* hierarchy = registry.try_get<components::Hierarchy>(child); hierarchy != nullptr)
        {
            detach_internal(registry, child);
            hierarchy->parent = entt::null;
            mark_subtree_dirty(registry, child);
        }
    }
} // namespace engine::scene::systems
