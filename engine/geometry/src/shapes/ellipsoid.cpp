#include "engine/geometry/shapes/ellipsoid.hpp"
#include "engine/math/utils.hpp"
#include "engine/math/utils_rotation.hpp"

#include <cmath>
#include <numbers>

namespace engine::geometry {
    float Volume(const Ellipsoid &ellipsoid) noexcept {
        return static_cast<float>(4.0 / 3.0) * std::numbers::pi_v<float> *
               ellipsoid.radii[0] *
               ellipsoid.radii[1] *
               ellipsoid.radii[2];
    }

    math::vec3 ClosestPoint(const Ellipsoid &ellipsoid, const math::vec3 &point) noexcept {
        const float min_radius = math::utils::min(ellipsoid.radii[0], math::utils::min(ellipsoid.radii[1], ellipsoid.radii[2]));
        if (min_radius <= 0.0f) {
            return ellipsoid.center;
        }

        const math::vec3 offset = point - ellipsoid.center;
        const math::mat3 rotation = math::utils::to_rotation_matrix<float>(ellipsoid.orientation);
        const math::mat3 rotation_transposed = math::transpose(rotation);
        const math::vec3 local = rotation_transposed * offset;

        const double rx = static_cast<double>(ellipsoid.radii[0]);
        const double ry = static_cast<double>(ellipsoid.radii[1]);
        const double rz = static_cast<double>(ellipsoid.radii[2]);

        const double lx = static_cast<double>(local[0]);
        const double ly = static_cast<double>(local[1]);
        const double lz = static_cast<double>(local[2]);

        const double inv_radii_sq_x = lx * lx / (rx * rx);
        const double inv_radii_sq_y = ly * ly / (ry * ry);
        const double inv_radii_sq_z = lz * lz / (rz * rz);
        const double value = inv_radii_sq_x + inv_radii_sq_y + inv_radii_sq_z;
        if (value <= 1.0) {
            return point;
        }

        double lambda = 0.0;
        for (int iteration = 0; iteration < 32; ++iteration) {
            const double denom_x = rx * rx + lambda;
            const double denom_y = ry * ry + lambda;
            const double denom_z = rz * rz + lambda;

            const double term_x = lx * lx * rx * rx / (denom_x * denom_x);
            const double term_y = ly * ly * ry * ry / (denom_y * denom_y);
            const double term_z = lz * lz * rz * rz / (denom_z * denom_z);

            const double function = term_x + term_y + term_z - 1.0;
            if (std::fabs(function) <= 1e-7) {
                break;
            }

            const double derivative = -2.0 * (lx * lx * rx * rx / (denom_x * denom_x * denom_x) +
                                              ly * ly * ry * ry / (denom_y * denom_y * denom_y) +
                                              lz * lz * rz * rz / (denom_z * denom_z * denom_z));

            if (derivative == 0.0) {
                break;
            }

            const double step = function / derivative;
            lambda -= step;
            if (lambda < 0.0) {
                lambda = 0.0;
            }
            if (std::fabs(step) <= 1e-7) {
                break;
            }
        }

        const double denom_x = rx * rx + lambda;
        const double denom_y = ry * ry + lambda;
        const double denom_z = rz * rz + lambda;

        const math::vec3 closest_local{
            static_cast<float>(lx * rx * rx / denom_x),
            static_cast<float>(ly * ry * ry / denom_y),
            static_cast<float>(lz * rz * rz / denom_z)
        };

        return ellipsoid.center + rotation * closest_local;
    }

    double SquaredDistance(const Ellipsoid &ellipsoid, const math::vec3 &point) noexcept {
        const math::vec3 closest = ClosestPoint(ellipsoid, point);
        const math::vec3 diff = point - closest;
        return static_cast<double>(math::dot(diff, diff));
    }
} // namespace engine::geometry
