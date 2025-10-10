#include "engine/scene/systems/registry.hpp"

#include "engine/scene/systems/hierarchy_system.hpp"
#include "engine/scene/systems/transform_system.hpp"

#include <entt/entity/registry.hpp>

namespace engine::scene::systems
{
    void register_scene_systems(entt::registry& registry)
    {
        (void)registry;
        register_transform_systems(registry);
        register_hierarchy_systems(registry);
    }
} // namespace engine::scene::systems
