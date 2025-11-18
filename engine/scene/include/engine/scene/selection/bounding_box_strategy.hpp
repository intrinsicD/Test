#pragma once

#include <functional>
#include <limits>
#include <optional>

#include <entt/entt.hpp>

#include "engine/geometry/shapes/aabb.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/selection/selection_engine.hpp"

namespace engine::scene::selection
{
    class BoundingBoxSelectionStrategy final : public SelectionStrategy
    {
    public:
        using BoundsProvider = std::function<std::optional<geometry::Aabb>(scene::Scene&, entt::entity)>;

        BoundingBoxSelectionStrategy();
        explicit BoundingBoxSelectionStrategy(BoundsProvider provider);

        void set_bounds_provider(BoundsProvider provider);
        void set_max_distance(float max_distance) noexcept;

        [[nodiscard]] SelectionHit try_pick(const SelectionContext& context) const override;

    private:
        BoundsProvider bounds_provider_{};
        float max_distance_{std::numeric_limits<float>::infinity()};

        [[nodiscard]] static std::optional<geometry::Aabb> derive_bounds(const scene::components::WorldTransform& transform);
        [[nodiscard]] std::optional<geometry::Aabb> resolve_bounds(const SelectionContext& context, entt::entity entity) const;
    };
} // namespace engine::scene::selection
