#include "engine/geometry/shapes/ellipsoid.hpp"

#include <cmath>
#include <limits>
#include <numbers>

namespace engine::geometry {
    float Volume(const Ellipsoid &ellipsoid) noexcept {
        return static_cast<float>(4.0 / 3.0) * std::numbers::pi_v<float> *
               ellipsoid.radii[0] *
               ellipsoid.radii[1] *
               ellipsoid.radii[2];
    }

    math::vec3 ClosestPoint(const Ellipsoid &ellipsoid, const math::vec3 &point) noexcept {
        //TODO
    }

    double SquaredDistance(const Ellipsoid &ellipsoid, const math::vec3 &point) noexcept {
        //TODO
    }
} // namespace engine::geometry
