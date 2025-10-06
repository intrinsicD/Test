#pragma once

#include "utils.hpp"
#include "engine/math/common.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/matrix.hpp"

namespace engine::math
{
    template <typename T>
    struct Quaternion
    {
        using value_type = T;

        value_type w;
        value_type x;
        value_type y;
        value_type z;

        ENGINE_MATH_INLINE Quaternion() noexcept
            : w(detail::zero<value_type>()),
              x(detail::zero<value_type>()),
              y(detail::zero<value_type>()),
              z(detail::zero<value_type>())
        {
        }

        ENGINE_MATH_INLINE Quaternion(value_type w_, value_type x_, value_type y_, value_type z_) noexcept
            : w(static_cast<value_type>(w_)),
              x(static_cast<value_type>(x_)),
              y(static_cast<value_type>(y_)),
              z(static_cast<value_type>(z_))
        {
        }

        ENGINE_MATH_INLINE Quaternion(value_type scalar, const Vector<T, 3>& vector_part) noexcept
            : Quaternion(scalar, vector_part[0], vector_part[1], vector_part[2])
        {
        }

        ENGINE_MATH_INLINE explicit Quaternion(const Vector<T, 3>& vector_part) noexcept
            : Quaternion(T(0), vector_part[0], vector_part[1], vector_part[2])
        {
        }

        ENGINE_MATH_INLINE static Quaternion Identity() noexcept
        {
            return Quaternion(detail::one<value_type>(),
                              detail::zero<value_type>(),
                              detail::zero<value_type>(),
                              detail::zero<value_type>());
        }

        ENGINE_MATH_INLINE value_type& operator[](std::size_t index) noexcept
        {
            switch (index)
            {
            case 0: return w;
            case 1: return x;
            case 2: return y;
            default: return z;
            }
        }

        ENGINE_MATH_INLINE value_type operator[](std::size_t index) const noexcept
        {
            switch (index)
            {
            case 0: return w;
            case 1: return x;
            case 2: return y;
            default: return z;
            }
        }

        ENGINE_MATH_INLINE Quaternion& operator+=(const Quaternion& rhs) noexcept
        {
            w += rhs.w;
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        ENGINE_MATH_INLINE Quaternion& operator-=(const Quaternion& rhs) noexcept
        {
            w -= rhs.w;
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        ENGINE_MATH_INLINE Quaternion& operator*=(value_type scalar) noexcept
        {
            w *= scalar;
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        ENGINE_MATH_INLINE Quaternion& operator/=(value_type scalar) noexcept
        {
            const value_type inv = detail::one<value_type>() / scalar;
            return (*this) *= inv;
        }
    };

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> operator+(Quaternion<T> lhs, const Quaternion<T>& rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> operator-(Quaternion<T> lhs, const Quaternion<T>& rhs) noexcept
    {
        lhs -= rhs;
        return lhs;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> operator*(Quaternion<T> lhs, T scalar) noexcept
    {
        lhs *= scalar;
        return lhs;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> operator*(T scalar, Quaternion<T> rhs) noexcept
    {
        rhs *= scalar;
        return rhs;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> operator/(Quaternion<T> lhs, T scalar) noexcept
    {
        lhs /= scalar;
        return lhs;
    }

    template <typename T>
    ENGINE_MATH_INLINE bool operator==(const Quaternion<T>& lhs, const Quaternion<T>& rhs) noexcept
    {
        return lhs.w == rhs.w && lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    template <typename T>
    ENGINE_MATH_INLINE bool operator!=(const Quaternion<T>& lhs, const Quaternion<T>& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> operator*(const Quaternion<T>& lhs, const Quaternion<T>& rhs) noexcept
    {
        return Quaternion<T>{
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
            lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
            lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
        };
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> conjugate(const Quaternion<T>& value) noexcept
    {
        return Quaternion<T>{value.w, -value.x, -value.y, -value.z};
    }

    template <typename T>
    ENGINE_MATH_INLINE T dot(const Quaternion<T>& lhs, const Quaternion<T>& rhs) noexcept
    {
        return lhs.w * rhs.w + lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    template <typename T>
    ENGINE_MATH_INLINE T length_squared(const Quaternion<T>& value) noexcept
    {
        return dot(value, value);
    }

    template <typename T>
    ENGINE_MATH_INLINE T length(const Quaternion<T>& value) noexcept
    {
        return static_cast<T>(::sqrt(static_cast<double>(length_squared(value))));
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> normalize(const Quaternion<T>& value) noexcept
    {
        const T len = length(value);
        if (len == detail::zero<T>())
        {
            return value;
        }
        return value / len;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> inverse(const Quaternion<T>& value) noexcept
    {
        const T len_sq = length_squared(value);
        if (len_sq == detail::zero<T>())
        {
            return Quaternion<T>{};
        }
        return conjugate(value) / len_sq;
    }


    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> from_angle_axis(T angle, const Vector<T, 3>& axis) noexcept
    {
        const T ax = axis[0];
        const T ay = axis[1];
        const T az = axis[2];

        const T len_sq = ax * ax + ay * ay + az * az;
        if (len_sq == detail::zero<T>() || angle == detail::zero<T>())
        {
            return Quaternion<T>{detail::one<T>(), detail::zero<T>(), detail::zero<T>(), detail::zero<T>()};
        }

        const T len = static_cast<T>(sqrt(static_cast<double>(len_sq)));
        const T half = angle * static_cast<T>(0.5);
        const T s = static_cast<T>(sin(static_cast<double>(half)));
        const T c = static_cast<T>(cos(static_cast<double>(half)));
        const T inv_len = detail::one<T>() / len;

        return Quaternion<T>{
            c,
            ax * inv_len * s,
            ay * inv_len * s,
            az * inv_len * s
        };
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> from_angle_axis(const Vector<T, 4>& value) noexcept
    {
        return from_angle_axis(
            value[0],
            Vector<T, 3>{value[1], value[2], value[3]}
        );
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> from_angle_axis(const Vector<T, 3>& value) noexcept
    {
        const T vx = value[0];
        const T vy = value[1];
        const T vz = value[2];

        const T angle = static_cast<T>(::sqrt(static_cast<double>(vx * vx + vy * vy + vz * vz)));
        if (angle == detail::zero<T>())
        {
            return Quaternion<T>{detail::one<T>(), detail::zero<T>(), detail::zero<T>(), detail::zero<T>()};
        }

        const T inv_angle = detail::one<T>() / angle;
        const Vector<T, 3> axis{vx * inv_angle, vy * inv_angle, vz * inv_angle};
        return from_angle_axis(angle, axis);
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> from_rotation_matrix(const Matrix<T, 3, 3>& value) noexcept
    {
        const T m00 = value[0][0];
        const T m01 = value[0][1];
        const T m02 = value[0][2];
        const T m10 = value[1][0];
        const T m11 = value[1][1];
        const T m12 = value[1][2];
        const T m20 = value[2][0];
        const T m21 = value[2][1];
        const T m22 = value[2][2];

        const T trace = m00 + m11 + m22;

        Quaternion<T> q;
        if (trace > detail::zero<T>())
        {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(static_cast<double>(trace + detail::one<T>())));
            q.w = static_cast<T>(0.25) * s;
            q.x = (m21 - m12) / s;
            q.y = (m02 - m20) / s;
            q.z = (m10 - m01) / s;
        }
        else if (m00 > m11 && m00 > m22)
        {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(
                static_cast<double>(detail::one<T>() + m00 - m11 - m22)));
            q.w = (m21 - m12) / s;
            q.x = static_cast<T>(0.25) * s;
            q.y = (m01 + m10) / s;
            q.z = (m02 + m20) / s;
        }
        else if (m11 > m22)
        {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(
                static_cast<double>(detail::one<T>() + m11 - m00 - m22)));
            q.w = (m02 - m20) / s;
            q.x = (m01 + m10) / s;
            q.y = static_cast<T>(0.25) * s;
            q.z = (m12 + m21) / s;
        }
        else
        {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(
                static_cast<double>(detail::one<T>() + m22 - m00 - m11)));
            q.w = (m10 - m01) / s;
            q.x = (m02 + m20) / s;
            q.y = (m12 + m21) / s;
            q.z = static_cast<T>(0.25) * s;
        }

        return normalize(q);
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> from_rotation_matrix(const Matrix<T, 4, 4>& value) noexcept
    {
        return from_rotation_matrix(Matrix<T, 3, 3>{
            value[0][0], value[0][1], value[0][2],
            value[1][0], value[1][1], value[1][2],
            value[2][0], value[2][1], value[2][2],
        });
    }

    template <typename S, typename T>
    ENGINE_MATH_INLINE Quaternion<S> cast(const Quaternion<T>& quat) noexcept
    {
        Quaternion<S> result;
        result.w = static_cast<S>(quat.w);
        result.x = static_cast<S>(quat.x);
        result.y = static_cast<S>(quat.y);
        result.z = static_cast<S>(quat.z);
        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> slerp(const Quaternion<T>& from, const Quaternion<T>& to, T t) noexcept
    {
        // Compute the cosine of the angle between the two quaternions
        T cos_theta = dot(from, to);
        Quaternion<T> to_interp = to;

        // Take shortest path
        if (cos_theta < T(0))
        {
            to_interp = Quaternion<T>{-to.w, -to.x, -to.y, -to.z};
            cos_theta = -cos_theta;
        }

        // Use lerp if very close
        if (cos_theta > T(0.9995))
        {
            return normalize((T(1) - t) * from + t * to_interp);
        }

        // Clamp to avoid acos domain errors
        cos_theta = utils::clamp(cos_theta, T(-1), T(1));

        const T theta = static_cast<T>(acos(static_cast<double>(cos_theta)));
        const T sin_theta = static_cast<T>(sin(static_cast<double>(theta)));

        const T ratio_a = static_cast<T>(sin(static_cast<double>((T(1) - t) * theta))) / sin_theta;
        const T ratio_b = static_cast<T>(sin(static_cast<double>(t * theta))) / sin_theta;

        return ratio_a * from + ratio_b * to_interp;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> squad(const Quaternion<T>& q1, const Quaternion<T>& q2,
                                           const Quaternion<T>& q3, const Quaternion<T>& q4, T t) noexcept
    {
        // Spherical and Quadrangle (squad) interpolation blends two SLERP arcs to achieve $C^1$ continuity.
        const Quaternion<T> slerp1 = slerp(q1, q4, t);
        const Quaternion<T> slerp2 = slerp(q2, q3, t);
        return slerp(slerp1, slerp2, static_cast<T>(2) * t * (detail::one<T>() - t));
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 4> to_angle_axis(const Quaternion<T>& quat) noexcept
    {
        Quaternion<T> q = normalize(quat);
        const T angle = static_cast<T>(2) * static_cast<T>(acos(static_cast<double>(q.w)));
        const T s = static_cast<T>(sqrt(static_cast<double>(1.0 - q.w * q.w)));

        if (s < static_cast<T>(0.001))
        {
            return Vector<T, 4>{angle, q.x, q.y, q.z};
        }

        return Vector<T, 4>{angle, q.x / s, q.y / s, q.z / s};
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 3> to_cayley_parameters(const Quaternion<T>& quat) noexcept
    {
        const Quaternion<T> normalized = normalize(quat);
        const T denom = detail::one<T>() + normalized.w;

        if (utils::nearly_equal(denom, detail::zero<T>()))
        {
            return Vector<T, 3>{detail::zero<T>()};
        }

        return Vector<T, 3>{
            normalized.x / denom,
            normalized.y / denom,
            normalized.z / denom,
        };
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> from_cayley_parameters(const Vector<T, 3>& cayley) noexcept
    {
        const T x = cayley[0];
        const T y = cayley[1];
        const T z = cayley[2];

        const T norm_sq = x * x + y * y + z * z;
        const T denom = detail::one<T>() + norm_sq;

        if (utils::nearly_equal(denom, detail::zero<T>()))
        {
            return Quaternion<T>::Identity();
        }

        return normalize(Quaternion<T>{
            (detail::one<T>() - norm_sq) / denom,
            (static_cast<T>(2) * x) / denom,
            (static_cast<T>(2) * y) / denom,
            (static_cast<T>(2) * z) / denom,
        });
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 3> to_euler_angles(const Quaternion<T>& quat) noexcept
    {
        // Normalize to avoid drift amplifying numerical issues
        const Quaternion<T> q = normalize(quat);
        const T w = q.w;
        const T x = q.x;
        const T y = q.y;
        const T z = q.z;

        // Roll (X axis rotation)
        const T sinr_cosp = static_cast<T>(2) * (w * x + y * z);
        const T cosr_cosp = detail::one<T>() - static_cast<T>(2) * (x * x + y * y);
        const T roll = static_cast<T>(::atan2(static_cast<double>(sinr_cosp),
                                              static_cast<double>(cosr_cosp)));

        // Pitch (Y axis rotation)
        T sinp = static_cast<T>(2) * (w * y - z * x);
        // Clamp to [-1, 1] to guard against slight numerical overshoot
        if (sinp > detail::one<T>()) sinp = detail::one<T>();
        if (sinp < -detail::one<T>()) sinp = -detail::one<T>();
        const T pitch = static_cast<T>(::asin(static_cast<double>(sinp)));

        // Yaw (Z axis rotation)
        const T siny_cosp = static_cast<T>(2) * (w * z + x * y);
        const T cosy_cosp = detail::one<T>() - static_cast<T>(2) * (y * y + z * z);
        const T yaw = static_cast<T>(::atan2(static_cast<double>(siny_cosp),
                                             static_cast<double>(cosy_cosp)));

        // Return (roll, pitch, yaw)
        return Vector<T, 3>{roll, pitch, yaw};
    }

    using quat = Quaternion<float>;
    using dquat = Quaternion<double>;
} // namespace engine::math
