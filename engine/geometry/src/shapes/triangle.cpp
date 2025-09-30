#include "engine/geometry/shapes/triangle.hpp"

#include <cmath>

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float half() noexcept { return 0.5f; }

}  // namespace

math::vec3 normal(const Triangle& t) noexcept {
    return math::cross(t.b - t.a, t.c - t.a);
}

math::vec3 unit_normal(const Triangle& t) noexcept {
    return math::normalize(normal(t));
}

float area(const Triangle& t) noexcept {
    return half() * math::length(normal(t));
}

math::vec3 centroid(const Triangle& t) noexcept {
    return (t.a + t.b + t.c) / static_cast<float>(3.0);
}

}  // namespace engine::geometry

