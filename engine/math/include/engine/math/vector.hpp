#pragma once

#include "engine/math/common.hpp"

namespace engine::math {

template <typename T, std::size_t N>
struct vector {
    static_assert(N > 0, "vector dimension must be positive");

    using value_type = T;
    using size_type = std::size_t;

    value_type elements[N];

    ENGINE_MATH_INLINE vector() noexcept : elements{} {}

    ENGINE_MATH_INLINE explicit vector(value_type scalar) noexcept : elements{} {
        for (size_type i = 0; i < N; ++i) {
            elements[i] = scalar;
        }
    }

    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
    ENGINE_MATH_INLINE vector(Args... args) noexcept : elements{static_cast<value_type>(args)...} {}

    ENGINE_MATH_INLINE value_type& operator[](size_type index) noexcept { return elements[index]; }

    ENGINE_MATH_INLINE value_type operator[](size_type index) const noexcept { return elements[index]; }

    ENGINE_MATH_INLINE vector& operator+=(const vector& rhs) noexcept {
        for (size_type i = 0; i < N; ++i) {
            elements[i] += rhs.elements[i];
        }
        return *this;
    }

    ENGINE_MATH_INLINE vector& operator-=(const vector& rhs) noexcept {
        for (size_type i = 0; i < N; ++i) {
            elements[i] -= rhs.elements[i];
        }
        return *this;
    }

    ENGINE_MATH_INLINE vector& operator*=(value_type scalar) noexcept {
        for (size_type i = 0; i < N; ++i) {
            elements[i] *= scalar;
        }
        return *this;
    }

    ENGINE_MATH_INLINE vector& operator/=(value_type scalar) noexcept {
        const value_type inv = detail::one<value_type>() / scalar;
        return (*this) *= inv;
    }
};

template <typename T, std::size_t N>
ENGINE_MATH_INLINE vector<T, N> operator+(vector<T, N> lhs, const vector<T, N>& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE vector<T, N> operator-(vector<T, N> lhs, const vector<T, N>& rhs) noexcept {
    lhs -= rhs;
    return lhs;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE vector<T, N> operator*(vector<T, N> lhs, T scalar) noexcept {
    lhs *= scalar;
    return lhs;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE vector<T, N> operator*(T scalar, vector<T, N> rhs) noexcept {
    rhs *= scalar;
    return rhs;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE vector<T, N> operator/(vector<T, N> lhs, T scalar) noexcept {
    lhs /= scalar;
    return lhs;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE bool operator==(const vector<T, N>& lhs, const vector<T, N>& rhs) noexcept {
    for (typename vector<T, N>::size_type i = 0; i < N; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE bool operator!=(const vector<T, N>& lhs, const vector<T, N>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE T dot(const vector<T, N>& lhs, const vector<T, N>& rhs) noexcept {
    T result = detail::zero<T>();
    for (typename vector<T, N>::size_type i = 0; i < N; ++i) {
        result += lhs[i] * rhs[i];
    }
    return result;
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE T length_squared(const vector<T, N>& value) noexcept {
    return dot(value, value);
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE T length(const vector<T, N>& value) noexcept {
    return static_cast<T>(::sqrt(static_cast<double>(length_squared(value))));
}

template <typename T, std::size_t N>
ENGINE_MATH_INLINE vector<T, N> normalize(const vector<T, N>& value) noexcept {
    const T len = length(value);
    if (len == detail::zero<T>()) {
        return value;
    }
    return value / len;
}

template <typename T>
ENGINE_MATH_INLINE vector<T, 3> cross(const vector<T, 3>& lhs, const vector<T, 3>& rhs) noexcept {
    return vector<T, 3>{
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

using vec2 = vector<float, 2>;
using vec3 = vector<float, 3>;
using vec4 = vector<float, 4>;

using dvec2 = vector<double, 2>;
using dvec3 = vector<double, 3>;
using dvec4 = vector<double, 4>;

using ivec2 = vector<int, 2>;
using ivec3 = vector<int, 3>;
using ivec4 = vector<int, 4>;

}  // namespace engine::math
