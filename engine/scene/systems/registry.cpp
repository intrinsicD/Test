#include "registry.hpp"

#include "hierarchy_system.hpp"
#include "transform_system.hpp"

#include <entt/entt.hpp>

namespace engine::scene::systems
{
    void register_scene_systems(entt::registry& registry)
    {
        (void)registry;
        register_transform_systems(registry);
        register_hierarchy_systems(registry);
    }
} // namespace engine::scene::systems
