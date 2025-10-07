#pragma once

#include <entt/entt.hpp>

namespace engine::scene::components
{
    struct Hierarchy
    {
        entt::entity parent{entt::null};
        entt::entity first_child{entt::null};
        entt::entity next_sibling{entt::null};
        entt::entity previous_sibling{entt::null};
    };

    [[nodiscard]] inline bool is_root(const Hierarchy& hierarchy) noexcept
    {
        return hierarchy.parent == entt::null;
    }

    [[nodiscard]] inline bool has_children(const Hierarchy& hierarchy) noexcept
    {
        return hierarchy.first_child != entt::null;
    }
} // namespace engine::scene::components
