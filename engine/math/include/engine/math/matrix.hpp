#pragma once

#include "engine/math/vector.hpp"

namespace engine::math {

template <typename T, std::size_t Rows, std::size_t Cols>
struct matrix {
    using value_type = T;
    using row_type = vector<T, Cols>;
    using size_type = std::size_t;

    row_type rows[Rows];

    ENGINE_MATH_INLINE matrix() noexcept : rows{} {}

    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Rows * Cols>>
    ENGINE_MATH_INLINE matrix(Args... args) noexcept : rows{} {
        const value_type values[] = {static_cast<value_type>(args)...};
        size_type index = 0U;
        for (size_type r = 0; r < Rows; ++r) {
            for (size_type c = 0; c < Cols; ++c) {
                rows[r][c] = values[index++];
            }
        }
    }

    ENGINE_MATH_INLINE row_type& operator[](size_type row) noexcept { return rows[row]; }
    ENGINE_MATH_INLINE const row_type& operator[](size_type row) const noexcept { return rows[row]; }

    ENGINE_MATH_INLINE matrix& operator+=(const matrix& rhs) noexcept {
        for (size_type r = 0; r < Rows; ++r) {
            rows[r] += rhs.rows[r];
        }
        return *this;
    }

    ENGINE_MATH_INLINE matrix& operator-=(const matrix& rhs) noexcept {
        for (size_type r = 0; r < Rows; ++r) {
            rows[r] -= rhs.rows[r];
        }
        return *this;
    }

    ENGINE_MATH_INLINE matrix& operator*=(value_type scalar) noexcept {
        for (size_type r = 0; r < Rows; ++r) {
            rows[r] *= scalar;
        }
        return *this;
    }
};

template <typename T, std::size_t Rows, std::size_t Cols>
ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator+(matrix<T, Rows, Cols> lhs, const matrix<T, Rows, Cols>& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

template <typename T, std::size_t Rows, std::size_t Cols>
ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator-(matrix<T, Rows, Cols> lhs, const matrix<T, Rows, Cols>& rhs) noexcept {
    lhs -= rhs;
    return lhs;
}

template <typename T, std::size_t Rows, std::size_t Cols>
ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator*(matrix<T, Rows, Cols> lhs, T scalar) noexcept {
    lhs *= scalar;
    return lhs;
}

template <typename T, std::size_t Rows, std::size_t Cols>
ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator*(T scalar, matrix<T, Rows, Cols> rhs) noexcept {
    rhs *= scalar;
    return rhs;
}

template <typename T, std::size_t Rows, std::size_t Cols>
ENGINE_MATH_INLINE vector<T, Rows> operator*(const matrix<T, Rows, Cols>& lhs, const vector<T, Cols>& rhs) noexcept {
    vector<T, Rows> result{};
    for (std::size_t r = 0; r < Rows; ++r) {
        result[r] = dot(lhs[r], rhs);
    }
    return result;
}

template <typename T, std::size_t Rows, std::size_t Shared, std::size_t Cols>
ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator*(const matrix<T, Rows, Shared>& lhs, const matrix<T, Shared, Cols>& rhs) noexcept {
    matrix<T, Rows, Cols> result{};
    for (std::size_t r = 0; r < Rows; ++r) {
        for (std::size_t c = 0; c < Cols; ++c) {
            T value = detail::zero<T>();
            for (std::size_t k = 0; k < Shared; ++k) {
                value += lhs[r][k] * rhs[k][c];
            }
            result[r][c] = value;
        }
    }
    return result;
}

template <typename T, std::size_t Rows, std::size_t Cols>
ENGINE_MATH_INLINE matrix<T, Cols, Rows> transpose(const matrix<T, Rows, Cols>& m) noexcept {
    matrix<T, Cols, Rows> result{};
    for (std::size_t r = 0; r < Rows; ++r) {
        for (std::size_t c = 0; c < Cols; ++c) {
            result[c][r] = m[r][c];
        }
    }
    return result;
}

template <typename T, std::size_t Dim>
ENGINE_MATH_INLINE matrix<T, Dim, Dim> identity_matrix() noexcept {
    matrix<T, Dim, Dim> result{};
    for (std::size_t i = 0; i < Dim; ++i) {
        result[i][i] = detail::one<T>();
    }
    return result;
}

template <typename T>
ENGINE_MATH_INLINE matrix<T, 4, 4> translation(const vector<T, 3>& offset) noexcept {
    auto result = identity_matrix<T, 4>();
    result[0][3] = offset[0];
    result[1][3] = offset[1];
    result[2][3] = offset[2];
    return result;
}

template <typename T>
ENGINE_MATH_INLINE matrix<T, 4, 4> scale(const vector<T, 3>& factors) noexcept {
    matrix<T, 4, 4> result{};
    result[0][0] = factors[0];
    result[1][1] = factors[1];
    result[2][2] = factors[2];
    result[3][3] = detail::one<T>();
    return result;
}

using mat2 = matrix<float, 2, 2>;
using mat3 = matrix<float, 3, 3>;
using mat4 = matrix<float, 4, 4>;

using dmat3 = matrix<double, 3, 3>;
using dmat4 = matrix<double, 4, 4>;

}  // namespace engine::math
