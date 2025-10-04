#pragma once

#include "engine/math/common.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/utils.hpp"

namespace engine::math::utils
{
    template<typename T>
    ENGINE_MATH_INLINE matrix<T, 4, 4> perspective(T fov_y, T aspect, T near, T far) noexcept
    {
        const T f = T(1) / utils::tan(fov_y / T(2));
        matrix<T, 4, 4> result{
            f / aspect, T(0), T(0), T(0),
            T(0), f, T(0), T(0),
            T(0), T(0), (far + near) / (near - far), T(-1),
            T(0), T(0), (T(2) * far * near) / (near - far), T(0)
        };
        return result;
    }

    template<typename T>
ENGINE_MATH_INLINE matrix<T, 4, 4> orthographic(T left, T right, T bottom, T top, T near, T far) noexcept
    {
        matrix<T, 4, 4> result{
            T(2) / (right - left), T(0), T(0), T(0),
            T(0), T(2) / (top - bottom), T(0), T(0),
            T(0), T(0), T(-2) / (far - near), T(0),
            -(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), T(1)
        };
        return result;
    }

    template<typename T>
ENGINE_MATH_INLINE T radians(T degrees) noexcept
    {
        return degrees * (static_cast<T>(3.14159265358979323846) / T(180));
    }

    template<typename T>
    ENGINE_MATH_INLINE T degrees(T radians) noexcept
    {
        return radians * (T(180) / static_cast<T>(3.14159265358979323846));
    }

template <typename T>
ENGINE_MATH_INLINE matrix<T, 4, 4> look_at(const Vector<T, 3>& eye, const Vector<T, 3>& center, const Vector<T, 3>& up) noexcept
{
    const Vector<T, 3> f = normalize(center - eye);
    const Vector<T, 3> s = normalize(cross(f, up));
    const Vector<T, 3> u = cross(s, f);

    matrix<T, 4, 4> result{
        s[0], u[0], -f[0], T(0),
        s[1], u[1], -f[1], T(0),
        s[2], u[2], -f[2], T(0),
        T(0), T(0), T(0), T(1)
    };

    result[0][3] = -dot(s, eye);
    result[1][3] = -dot(u, eye);
    result[2][3] = dot(f, eye);

    return result;
}
} // namespace engine::math::utils