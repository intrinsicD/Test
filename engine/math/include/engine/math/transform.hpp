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
        const T zero = detail::zero<T>();
        const T one = detail::one<T>();

        Matrix<T, 4, 4> local = matrix;

        // Normalize so local[3][3] == 1 (affine w)
        if (utils::nearly_equal(local[3][3], zero))
            return Transform<T>::Identity();

        if (!utils::nearly_equal(local[3][3], one))
        {
            const T inv_w = one / local[3][3];
            for (std::size_t r = 0; r < 4; ++r)
                for (std::size_t c = 0; c < 4; ++c)
                    local[r][c] *= inv_w;
        }

        // Clear perspective (cannot represent)
        if (!utils::nearly_equal(local[3][0], zero) ||
            !utils::nearly_equal(local[3][1], zero) ||
            !utils::nearly_equal(local[3][2], zero))
        {
            local[3][0] = local[3][1] = local[3][2] = zero;
            local[3][3] = one;
        }

        Transform<T> result;

        // Translation is last column (rows 0..2)
        result.translation = Vector<T, 3>{local[0][3], local[1][3], local[2][3]};

        // Extract the 3 column vectors (rotation * scale)
        Vector<T, 3> axes[3];
        bool valid = true;
        for (std::size_t c = 0; c < 3; ++c)
        {
            Vector<T, 3> col{
                local[0][c],
                local[1][c],
                local[2][c]
            };
            const T mag = length(col);
            result.scale[c] = mag;
            if (utils::nearly_equal(mag, zero))
            {
                axes[c] = Vector<T, 3>{
                    (c == 0) ? one : zero,
                    (c == 1) ? one : zero,
                    (c == 2) ? one : zero
                };
                valid = false;
            }
            else
            {
                axes[c] = col / mag;
            }
        }

        if (valid)
        {
            // Ensure right-handed orthonormal basis
            const T det = dot(axes[0], cross(axes[1], axes[2]));
            if (det < zero)
            {
                // Flip the axis with the largest magnitude to preserve relative scales
                std::size_t idx = 0;
                if (utils::abs(result.scale[1]) > utils::abs(result.scale[idx])) idx = 1;
                if (utils::abs(result.scale[2]) > utils::abs(result.scale[idx])) idx = 2;
                result.scale[idx] = -result.scale[idx];
                axes[idx] = -axes[idx];
            }

            // Build 3x3 rotation matrix (rows r, columns c)
            Matrix<T, 3, 3> rot{};
            for (std::size_t r = 0; r < 3; ++r)
            {
                rot[r][0] = axes[0][r];
                rot[r][1] = axes[1][r];
                rot[r][2] = axes[2][r];
            }
            result.rotation = normalize(utils::to_quaternion(rot));
        }
        else
        {
            // Degenerate case
            result.rotation = Quaternion<T>::Identity();
        }

        return result;
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
        const auto M_inv = try_inverse(cast<double>(to_matrix(transform))); // your reliable 4×4 inverse
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
