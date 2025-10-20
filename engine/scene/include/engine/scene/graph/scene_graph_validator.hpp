#pragma once

#include "engine/scene/errors.hpp"

#include <entt/entity/fwd.hpp>

#include <unordered_set>
#include <vector>

namespace engine::scene
{
    class Scene;
}

namespace engine::scene::graph
{
    class SceneGraphValidator
    {
    public:
        explicit SceneGraphValidator(const entt::registry& registry) noexcept;

        [[nodiscard]] SceneGraphResult<void> validate() const;

    private:
        [[nodiscard]] SceneGraphResult<void> validate_entity(entt::entity entity,
                                                             std::unordered_set<entt::entity>& visited,
                                                             std::vector<entt::entity>& stack,
                                                             std::unordered_set<entt::entity>& stack_lookup) const;

        const entt::registry* registry_{nullptr};
    };

    [[nodiscard]] SceneGraphResult<void> validate(const entt::registry& registry);

    [[nodiscard]] SceneGraphResult<void> validate(const Scene& scene);
} // namespace engine::scene::graph
