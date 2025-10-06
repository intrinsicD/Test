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

        // Normalise the matrix so that w = 1.
        if (utils::nearly_equal(local[3][3], zero))
        {
            return Transform<T>::Identity();
        }

        if (!utils::nearly_equal(local[3][3], one))
        {
            const T inv_w = one / local[3][3];
            for (std::size_t r = 0; r < 4; ++r)
            {
                for (std::size_t c = 0; c < 4; ++c)
                {
                    local[r][c] *= inv_w;
                }
            }
        }

        // Clear perspective terms – they cannot be represented by Transform.
        if (!utils::nearly_equal(local[3][0], zero) || !utils::nearly_equal(local[3][1], zero)
            || !utils::nearly_equal(local[3][2], zero))
        {
            local[3][0] = zero;
            local[3][1] = zero;
            local[3][2] = zero;
            local[3][3] = one;
        }

        Transform<T> result;
        result.translation = Vector<T, 3>{local[0][3], local[1][3], local[2][3]};
        local[0][3] = zero;
        local[1][3] = zero;
        local[2][3] = zero;

        Vector<T, 3> row[3] = {
            Vector<T, 3>{local[0][0], local[0][1], local[0][2]},
            Vector<T, 3>{local[1][0], local[1][1], local[1][2]},
            Vector<T, 3>{local[2][0], local[2][1], local[2][2]},
        };

        bool valid_basis = true;

        result.scale[0] = length(row[0]);
        if (utils::nearly_equal(result.scale[0], zero))
        {
            valid_basis = false;
            row[0] = Vector<T, 3>{one, zero, zero};
        }
        else
        {
            row[0] /= result.scale[0];
        }

        // Compute shear XY and orthogonalise row 1 against row 0.
        T shear_xy = dot(row[0], row[1]);
        row[1] -= shear_xy * row[0];

        result.scale[1] = length(row[1]);
        if (utils::nearly_equal(result.scale[1], zero))
        {
            valid_basis = false;
            row[1] = Vector<T, 3>{zero, one, zero};
        }
        else
        {
            row[1] /= result.scale[1];
        }

        // Compute shear XZ and orthogonalise row 2 against row 0.
        T shear_xz = dot(row[0], row[2]);
        row[2] -= shear_xz * row[0];

        // Compute shear YZ and orthogonalise row 2 against row 1.
        T shear_yz = dot(row[1], row[2]);
        row[2] -= shear_yz * row[1];

        result.scale[2] = length(row[2]);
        if (utils::nearly_equal(result.scale[2], zero))
        {
            valid_basis = false;
            row[2] = Vector<T, 3>{zero, zero, one};
        }
        else
        {
            row[2] /= result.scale[2];
        }

        if (valid_basis)
        {
            const Vector<T, 3> pdum3 = cross(row[1], row[2]);
            if (dot(row[0], pdum3) < zero)
            {
                for (std::size_t i = 0; i < 3; ++i)
                {
                    result.scale[i] = -result.scale[i];
                    row[i] = -row[i];
                }
            }

            Matrix<T, 3, 3> rotation_matrix{};
            for (std::size_t r = 0; r < 3; ++r)
            {
                rotation_matrix[r][0] = row[r][0];
                rotation_matrix[r][1] = row[r][1];
                rotation_matrix[r][2] = row[r][2];
            }

            result.rotation = normalize(utils::to_quaternion(rotation_matrix));
        }
        else
        {
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
        Transform<T> result;
        result.scale = Vector<T, 3>{
            transform.scale[0] == detail::zero<T>() ? detail::zero<T>() : detail::one<T>() / transform.scale[0],
            transform.scale[1] == detail::zero<T>() ? detail::zero<T>() : detail::one<T>() / transform.scale[1],
            transform.scale[2] == detail::zero<T>() ? detail::zero<T>() : detail::one<T>() / transform.scale[2],
        };

        const Quaternion<T> normalized = normalize(transform.rotation);
        const Quaternion<T> inverse_rotation = conjugate(normalized);
        result.rotation = inverse_rotation;

        const Quaternion<T> pure(detail::zero<T>(), transform.translation);
        const Quaternion<T> rotated = inverse_rotation * pure * normalized;

        const Vector<T, 3> rotated_translation{-rotated.x, -rotated.y, -rotated.z};

        result.translation = Vector<T, 3>{
            rotated_translation[0] * result.scale[0],
            rotated_translation[1] * result.scale[1],
            rotated_translation[2] * result.scale[2],
        };

        return result;
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
