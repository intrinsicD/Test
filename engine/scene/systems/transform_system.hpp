#pragma once

#include <entt/entt.hpp>

namespace engine::scene::systems
{
    void register_transform_systems(entt::registry& registry);

    void mark_transform_dirty(entt::registry& registry, entt::entity entity);

    void mark_subtree_dirty(entt::registry& registry, entt::entity root);

    void propagate_transforms(entt::registry& registry);
}
