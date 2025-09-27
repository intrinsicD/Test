#pragma once

#include "engine/math/common.hpp"
#include "engine/math/vector.hpp"

namespace engine::math {

template <typename T>
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
          z(detail::zero<value_type>()) {}

    ENGINE_MATH_INLINE quaternion(value_type w_, value_type x_, value_type y_, value_type z_) noexcept
        : w(static_cast<value_type>(w_)),
          x(static_cast<value_type>(x_)),
          y(static_cast<value_type>(y_)),
          z(static_cast<value_type>(z_)) {}

    ENGINE_MATH_INLINE quaternion(value_type scalar, const vector<T, 3>& vector_part) noexcept
        : quaternion(scalar, vector_part[0], vector_part[1], vector_part[2]) {}

    ENGINE_MATH_INLINE value_type& operator[](std::size_t index) noexcept {
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

    ENGINE_MATH_INLINE quaternion& operator+=(const quaternion& rhs) noexcept {
        w += rhs.w;
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    ENGINE_MATH_INLINE quaternion& operator-=(const quaternion& rhs) noexcept {
        w -= rhs.w;
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    ENGINE_MATH_INLINE quaternion& operator*=(value_type scalar) noexcept {
        w *= scalar;
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    ENGINE_MATH_INLINE quaternion& operator/=(value_type scalar) noexcept {
        const value_type inv = detail::one<value_type>() / scalar;
        return (*this) *= inv;
    }
};

template <typename T>
ENGINE_MATH_INLINE quaternion<T> operator+(quaternion<T> lhs, const quaternion<T>& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> operator-(quaternion<T> lhs, const quaternion<T>& rhs) noexcept {
    lhs -= rhs;
    return lhs;
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> operator*(quaternion<T> lhs, T scalar) noexcept {
    lhs *= scalar;
    return lhs;
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> operator*(T scalar, quaternion<T> rhs) noexcept {
    rhs *= scalar;
    return rhs;
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> operator/(quaternion<T> lhs, T scalar) noexcept {
    lhs /= scalar;
    return lhs;
}

template <typename T>
ENGINE_MATH_INLINE bool operator==(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return lhs.w == rhs.w && lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

template <typename T>
ENGINE_MATH_INLINE bool operator!=(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> operator*(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return quaternion<T>{
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
        lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
    };
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> conjugate(const quaternion<T>& value) noexcept {
    return quaternion<T>{value.w, -value.x, -value.y, -value.z};
}

template <typename T>
ENGINE_MATH_INLINE T dot(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return lhs.w * rhs.w + lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

template <typename T>
ENGINE_MATH_INLINE T length_squared(const quaternion<T>& value) noexcept {
    return dot(value, value);
}

template <typename T>
ENGINE_MATH_INLINE T length(const quaternion<T>& value) noexcept {
    return static_cast<T>(::sqrt(static_cast<double>(length_squared(value))));
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> normalize(const quaternion<T>& value) noexcept {
    const T len = length(value);
    if (len == detail::zero<T>()) {
        return value;
    }
    return value / len;
}

template <typename T>
ENGINE_MATH_INLINE quaternion<T> inverse(const quaternion<T>& value) noexcept {
    const T len_sq = length_squared(value);
    if (len_sq == detail::zero<T>()) {
        return quaternion<T>{};
    }
    return conjugate(value) / len_sq;
}

using quat = quaternion<float>;
using dquat = quaternion<double>;

}  // namespace engine::math

