#pragma once

#include <entt/entt.hpp>

namespace engine::scene::systems
{
    void register_hierarchy_systems(entt::registry& registry);

    void set_parent(entt::registry& registry, entt::entity child, entt::entity parent);

    void detach_from_parent(entt::registry& registry, entt::entity child);
}
