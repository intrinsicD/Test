#pragma once

#include <entt/entity/registry.hpp>

namespace engine::scene::systems
{
    void register_hierarchy_systems(entt::registry& registry);

    void set_parent(entt::registry& registry, entt::entity child, entt::entity parent,
                    bool preserve_world_transform = false);

    void detach_from_parent(entt::registry& registry, entt::entity child,
                            bool preserve_world_transform = false);

    bool detect_hierarchy_cycle(entt::registry& registry, entt::entity child, entt::entity parent);
}
