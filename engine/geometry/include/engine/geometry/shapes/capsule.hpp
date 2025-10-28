#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry
{
    struct ENGINE_GEOMETRY_API Capsule
    {
        math::vec3 point_a{0.0f, 0.0f, 0.0f};
        math::vec3 point_b{0.0f, 0.0f, 0.0f};
        float radius{0.0f};
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Center(const Capsule& capsule) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Axis(const Capsule& capsule) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 AxisDirection(const Capsule& capsule) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Length(const Capsule& capsule) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SurfaceArea(const Capsule& capsule) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Volume(const Capsule& capsule) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Capsule& capsule,
                                                              const math::vec3& point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Capsule& capsule,
                                                              const math::vec3& point) noexcept;

    ENGINE_GEOMETRY_API void Random(Capsule& capsule, RandomEngine& rng) noexcept;

    ENGINE_GEOMETRY_API void Random(Capsule& capsule) noexcept;
}
