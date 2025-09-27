#include "engine/geometry/shapes/ellipsoid.hpp"

#include <cmath>
#include <limits>
#include <numbers>

namespace engine::geometry {

float volume(const ellipsoid& e) noexcept {
    return static_cast<float>(4.0 / 3.0) * std::numbers::pi_v<float> * e.radii[0] * e.radii[1] * e.radii[2];
}

bool contains(const ellipsoid& e, const math::vec3& point) noexcept {
    const math::vec3 relative = point - e.center;
    const math::mat3 inverse_orientation = math::transpose(e.orientation);
    const math::vec3 local = inverse_orientation * relative;

    float sum = 0.0f;
    for (std::size_t i = 0; i < 3; ++i) {
        if (e.radii[i] == 0.0f) {
            if (local[i] != 0.0f) {
                return false;
            }
            continue;
        }
        const float scaled = local[i] / e.radii[i];
        sum += scaled * scaled;
    }

    return sum <= 1.0f + std::numeric_limits<float>::epsilon();
}

}  // namespace engine::geometry

