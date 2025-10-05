#pragma once

#include "engine/math/common.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/quaternion.hpp"

namespace engine::math::utils
{
    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> to_rotation_matrix(const Quaternion<T>& quat) noexcept
    {
        Matrix<T, 4, 4> result = identity_matrix<T, 4>();
        const T xx = quat.x * quat.x;
        const T yy = quat.y * quat.y;
        const T zz = quat.z * quat.z;
        const T xy = quat.x * quat.y;
        const T xz = quat.x * quat.z;
        const T yz = quat.y * quat.z;
        const T wx = quat.w * quat.x;
        const T wy = quat.w * quat.y;
        const T wz = quat.w * quat.z;

        result[0][0] = detail::one<T>() - static_cast<T>(2) * (yy + zz);
        result[0][1] = static_cast<T>(2) * (xy - wz);
        result[0][2] = static_cast<T>(2) * (xz + wy);

        result[1][0] = static_cast<T>(2) * (xy + wz);
        result[1][1] = detail::one<T>() - static_cast<T>(2) * (xx + zz);
        result[1][2] = static_cast<T>(2) * (yz - wx);

        result[2][0] = static_cast<T>(2) * (xz - wy);
        result[2][1] = static_cast<T>(2) * (yz + wx);
        result[2][2] = detail::one<T>() - static_cast<T>(2) * (xx + yy);

        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> to_quaternion(const Matrix<T, 3, 3>& rot) noexcept
    {
        const T m00 = rot[0][0], m01 = rot[0][1], m02 = rot[0][2];
        const T m10 = rot[1][0], m11 = rot[1][1], m12 = rot[1][2];
        const T m20 = rot[2][0], m21 = rot[2][1], m22 = rot[2][2];

        Quaternion<T> q{};
        const T trace = m00 + m11 + m22;
        if (trace > T(0))
        {
            T s = std::sqrt(trace + T(1)) * T(0.5);
            q.w = s;
            T inv4s = T(0.25) / s;
            q.x = (m21 - m12) * inv4s;
            q.y = (m02 - m20) * inv4s;
            q.z = (m10 - m01) * inv4s;
        }
        else
        {
            if (m00 > m11 && m00 > m22)
            {
                T s = std::sqrt(T(1) + m00 - m11 - m22) * T(0.5);
                T inv4s = T(0.25) / s;
                q.x = s;
                q.y = (m01 + m10) * inv4s;
                q.z = (m02 + m20) * inv4s;
                q.w = (m21 - m12) * inv4s;
            }
            else if (m11 > m22)
            {
                T s = std::sqrt(T(1) + m11 - m00 - m22) * T(0.5);
                T inv4s = T(0.25) / s;
                q.x = (m01 + m10) * inv4s;
                q.y = s;
                q.z = (m12 + m21) * inv4s;
                q.w = (m02 - m20) * inv4s;
            }
            else
            {
                T s = std::sqrt(T(1) + m22 - m00 - m11) * T(0.5);
                T inv4s = T(0.25) / s;
                q.x = (m02 + m20) * inv4s;
                q.y = (m12 + m21) * inv4s;
                q.z = s;
                q.w = (m10 - m01) * inv4s;
            }
        }
        // Normalize (safeguard)
        const T n2 = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
        if (n2 > T(0))
        {
            T inv = T(1) / std::sqrt(n2);
            q.w *= inv;
            q.x *= inv;
            q.y *= inv;
            q.z *= inv;
        }
        return q;
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> to_rotation_matrix(T angle, const Vector<T, 3>& axis) noexcept
    {
        //The input vector is expected to be a normalized axis of rotation
        Matrix<T, 4, 4> R = math::identity_matrix<T, 4>();
        const T eps = T(1e-8);

        // Normalize axis defensively
        T ax = axis[0], ay = axis[1], az = axis[2];
        T len2 = ax * ax + ay * ay + az * az;
        if (len2 > eps)
        {
            T invLen = T(1) / std::sqrt(len2);
            ax *= invLen;
            ay *= invLen;
            az *= invLen;
        }
        else
        {
            // Fallback identity if axis degenerate
            R[0][0] = R[1][1] = R[2][2] = T(1);
            R[0][1] = R[0][2] = R[1][0] = R[1][2] = R[2][0] = R[2][1] = T(0);
            return R;
        }

        const T s = std::sin(angle);
        const T c = std::cos(angle);
        const T one_c = T(1) - c;

        const T xx = ax * ax, yy = ay * ay, zz = az * az;
        const T xy = ax * ay, xz = ax * az, yz = ay * az;

        R[0][0] = c + xx * one_c;
        R[0][1] = xy * one_c - az * s;
        R[0][2] = xz * one_c + ay * s;

        R[1][0] = xy * one_c + az * s;
        R[1][1] = c + yy * one_c;
        R[1][2] = yz * one_c - ax * s;

        R[2][0] = xz * one_c - ay * s;
        R[2][1] = yz * one_c + ax * s;
        R[2][2] = c + zz * one_c;

        return R;
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> to_rotation_matrix(const Vector<T, 3>& angle_axis) noexcept
    {
        //The input vector is expected to be (axis.x, axis.y, axis.z) * angle
        const T x = angle_axis[0];
        const T y = angle_axis[1];
        const T z = angle_axis[2];
        const T angle = std::sqrt(x * x + y * y + z * z);
        const T eps = T(1e-8);

        if (angle < eps)
        {
            return math::identity_matrix<T, 4>();
        }

        Vector<T, 3> axis{};
        axis[0] = x / angle;
        axis[1] = y / angle;
        axis[2] = z / angle;
        return to_rotation_matrix(angle, axis);
    }


    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> to_rotation_matrix(const Vector<T, 4>& angle_axis) noexcept
    {
        //The input vector is expected to be (angle, axis.x, axis.y, axis.z)
        const T angle = angle_axis[0];
        Vector<T, 3> axis{};
        axis[0] = angle_axis[1];
        axis[1] = angle_axis[2];
        axis[2] = angle_axis[3];
        return to_rotation_matrix(angle, axis);
    }
} // namespace engine::math::utils
