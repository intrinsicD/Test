#pragma once

#include "engine/math/vector.hpp"

namespace engine::math {
    template<typename T, std::size_t Rows, std::size_t Cols>
    struct matrix {
        using value_type = T;
        using row_type = Vector<T, Cols>;
        using size_type = std::size_t;

        row_type rows[Rows];

        ENGINE_MATH_INLINE matrix() noexcept : rows{} {
        }

        template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == Rows * Cols> >
        ENGINE_MATH_INLINE matrix(Args... args) noexcept : rows{} {
            const value_type values[] = {static_cast<value_type>(args)...};
            size_type index = 0U;
            for (size_type r = 0; r < Rows; ++r) {
                for (size_type c = 0; c < Cols; ++c) {
                    rows[r][c] = values[index++];
                }
            }
        }

        ENGINE_MATH_INLINE row_type &operator[](size_type row) noexcept { return rows[row]; }
        ENGINE_MATH_INLINE const row_type &operator[](size_type row) const noexcept { return rows[row]; }

        ENGINE_MATH_INLINE matrix &operator+=(const matrix &rhs) noexcept {
            for (size_type r = 0; r < Rows; ++r) {
                rows[r] += rhs.rows[r];
            }
            return *this;
        }

        ENGINE_MATH_INLINE matrix &operator-=(const matrix &rhs) noexcept {
            for (size_type r = 0; r < Rows; ++r) {
                rows[r] -= rhs.rows[r];
            }
            return *this;
        }

        ENGINE_MATH_INLINE matrix &operator*=(value_type scalar) noexcept {
            for (size_type r = 0; r < Rows; ++r) {
                rows[r] *= scalar;
            }
            return *this;
        }
    };

    template<typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator+(matrix<T, Rows, Cols> lhs,
                                                       const matrix<T, Rows, Cols> &rhs) noexcept {
        lhs += rhs;
        return lhs;
    }

    template<typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator-(matrix<T, Rows, Cols> lhs,
                                                       const matrix<T, Rows, Cols> &rhs) noexcept {
        lhs -= rhs;
        return lhs;
    }

    template<typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator*(matrix<T, Rows, Cols> lhs, T scalar) noexcept {
        lhs *= scalar;
        return lhs;
    }

    template<typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator*(T scalar, matrix<T, Rows, Cols> rhs) noexcept {
        rhs *= scalar;
        return rhs;
    }

    template<typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Vector<T, Rows> operator
    *(const matrix<T, Rows, Cols> &lhs, const Vector<T, Cols> &rhs) noexcept {
        Vector<T, Rows> result{};
        for (std::size_t r = 0; r < Rows; ++r) {
            result[r] = dot(lhs[r], rhs);
        }
        return result;
    }

    template<typename T, std::size_t Rows, std::size_t Shared, std::size_t Cols>
    ENGINE_MATH_INLINE matrix<T, Rows, Cols> operator*(const matrix<T, Rows, Shared> &lhs,
                                                       const matrix<T, Shared, Cols> &rhs) noexcept {
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

    template<typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE matrix<T, Cols, Rows> transpose(const matrix<T, Rows, Cols> &m) noexcept {
        matrix<T, Cols, Rows> result{};
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result[c][r] = m[r][c];
            }
        }
        return result;
    }

    template<typename T, std::size_t Dim>
    ENGINE_MATH_INLINE matrix<T, Dim, Dim> identity_matrix() noexcept {
        matrix<T, Dim, Dim> result{};
        for (std::size_t i = 0; i < Dim; ++i) {
            result[i][i] = detail::one<T>();
        }
        return result;
    }

    template<typename T>
    ENGINE_MATH_INLINE matrix<T, 2, 2> inverse(const matrix<T, 2, 2> &m) noexcept {
        const T a = m[0][0], b = m[0][1];
        const T c = m[1][0], d = m[1][1];
        const T det = a * d - b * c;
        matrix<T, 2, 2> r{};
        if (det == detail::zero<T>()) { return r; }
        const T inv = detail::one<T>() / det;
        r[0][0] =  d * inv; r[0][1] = -b * inv;
        r[1][0] = -c * inv; r[1][1] =  a * inv;
        return r;
    }

    template<typename T>
    ENGINE_MATH_INLINE matrix<T, 3, 3> inverse(const matrix<T, 3, 3> &m) noexcept {
        const T a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
        const T a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
        const T a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];

        const T c00 =  a11 * a22 - a12 * a21;
        const T c01 = -(a10 * a22 - a12 * a20);
        const T c02 =  a10 * a21 - a11 * a20;

        const T c10 = -(a01 * a22 - a02 * a21);
        const T c11 =  a00 * a22 - a02 * a20;
        const T c12 = -(a00 * a21 - a01 * a20);

        const T c20 =  a01 * a12 - a02 * a11;
        const T c21 = -(a00 * a12 - a02 * a10);
        const T c22 =  a00 * a11 - a01 * a10;

        const T det = a00 * c00 + a01 * c01 + a02 * c02;
        matrix<T, 3, 3> r{};
        if (det == detail::zero<T>()) { return r; }
        const T inv = detail::one<T>() / det;

        // adjugate^T / det
        r[0][0] = c00 * inv; r[0][1] = c10 * inv; r[0][2] = c20 * inv;
        r[1][0] = c01 * inv; r[1][1] = c11 * inv; r[1][2] = c21 * inv;
        r[2][0] = c02 * inv; r[2][1] = c12 * inv; r[2][2] = c22 * inv;
        return r;
    }

    template<typename T>
    ENGINE_MATH_INLINE matrix<T, 4, 4> inverse(const matrix<T, 4, 4> &m) noexcept {
        // Build augmented [M | I]
        T aug[4][8]{};
        for (std::size_t r = 0; r < 4; ++r) {
            for (std::size_t c = 0; c < 4; ++c) aug[r][c] = m[r][c];
            for (std::size_t c = 0; c < 4; ++c) aug[r][4 + c] = (r == c) ? detail::one<T>() : detail::zero<T>();
        }

        // Forward elimination
        for (std::size_t col = 0; col < 4; ++col) {
            // pivot row
            std::size_t piv = col;
            T maxA = (aug[piv][col] < T(0) ? -aug[piv][col] : aug[piv][col]);
            for (std::size_t r = col + 1; r < 4; ++r) {
                T v = aug[r][col]; v = (v < T(0) ? -v : v);
                if (v > maxA) { maxA = v; piv = r; }
            }
            if (maxA == detail::zero<T>()) { return matrix<T, 4, 4>{}; } // singular

            // swap rows
            if (piv != col) {
                for (std::size_t c = 0; c < 8; ++c) { const T tmp = aug[piv][c]; aug[piv][c] = aug[col][c]; aug[col][c] = tmp; }
            }

            const T pivot = aug[col][col];
            const T invp = detail::one<T>() / pivot;

            // normalize pivot row
            for (std::size_t c = 0; c < 8; ++c) aug[col][c] *= invp;

            // eliminate other rows
            for (std::size_t r = 0; r < 4; ++r) {
                if (r == col) continue;
                const T f = aug[r][col];
                if (f == detail::zero<T>()) continue;
                for (std::size_t c = 0; c < 8; ++c) aug[r][c] -= f * aug[col][c];
            }
        }

        // Extract inverse
        matrix<T, 4, 4> inv{};
        for (std::size_t r = 0; r < 4; ++r)
            for (std::size_t c = 0; c < 4; ++c)
                inv[r][c] = aug[r][4 + c];
        return inv;
    }

    template<typename T>
    ENGINE_MATH_INLINE matrix<T, 4, 4> translation(const Vector<T, 3> &offset) noexcept {
        auto result = identity_matrix<T, 4>();
        result[0][3] = offset[0];
        result[1][3] = offset[1];
        result[2][3] = offset[2];
        return result;
    }

    template<typename T>
    ENGINE_MATH_INLINE matrix<T, 4, 4> scale(const Vector<T, 3> &factors) noexcept {
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
} // namespace engine::math
