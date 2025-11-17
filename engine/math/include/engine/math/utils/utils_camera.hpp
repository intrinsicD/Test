#pragma once

#include "engine/math/common.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/utils/utils.hpp"

namespace engine::math::utils
{
    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> perspective(T fov_y, T aspect, T near, T far) noexcept
    {
        const T f = T(1) / utils::tan(fov_y / T(2));

        Matrix<T, 4, 4> result{};
        result[0][0] = f / aspect;
        result[1][1] = f;
        result[2][2] = (far + near) / (near - far);
        result[2][3] = (T(2) * far * near) / (near - far);
        result[3][2] = T(-1);
        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> orthographic(T left, T right, T bottom, T top, T near, T far) noexcept
    {
        Matrix<T, 4, 4> result{};
        result[0][0] = T(2) / (right - left);
        result[1][1] = T(2) / (top - bottom);
        result[2][2] = T(-2) / (far - near);
        result[0][3] = -(right + left) / (right - left);
        result[1][3] = -(top + bottom) / (top - bottom);
        result[2][3] = -(far + near) / (far - near);
        result[3][3] = T(1);
        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE T radians(T degrees) noexcept
    {
        return degrees * (static_cast<T>(3.14159265358979323846) / T(180));
    }

    template <typename T>
    ENGINE_MATH_INLINE T degrees(T radians) noexcept
    {
        return radians * (T(180) / static_cast<T>(3.14159265358979323846));
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> look_at(const Vector<T, 3>& eye, const Vector<T, 3>& center,
                                               const Vector<T, 3>& up) noexcept
    {
        const Vector<T, 3> forward = center - eye;
        const T min_length_sq = std::numeric_limits<T>::epsilon();
        if (length_squared(forward) < min_length_sq)
        {
            return identity_matrix<T, 4>();
        }
        const Vector<T, 3> f = normalize(forward);
        const Vector<T, 3> s = normalize(cross(f, up));
        const Vector<T, 3> u = cross(s, f);

        Matrix<T, 4, 4> result{};

        // Row 0: camera right vector components
        result[0][0] = s[0];
        result[0][1] = s[1];
        result[0][2] = s[2];
        result[0][3] = -dot(s, eye);

        // Row 1: camera up vector components
        result[1][0] = u[0];
        result[1][1] = u[1];
        result[1][2] = u[2];
        result[1][3] = -dot(u, eye);

        // Row 2: camera forward vector (negative because we look down -Z)
        result[2][0] = -f[0];
        result[2][1] = -f[1];
        result[2][2] = -f[2];
        result[2][3] = dot(f, eye);

        // Row 3: homogeneous row
        result[3][0] = T(0);
        result[3][1] = T(0);
        result[3][2] = T(0);
        result[3][3] = T(1);

        return result;
    }
} // namespace engine::math::utils
