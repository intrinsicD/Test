#include "engine/scene/selection/bounding_box_strategy.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include "engine/geometry/shapes/ray.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/math/transform.hpp"

namespace engine::scene::selection
{
    namespace
    {
        geometry::Aabb make_bounds_from_transform(const scene::components::WorldTransform& transform)
        {
            static constexpr std::array<float, 2> kSigns{-0.5F, 0.5F};
            geometry::Aabb bounds{};
            bool initialized = false;
            for (float sx : kSigns)
            {
                for (float sy : kSigns)
                {
                    for (float sz : kSigns)
                    {
                        const math::vec3 corner{sx, sy, sz};
                        const math::vec3 world_corner = math::transform_point(transform.value, corner);
                        if (!initialized)
                        {
                            bounds.min = world_corner;
                            bounds.max = world_corner;
                            initialized = true;
                        }
                        else
                        {
                            bounds.min[0] = std::min(bounds.min[0], world_corner[0]);
                            bounds.min[1] = std::min(bounds.min[1], world_corner[1]);
                            bounds.min[2] = std::min(bounds.min[2], world_corner[2]);
                            bounds.max[0] = std::max(bounds.max[0], world_corner[0]);
                            bounds.max[1] = std::max(bounds.max[1], world_corner[1]);
                            bounds.max[2] = std::max(bounds.max[2], world_corner[2]);
                        }
                    }
                }
            }
            return bounds;
        }
    } // namespace

    BoundingBoxSelectionStrategy::BoundingBoxSelectionStrategy() = default;

    BoundingBoxSelectionStrategy::BoundingBoxSelectionStrategy(BoundsProvider provider)
        : bounds_provider_(std::move(provider))
    {
    }

    void BoundingBoxSelectionStrategy::set_bounds_provider(BoundsProvider provider)
    {
        bounds_provider_ = std::move(provider);
    }

    void BoundingBoxSelectionStrategy::set_max_distance(float max_distance) noexcept
    {
        max_distance_ = max_distance;
    }

    SelectionHit BoundingBoxSelectionStrategy::try_pick(const SelectionContext& context) const
    {
        SelectionHit best{};
        best.distance = std::numeric_limits<float>::infinity();

        if (context.scene == nullptr)
        {
            return best;
        }

        auto& registry = context.scene->registry();
        auto view = registry.view<scene::components::WorldTransform>();
        for (auto entity : view)
        {
            auto bounds = resolve_bounds(context, entity);
            if (!bounds)
            {
                continue;
            }

            geometry::Result result{};
            if (!geometry::Intersects(*bounds, context.cursor_ray, &result))
            {
                continue;
            }

            const float distance = std::max(0.0F, result.t_min);
            if (distance >= best.distance || distance > max_distance_)
            {
                continue;
            }

            best.entity = entity;
            best.distance = distance;
            best.position = geometry::PointAt(context.cursor_ray, distance);
        }

        return best;
    }

    std::optional<geometry::Aabb> BoundingBoxSelectionStrategy::derive_bounds(
        const scene::components::WorldTransform& transform
    )
    {
        return make_bounds_from_transform(transform);
    }

    std::optional<geometry::Aabb> BoundingBoxSelectionStrategy::resolve_bounds(
        const SelectionContext& context,
        entt::entity entity
    ) const
    {
        if (bounds_provider_)
        {
            if (auto result = bounds_provider_(*context.scene, entity))
            {
                return result;
            }
        }

        auto& registry = context.scene->registry();
        if (const auto* transform = registry.try_get<scene::components::WorldTransform>(entity); transform != nullptr)
        {
            return derive_bounds(*transform);
        }

        return std::nullopt;
    }
} // namespace engine::scene::selection
