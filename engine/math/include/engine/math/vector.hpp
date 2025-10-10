#pragma once

#include "engine/math/common.hpp"

#include <ostream>
#include <cassert>

namespace engine::math
{
    template <typename T, std::size_t N>
    struct Vector
    {
        static_assert(N > 0, "vector dimension must be positive");

        using value_type = T;
        using size_type = std::size_t;

        value_type elements[N];

        ENGINE_MATH_INLINE Vector() noexcept : elements{}
        {
        }

        ENGINE_MATH_INLINE explicit Vector(value_type scalar) noexcept : elements{}
        {
            for (size_type i = 0; i < N; ++i)
            {
                elements[i] = scalar;
            }
        }

        template <typename S>
        ENGINE_MATH_INLINE Vector(const Vector<S, N>& other) noexcept : elements{}
        {
            for (size_type i = 0; i < N; ++i)
            {
                elements[i] = other[i];
            }
        }

        template <typename S>
        ENGINE_MATH_INLINE Vector(const Vector<S, N + 1>& other) noexcept : elements{}
        {
            for (size_type i = 0; i < N; ++i)
            {
                elements[i] = other[i];
            }
        }

        template <typename S>
        ENGINE_MATH_INLINE Vector(const Vector<S, N - 1>& other) noexcept : elements{}
        {
            for (size_type i = 0; i < N - 1; ++i)
            {
                elements[i] = other[i];
            }
            elements[N - 1] = detail::one<value_type>();
        }

        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
        ENGINE_MATH_INLINE Vector(Args... args) noexcept : elements{static_cast<value_type>(args)...}
        {
        }

        ENGINE_MATH_INLINE value_type& operator[](size_type index) noexcept
        {
            assert(index < N && "vector index out of bounds");
            return elements[index];
        }

        ENGINE_MATH_INLINE value_type operator[](size_type index) const noexcept
        {
            assert(index < N && "vector index out of bounds");
            return elements[index];
        }

        ENGINE_MATH_INLINE Vector operator-() const noexcept
        {
            Vector result;
            for (size_type i = 0; i < N; ++i)
            {
                result[i] = -elements[i];
            }
            return result;
        }

        ENGINE_MATH_INLINE Vector& operator+=(const Vector& rhs) noexcept
        {
            for (size_type i = 0; i < N; ++i)
            {
                elements[i] += rhs.elements[i];
            }
            return *this;
        }

        ENGINE_MATH_INLINE Vector& operator-=(const Vector& rhs) noexcept
        {
            for (size_type i = 0; i < N; ++i)
            {
                elements[i] -= rhs.elements[i];
            }
            return *this;
        }

        ENGINE_MATH_INLINE Vector& operator*=(value_type scalar) noexcept
        {
            for (size_type i = 0; i < N; ++i)
            {
                elements[i] *= scalar;
            }
            return *this;
        }

        ENGINE_MATH_INLINE Vector& operator/=(value_type scalar) noexcept
        {
            const value_type inv = detail::one<value_type>() / scalar;
            return (*this) *= inv;
        }

        ENGINE_MATH_INLINE T* data() noexcept { return elements; }

        ENGINE_MATH_INLINE const T* data() const noexcept { return elements; }
    };

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> operator+(Vector<T, N> lhs, const Vector<T, N>& rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> operator-(Vector<T, N> lhs, const Vector<T, N>& rhs) noexcept
    {
        lhs -= rhs;
        return lhs;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> operator*(Vector<T, N> lhs, T scalar) noexcept
    {
        lhs *= scalar;
        return lhs;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> operator*(T scalar, Vector<T, N> rhs) noexcept
    {
        rhs *= scalar;
        return rhs;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> operator/(Vector<T, N> lhs, T scalar) noexcept
    {
        lhs /= scalar;
        return lhs;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE bool operator==(const Vector<T, N>& lhs, const Vector<T, N>& rhs) noexcept
    {
        for (typename Vector<T, N>::size_type i = 0; i < N; ++i)
        {
            if (lhs[i] != rhs[i])
            {
                return false;
            }
        }
        return true;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE bool operator!=(const Vector<T, N>& lhs, const Vector<T, N>& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    template <typename S, typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<S, N> cast(const Vector<T, N>& vec) noexcept
    {
        Vector<S, N> result;
        for (typename Vector<S, N>::size_type i = 0; i < N; ++i)
        {
            result[i] = static_cast<S>(vec[i]);
        }
        return result;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE T dot(const Vector<T, N>& lhs, const Vector<T, N>& rhs) noexcept
    {
        T result = detail::zero<T>();
        for (typename Vector<T, N>::size_type i = 0; i < N; ++i)
        {
            result += lhs[i] * rhs[i];
        }
        return result;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE T length_squared(const Vector<T, N>& value) noexcept
    {
        return dot(value, value);
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE T length(const Vector<T, N>& value) noexcept
    {
        return static_cast<T>(::sqrt(static_cast<double>(length_squared(value))));
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> normalize(const Vector<T, N>& value) noexcept
    {
        const T len = length(value);
        if (len == detail::zero<T>())
        {
            return value;
        }
        return value / len;
    }

    template <typename T>
    ENGINE_MATH_INLINE Vector<T, 3> cross(const Vector<T, 3>& lhs, const Vector<T, 3>& rhs) noexcept
    {
        return Vector<T, 3>{
            lhs[1] * rhs[2] - lhs[2] * rhs[1],
            lhs[2] * rhs[0] - lhs[0] * rhs[2],
            lhs[0] * rhs[1] - lhs[1] * rhs[0],
        };
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> reflect(const Vector<T, N>& incident, const Vector<T, N>& normal) noexcept
    {
        // Reflection across a plane with normal $n$ follows $r = i - 2\langle i, n \rangle n$.
        const auto normalized_normal = normalize(normal);
        return incident - static_cast<T>(2) * dot(incident, normalized_normal) * normalized_normal;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> refract(const Vector<T, N>& incident, const Vector<T, N>& normal, T eta) noexcept
    {
        const Vector<T, N> unit_incident = normalize(incident);
        const Vector<T, N> unit_normal = normalize(normal);

        const T cos_i = dot(-unit_incident, unit_normal);
        const T sin2_t = eta * eta * (detail::one<T>() - cos_i * cos_i);
        if (sin2_t > detail::one<T>())
        {
            return Vector<T, N>{}; // Total internal reflection
        }
        const T cos_t = static_cast<T>(sqrt(static_cast<double>(detail::one<T>() - sin2_t)));
        return eta * unit_incident + (eta * cos_i - cos_t) * unit_normal;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE T projection_coefficient(const Vector<T, N>& a, const Vector<T, N>& b) noexcept
    {
        const T b_len_sq = length_squared(b);
        if (b_len_sq == detail::zero<T>())
        {
            return detail::zero<T>();
        }
        return dot(a, b) / b_len_sq;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> project(const Vector<T, N>& a, const Vector<T, N>& b) noexcept
    {
        return projection_coefficient(a, b) * b;
    }

    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE Vector<T, N> lerp(const Vector<T, N>& a, const Vector<T, N>& b, T t) noexcept
    {
        return (detail::one<T>() - t) * a + t * b;
    }

    template <typename T, std::size_t N>
    std::ostream& operator<<(std::ostream& os, const Vector<T, N>& vector)
    {
        os << "(";
        for (typename Vector<T, N>::size_type i = 0; i < N; ++i)
        {
            os << vector[i];
            if (i < N - 1)
            {
                os << ", ";
            }
        }
        os << ")";
        return os;
    }

    using vec2 = Vector<float, 2>;
    using vec3 = Vector<float, 3>;
    using vec4 = Vector<float, 4>;

    using dvec2 = Vector<double, 2>;
    using dvec3 = Vector<double, 3>;
    using dvec4 = Vector<double, 4>;

    using ivec2 = Vector<int, 2>;
    using ivec3 = Vector<int, 3>;
    using ivec4 = Vector<int, 4>;
} // namespace engine::math
