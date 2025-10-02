#include "engine/geometry/shapes/cylinder.hpp"

#include <cmath>
#include <numbers>

#include "engine/geometry/shapes/sphere.hpp"

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float two() noexcept { return 2.0f; }

}  // namespace

math::vec3 AxisDirection(const Cylinder& c) noexcept {
    const float len = math::length(c.axis);
    if (len == 0.0f) {
        return math::vec3{0.0f};
    }
    return c.axis / len;
}

math::vec3 TopCenter(const Cylinder& c) noexcept {
    return c.center + AxisDirection(c) * c.half_height;
}

math::vec3 BottomCenter(const Cylinder& c) noexcept {
    return c.center - AxisDirection(c) * c.half_height;
}

float Volume(const Cylinder& c) noexcept {
    const float height = c.half_height * two();
    return std::numbers::pi_v<float> * c.radius * c.radius * height;
}

float LateralSurfaceArea(const Cylinder& c) noexcept {
    const float height = c.half_height * two();
    return two() * std::numbers::pi_v<float> * c.radius * height;
}

float SurfaceArea(const Cylinder& c) noexcept {
    return LateralSurfaceArea(c) + two() * std::numbers::pi_v<float> * c.radius * c.radius;
}

}  // namespace engine::geometry

