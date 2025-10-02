#include "engine/geometry/shapes/ray.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/geometry/shapes.hpp"

namespace engine::geometry {
    math::vec3 PointAt(const Ray &ray, float t) noexcept {
        return ray.origin + ray.direction * t;
    }

    math::vec3 ClosestPoint(const Ray &ray, const math::vec3 &point) noexcept {
        //TODO
    }

    double SquaredDistance(const Ray &ray, const math::vec3 &point) noexcept {
        //TODO
    }
} // namespace engine::geometry
