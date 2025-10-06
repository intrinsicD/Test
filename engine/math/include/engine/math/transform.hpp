#pragma once

#include "engine/math/common.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/quaternion.hpp"
#include "engine/math/utils.hpp"
#include "engine/math/utils_rotation.hpp"
#include "engine/math/vector.hpp"

namespace engine::math
{
    template <typename T>
    struct Transform
    {
        using value_type = T;

        Vector<T, 3> scale;
        Quaternion<T> rotation;
        Vector<T, 3> translation;

        ENGINE_MATH_INLINE Transform() noexcept
            : scale(detail::one<T>()),
              rotation(Quaternion<T>::Identity()),
              translation(detail::zero<T>())
        {
        }

        ENGINE_MATH_INLINE Transform(const Vector<T, 3>& scale_, const Quaternion<T>& rotation_,
                                     const Vector<T, 3>& translation_) noexcept
            : scale(scale_), rotation(rotation_), translation(translation_)
        {
        }

        ENGINE_MATH_INLINE static Transform Identity() noexcept { return Transform{}; }
    };

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> to_matrix(const Transform<T>& transform) noexcept
    {
        const Quaternion<T> normalized = normalize(transform.rotation);
        Matrix<T, 4, 4> result = utils::to_rotation_matrix(normalized);

        for (std::size_t column = 0; column < 3; ++column)
        {
            result.columns[column] *= transform.scale[column];
        }

        result[0][3] = transform.translation[0];
        result[1][3] = transform.translation[1];
        result[2][3] = transform.translation[2];
        result[3][3] = detail::one<T>();

        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE Transform<T> from_matrix(const Matrix<T, 4, 4>& matrix) noexcept
    {
        Transform<T> out;

        // Translation
        out.translation = Vector<T,3>{ matrix[0][3], matrix[1][3], matrix[2][3] };

        // Extract columns (linear block)
        Vector<T,3> c[3] = {
            { matrix[0][0], matrix[1][0], matrix[2][0] },
            { matrix[0][1], matrix[1][1], matrix[2][1] },
            { matrix[0][2], matrix[1][2], matrix[2][2] },
        };

        // Lengths (unsigned scales)
        T l[3] = { length(c[0]), length(c[1]), length(c[2]) };

        // Handle degenerate
        bool ok = (l[0] > detail::zero<T>()) && (l[1] > detail::zero<T>()) && (l[2] > detail::zero<T>());
        if (!ok) {
            out.scale = Vector<T,3>{ l[0], l[1], l[2] };
            out.rotation = Quaternion<T>::Identity();
            return out;
        }

        // Normalized columns (may include sign of original scales)
        Vector<T,3> u[3] = { c[0] / l[0], c[1] / l[1], c[2] / l[2] };

        // Build rotation matrix with current handedness
        Matrix<T,3,3> R;
        for (std::size_t j=0; j<3; ++j) {
            R[0][j] = u[j][0];
            R[1][j] = u[j][1];
            R[2][j] = u[j][2];
        }

        // Fix handedness deterministically by flipping the largest column if needed
        const T detR =
            R[0][0]*(R[1][1]*R[2][2] - R[1][2]*R[2][1]) -
            R[0][1]*(R[1][0]*R[2][2] - R[1][2]*R[2][0]) +
            R[0][2]*(R[1][0]*R[2][1] - R[1][1]*R[2][0]);

        if (detR < detail::zero<T>()) {
            // choose column with largest length to flip
            std::size_t k = 0;
            if (l[1] > l[k]) k = 1;
            if (l[2] > l[k]) k = 2;

            // flip that column in R and put the minus into the corresponding scale
            u[k] = -u[k];
            l[k] = -l[k];

            // rebuild R
            for (std::size_t j=0; j<3; ++j) {
                R[0][j] = u[j][0];
                R[1][j] = u[j][1];
                R[2][j] = u[j][2];
            }
        }

        out.scale = Vector<T,3>{ l[0], l[1], l[2] };
        out.rotation = normalize(utils::to_quaternion(R));
        return out;
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 3> transform_vector(const Transform<T>& transform, const Vector<T, 3>& vector) noexcept
    {
        Vector<T, 3> scaled{
            vector[0] * transform.scale[0],
            vector[1] * transform.scale[1],
            vector[2] * transform.scale[2],
        };

        const Quaternion<T> normalized = normalize(transform.rotation);
        const Quaternion<T> pure(detail::zero<T>(), scaled);
        const Quaternion<T> rotated = normalized * pure * conjugate(normalized);

        return Vector<T, 3>{rotated.x, rotated.y, rotated.z};
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 3> transform_point(const Transform<T>& transform, const Vector<T, 3>& point) noexcept
    {
        const Vector<T, 3> rotated = transform_vector(transform, point);
        return rotated + transform.translation;
    }

    template <typename T>
    ENGINE_MATH_INLINE Transform<T> combine(const Transform<T>& parent, const Transform<T>& child) noexcept
    {
        const Matrix<T, 4, 4> combined = to_matrix(parent) * to_matrix(child);
        return from_matrix(combined);
    }

    template <typename T>
    ENGINE_MATH_INLINE Transform<T> inverse(const Transform<T>& transform) noexcept
    {
        const auto M_inv = try_inverse(cast<double>(to_matrix(transform)));   // your reliable 4×4 inverse
        assert(M_inv.has_value() && "Transform::inverse: singular matrix");
        return from_matrix(cast<float>(M_inv.value()));
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 4> to_angle_axis(const Transform<T>& transform) noexcept
    {
        return to_angle_axis(transform.rotation);
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 3> to_cayley_parameters(const Transform<T>& transform) noexcept
    {
        return to_cayley_parameters(transform.rotation);
    }

    template <typename T>
    ENGINE_MATH_INLINE Transform<T> from_angle_axis(const Vector<T, 4>& angle_axis, const Vector<T, 3>& scale,
                                                    const Vector<T, 3>& translation) noexcept
    {
        return Transform<T>{scale, from_angle_axis(angle_axis), translation};
    }

    template <typename T>
    ENGINE_MATH_INLINE Transform<T> from_cayley_parameters(const Vector<T, 3>& cayley, const Vector<T, 3>& scale,
                                                           const Vector<T, 3>& translation) noexcept
    {
        return Transform<T>{scale, from_cayley_parameters(cayley), translation};
    }
} // namespace engine::math
