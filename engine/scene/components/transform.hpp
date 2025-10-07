#pragma once

#include "engine/math/transform.hpp"

#include <entt/entt.hpp>

namespace engine::scene::components
{
    struct LocalTransform
    {
        engine::math::Transform<float> value{engine::math::Transform<float>::Identity()};
    };

    struct WorldTransform
    {
        engine::math::Transform<float> value{engine::math::Transform<float>::Identity()};
    };

    struct DirtyTransform
    {
    };

    inline void mark_dirty(entt::registry& registry, entt::entity entity)
    {
        registry.emplace_or_replace<DirtyTransform>(entity);
    }
} // namespace engine::scene::components
