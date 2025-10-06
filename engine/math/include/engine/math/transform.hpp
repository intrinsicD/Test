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
        Transform<T> result;
        result.translation = Vector<T, 3>{matrix[0][3], matrix[1][3], matrix[2][3]};

        Vector<T, 3> basis[3];
        bool valid_basis = true;

        for (std::size_t column = 0; column < 3; ++column)
        {
            const Vector<T, 3> column_vector{
                matrix[0][column],
                matrix[1][column],
                matrix[2][column],
            };

            const T magnitude = length(column_vector);
            result.scale[column] = magnitude;

            if (utils::nearly_equal(magnitude, detail::zero<T>()))
            {
                valid_basis = false;
                basis[column] = Vector<T, 3>{detail::zero<T>()};
            }
            else
            {
                basis[column] = column_vector / magnitude;
            }
        }

        if (valid_basis)
        {
            const Vector<T, 3> cross_product = cross(basis[0], basis[1]);
            if (dot(cross_product, basis[2]) < detail::zero<T>())
            {
                result.scale[2] = -result.scale[2];
                basis[2] *= static_cast<T>(-1);
            }

            Matrix<T, 3, 3> rotation_matrix{};
            for (std::size_t column = 0; column < 3; ++column)
            {
                rotation_matrix[0][column] = basis[column][0];
                rotation_matrix[1][column] = basis[column][1];
                rotation_matrix[2][column] = basis[column][2];
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

        result.rotation = normalize(conjugate(transform.rotation));

        const Vector<T, 3> scaled_translation{
            transform.translation[0] * result.scale[0],
            transform.translation[1] * result.scale[1],
            transform.translation[2] * result.scale[2],
        };

        const Quaternion<T> normalized = normalize(transform.rotation);
        const Quaternion<T> pure(detail::zero<T>(), scaled_translation);
        const Quaternion<T> rotated = conjugate(normalized) * pure * normalized;

        result.translation = Vector<T, 3>{-rotated.x, -rotated.y, -rotated.z};

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
