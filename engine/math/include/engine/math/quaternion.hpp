#pragma once

#include "engine/math/common.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/matrix.hpp"

namespace engine::math {
    template<typename T>
    struct quaternion {
        using value_type = T;

        value_type w;
        value_type x;
        value_type y;
        value_type z;

        ENGINE_MATH_INLINE quaternion() noexcept
            : w(detail::zero<value_type>()),
              x(detail::zero<value_type>()),
              y(detail::zero<value_type>()),
              z(detail::zero<value_type>()) {
        }

        ENGINE_MATH_INLINE quaternion(value_type w_, value_type x_, value_type y_, value_type z_) noexcept
            : w(static_cast<value_type>(w_)),
              x(static_cast<value_type>(x_)),
              y(static_cast<value_type>(y_)),
              z(static_cast<value_type>(z_)) {
        }

        ENGINE_MATH_INLINE quaternion(value_type scalar, const Vector<T, 3> &vector_part) noexcept
            : quaternion(scalar, vector_part[0], vector_part[1], vector_part[2]) {
        }

        ENGINE_MATH_INLINE explicit quaternion(const Vector<T, 3> &vector_part) noexcept
            : quaternion(T(0), vector_part[0], vector_part[1], vector_part[2]) {
        }

        ENGINE_MATH_INLINE value_type &operator[](std::size_t index) noexcept {
            switch (index) {
                case 0: return w;
                case 1: return x;
                case 2: return y;
                default: return z;
            }
        }

        ENGINE_MATH_INLINE value_type operator[](std::size_t index) const noexcept {
            switch (index) {
                case 0: return w;
                case 1: return x;
                case 2: return y;
                default: return z;
            }
        }

        ENGINE_MATH_INLINE quaternion &operator+=(const quaternion &rhs) noexcept {
            w += rhs.w;
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        ENGINE_MATH_INLINE quaternion &operator-=(const quaternion &rhs) noexcept {
            w -= rhs.w;
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        ENGINE_MATH_INLINE quaternion &operator*=(value_type scalar) noexcept {
            w *= scalar;
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        ENGINE_MATH_INLINE quaternion &operator/=(value_type scalar) noexcept {
            const value_type inv = detail::one<value_type>() / scalar;
            return (*this) *= inv;
        }

        ENGINE_MATH_INLINE matrix<T, 3, 3> to_rotation_matrix() const noexcept {
            matrix<T, 3, 3> result;
            const T xx = x * x;
            const T yy = y * y;
            const T zz = z * z;
            const T xy = x * y;
            const T xz = x * z;
            const T yz = y * z;
            const T wx = w * x;
            const T wy = w * y;
            const T wz = w * z;

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
    };

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> operator+(quaternion<T> lhs, const quaternion<T> &rhs) noexcept {
        lhs += rhs;
        return lhs;
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> operator-(quaternion<T> lhs, const quaternion<T> &rhs) noexcept {
        lhs -= rhs;
        return lhs;
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> operator*(quaternion<T> lhs, T scalar) noexcept {
        lhs *= scalar;
        return lhs;
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> operator*(T scalar, quaternion<T> rhs) noexcept {
        rhs *= scalar;
        return rhs;
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> operator/(quaternion<T> lhs, T scalar) noexcept {
        lhs /= scalar;
        return lhs;
    }

    template<typename T>
    ENGINE_MATH_INLINE bool operator==(const quaternion<T> &lhs, const quaternion<T> &rhs) noexcept {
        return lhs.w == rhs.w && lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    template<typename T>
    ENGINE_MATH_INLINE bool operator!=(const quaternion<T> &lhs, const quaternion<T> &rhs) noexcept {
        return !(lhs == rhs);
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> operator*(const quaternion<T> &lhs, const quaternion<T> &rhs) noexcept {
        return quaternion<T>{
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
            lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
            lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
        };
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> conjugate(const quaternion<T> &value) noexcept {
        return quaternion<T>{value.w, -value.x, -value.y, -value.z};
    }

    template<typename T>
    ENGINE_MATH_INLINE T dot(const quaternion<T> &lhs, const quaternion<T> &rhs) noexcept {
        return lhs.w * rhs.w + lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    template<typename T>
    ENGINE_MATH_INLINE T length_squared(const quaternion<T> &value) noexcept {
        return dot(value, value);
    }

    template<typename T>
    ENGINE_MATH_INLINE T length(const quaternion<T> &value) noexcept {
        return static_cast<T>(::sqrt(static_cast<double>(length_squared(value))));
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> normalize(const quaternion<T> &value) noexcept {
        const T len = length(value);
        if (len == detail::zero<T>()) {
            return value;
        }
        return value / len;
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> inverse(const quaternion<T> &value) noexcept {
        const T len_sq = length_squared(value);
        if (len_sq == detail::zero<T>()) {
            return quaternion<T>{};
        }
        return conjugate(value) / len_sq;
    }


    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> from_angle_axis(T angle, const Vector<T, 3> &axis) noexcept {
        const T ax = axis[0];
        const T ay = axis[1];
        const T az = axis[2];

        const T len_sq = ax * ax + ay * ay + az * az;
        if (len_sq == detail::zero<T>() || angle == detail::zero<T>()) {
            return quaternion<T>{detail::one<T>(), detail::zero<T>(), detail::zero<T>(), detail::zero<T>()};
        }

        const T len = static_cast<T>(sqrt(static_cast<double>(len_sq)));
        const T half = angle * static_cast<T>(0.5);
        const T s = static_cast<T>(sin(static_cast<double>(half)));
        const T c = static_cast<T>(cos(static_cast<double>(half)));
        const T inv_len = detail::one<T>() / len;

        return quaternion<T>{
            c,
            ax * inv_len * s,
            ay * inv_len * s,
            az * inv_len * s
        };
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> from_angle_axis(const Vector<T, 4> &value) noexcept {
        return from_angle_axis(
            value[0],
            Vector<T, 3>{value[1], value[2], value[3]}
        );
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> from_angle_axis(const Vector<T, 3> &value) noexcept {
        const T vx = value[0];
        const T vy = value[1];
        const T vz = value[2];

        const T angle = static_cast<T>(::sqrt(static_cast<double>(vx * vx + vy * vy + vz * vz)));
        if (angle == detail::zero<T>()) {
            return quaternion<T>{detail::one<T>(), detail::zero<T>(), detail::zero<T>(), detail::zero<T>()};
        }

        const T inv_angle = detail::one<T>() / angle;
        const Vector<T, 3> axis{vx * inv_angle, vy * inv_angle, vz * inv_angle};
        return from_angle_axis(angle, axis);
    }

    template<typename T>
    ENGINE_MATH_INLINE quaternion<T> from_rotation_matrix(const matrix<T, 3, 3> &value) noexcept {
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

        quaternion<T> q;
        if (trace > detail::zero<T>()) {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(static_cast<double>(trace + detail::one<T>())));
            q.w = static_cast<T>(0.25) * s;
            q.x = (m21 - m12) / s;
            q.y = (m02 - m20) / s;
            q.z = (m10 - m01) / s;
        } else if (m00 > m11 && m00 > m22) {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(
                            static_cast<double>(detail::one<T>() + m00 - m11 - m22)));
            q.w = (m21 - m12) / s;
            q.x = static_cast<T>(0.25) * s;
            q.y = (m01 + m10) / s;
            q.z = (m02 + m20) / s;
        } else if (m11 > m22) {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(
                            static_cast<double>(detail::one<T>() + m11 - m00 - m22)));
            q.w = (m02 - m20) / s;
            q.x = (m01 + m10) / s;
            q.y = static_cast<T>(0.25) * s;
            q.z = (m12 + m21) / s;
        } else {
            const T s = static_cast<T>(2) * static_cast<T>(::sqrt(
                            static_cast<double>(detail::one<T>() + m22 - m00 - m11)));
            q.w = (m10 - m01) / s;
            q.x = (m02 + m20) / s;
            q.y = (m12 + m21) / s;
            q.z = static_cast<T>(0.25) * s;
        }

        return normalize(q);
    }

    using quat = quaternion<float>;
    using dquat = quaternion<double>;
} // namespace engine::math
