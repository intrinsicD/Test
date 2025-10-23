#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

#include <array>

namespace engine::geometry {

    /// @brief View frustum defined by 6 planes (left, right, bottom, top, near, far)
    /// Planes point inward (normals toward frustum interior)
    struct ENGINE_GEOMETRY_API Frustum {
        std::array<Plane, 6> planes;

        enum PlaneIndex : size_t {
            kLeft = 0,
            kRight = 1,
            kBottom = 2,
            kTop = 3,
            kNear = 4,
            kFar = 5
        };
    };

    /// @brief Extract frustum from a view-projection matrix
    /// @param view_projection Combined view-projection matrix
    /// @return Frustum with planes pointing inward
    [[nodiscard]] ENGINE_GEOMETRY_API Frustum ExtractFrustum(const math::mat4& view_projection) noexcept;

    /// @brief Get the 8 corner points of the frustum in world space
    /// @param frustum The frustum to extract corners from
    /// @return Array of 8 corner points (near: bottom-left, bottom-right, top-right, top-left; far: same order)
    [[nodiscard]] ENGINE_GEOMETRY_API std::array<math::vec3, 8> GetCorners(const Frustum& frustum) noexcept;

} // namespace engine::geometry

