#pragma once

#include "engine/math/vector.hpp"

#include <optional>

namespace engine::math
{
    template <typename T, std::size_t Rows, std::size_t Cols>
    struct Matrix
    {
        //TODO: Change this to a graphics friendly layout (column-major). Make sure to adapt all code neccessary as well as the tests.
        using value_type = T;
        using row_type = Vector<T, Cols>;
        using size_type = std::size_t;

        row_type rows[Rows];

        ENGINE_MATH_INLINE Matrix() noexcept : rows{}
        {
        }

        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Rows * Cols>>
        ENGINE_MATH_INLINE Matrix(Args... args) noexcept : rows{}
        {
            const value_type values[] = {static_cast<value_type>(args)...};
            size_type index = 0U;
            for (size_type r = 0; r < Rows; ++r)
            {
                for (size_type c = 0; c < Cols; ++c)
                {
                    rows[r][c] = values[index++];
                }
            }
        }

        ENGINE_MATH_INLINE Matrix(const Matrix<T, Rows + 1, Cols + 1>& other) noexcept : rows{}
        {
            for (size_type r = 0; r < Rows; ++r)
            {
                for (size_type c = 0; c < Cols; ++c)
                {
                    rows[r][c] = other[r][c];
                }
            }
        }

        ENGINE_MATH_INLINE Matrix& operator=(const Matrix<T, Rows + 1, Cols + 1>& other) noexcept
        {
            for (size_type r = 0; r < Rows; ++r)
            {
                for (size_type c = 0; c < Cols; ++c)
                {
                    rows[r][c] = other[r][c];
                }
            }
            return *this;
        }

        ENGINE_MATH_INLINE row_type& operator[](size_type row) noexcept { return rows[row]; }
        ENGINE_MATH_INLINE const row_type& operator[](size_type row) const noexcept { return rows[row]; }

        ENGINE_MATH_INLINE Matrix& operator+=(const Matrix& rhs) noexcept
        {
            for (size_type r = 0; r < Rows; ++r)
            {
                rows[r] += rhs.rows[r];
            }
            return *this;
        }

        ENGINE_MATH_INLINE Matrix& operator-=(const Matrix& rhs) noexcept
        {
            for (size_type r = 0; r < Rows; ++r)
            {
                rows[r] -= rhs.rows[r];
            }
            return *this;
        }

        ENGINE_MATH_INLINE Matrix& operator*=(value_type scalar) noexcept
        {
            for (size_type r = 0; r < Rows; ++r)
            {
                rows[r] *= scalar;
            }
            return *this;
        }
    };

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Rows, Cols> operator+(Matrix<T, Rows, Cols> lhs,
                                                       const Matrix<T, Rows, Cols>& rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Rows, Cols> operator-(Matrix<T, Rows, Cols> lhs,
                                                       const Matrix<T, Rows, Cols>& rhs) noexcept
    {
        lhs -= rhs;
        return lhs;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Rows, Cols> operator*(Matrix<T, Rows, Cols> lhs, T scalar) noexcept
    {
        lhs *= scalar;
        return lhs;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Rows, Cols> operator*(T scalar, Matrix<T, Rows, Cols> rhs) noexcept
    {
        rhs *= scalar;
        return rhs;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Vector<T, Rows> operator*(const Matrix<T, Rows, Cols>& lhs, const Vector<T, Cols>& rhs) noexcept
    {
        Vector<T, Rows> result{};
        for (std::size_t r = 0; r < Rows; ++r)
        {
            result[r] = dot(lhs[r], rhs);
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Vector<T, Rows> operator*(const Matrix<T, Rows, Cols>& lhs,
                                                 const Vector<T, Cols - 1>& rhs) noexcept
    {
        Vector<T, Cols> tmp = rhs;
        return operator*(lhs, tmp);
    }

    template <typename T, std::size_t Rows, std::size_t Shared, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Rows, Cols> operator*(const Matrix<T, Rows, Shared>& lhs,
                                                       const Matrix<T, Shared, Cols>& rhs) noexcept
    {
        Matrix<T, Rows, Cols> result{};
        for (std::size_t r = 0; r < Rows; ++r)
        {
            for (std::size_t c = 0; c < Cols; ++c)
            {
                T value = detail::zero<T>();
                for (std::size_t k = 0; k < Shared; ++k)
                {
                    value += lhs[r][k] * rhs[k][c];
                }
                result[r][c] = value;
            }
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Cols, Rows> transpose(const Matrix<T, Rows, Cols>& m) noexcept
    {
        Matrix<T, Cols, Rows> result{};
        for (std::size_t r = 0; r < Rows; ++r)
        {
            for (std::size_t c = 0; c < Cols; ++c)
            {
                result[c][r] = m[r][c];
            }
        }
        return result;
    }

    template <typename T, std::size_t Dim>
    ENGINE_MATH_INLINE Matrix<T, Dim, Dim> identity_matrix() noexcept
    {
        Matrix<T, Dim, Dim> result{};
        for (std::size_t i = 0; i < Dim; ++i)
        {
            result[i][i] = detail::one<T>();
        }
        return result;
    }


    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> translation(const Vector<T, 3>& offset) noexcept
    {
        auto result = identity_matrix<T, 4>();
        result[0][3] = offset[0];
        result[1][3] = offset[1];
        result[2][3] = offset[2];
        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 4, 4> scale(const Vector<T, 3>& factors) noexcept
    {
        Matrix<T, 4, 4> result{};
        result[0][0] = factors[0];
        result[1][1] = factors[1];
        result[2][2] = factors[2];
        result[3][3] = detail::one<T>();
        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE T determinant(const Matrix<T, 2, 2>& m) noexcept
    {
        return m[0][0] * m[1][1] - m[0][1] * m[1][0];
    }

    template <typename T>
    ENGINE_MATH_INLINE T determinant(const Matrix<T, 3, 3>& m) noexcept
    {
        return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
            - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
            + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    }

    template <typename T>
    ENGINE_MATH_INLINE T determinant(const Matrix<T, 4, 4>& m) noexcept
    {
        const T a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3];
        const T a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3];
        const T a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3];
        const T a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3];

        // Precompute reused 2x2 determinants (minors of the lower-right 2x2 blocks)
        const T sub00 = a22 * a33 - a23 * a32;
        const T sub01 = a21 * a33 - a23 * a31;
        const T sub02 = a21 * a32 - a22 * a31;
        const T sub03 = a20 * a33 - a23 * a30;
        const T sub04 = a20 * a32 - a22 * a30;
        const T sub05 = a20 * a31 - a21 * a30;

        return
            a00 * (a11 * sub00 - a12 * sub01 + a13 * sub02)
            - a01 * (a10 * sub00 - a12 * sub03 + a13 * sub04)
            + a02 * (a10 * sub01 - a11 * sub03 + a13 * sub05)
            - a03 * (a10 * sub02 - a11 * sub04 + a12 * sub05);
    }


    template <typename T, std::size_t N>
    ENGINE_MATH_INLINE bool is_invertible(const Matrix<T, N, N>& m) noexcept
    {
        // A matrix is invertible if its determinant is non-zero.
        T det = determinant<T>(m);
        return det != detail::zero<T>();
    }

    template <typename T>
    ENGINE_MATH_INLINE std::optional<Matrix<T, 2, 2>> try_inverse(const Matrix<T, 2, 2>& m) noexcept
    {
        const T det = determinant(m);
        if (det == detail::zero<T>()) { return std::nullopt; }
        const T inv = detail::one<T>() / det;
        Matrix<T, 2, 2> r{};
        r[0][0] = m[1][1] * inv;
        r[0][1] = -m[0][1] * inv;
        r[1][0] = -m[1][0] * inv;
        r[1][1] = m[0][0] * inv;
        return r;
    }

    template <typename T>
    ENGINE_MATH_INLINE std::optional<Matrix<T, 3, 3>> try_inverse(const Matrix<T, 3, 3>& m) noexcept
    {
        const T a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
        const T a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
        const T a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];

        const T c00 = a11 * a22 - a12 * a21;
        const T c01 = -(a10 * a22 - a12 * a20);
        const T c02 = a10 * a21 - a11 * a20;

        const T c10 = -(a01 * a22 - a02 * a21);
        const T c11 = a00 * a22 - a02 * a20;
        const T c12 = -(a00 * a21 - a01 * a20);

        const T c20 = a01 * a12 - a02 * a11;
        const T c21 = -(a00 * a12 - a02 * a10);
        const T c22 = a00 * a11 - a01 * a10;

        const T det = a00 * c00 + a01 * c01 + a02 * c02;
        Matrix<T, 3, 3> r{};
        if (det == detail::zero<T>()) { return std::nullopt; }
        const T inv = detail::one<T>() / det;

        // adjugate^T / det
        r[0][0] = c00 * inv;
        r[0][1] = c10 * inv;
        r[0][2] = c20 * inv;
        r[1][0] = c01 * inv;
        r[1][1] = c11 * inv;
        r[1][2] = c21 * inv;
        r[2][0] = c02 * inv;
        r[2][1] = c12 * inv;
        r[2][2] = c22 * inv;
        return r;
    }

    template <typename T>
    ENGINE_MATH_INLINE std::optional<Matrix<T, 4, 4>> try_inverse(const Matrix<T, 4, 4>& m) noexcept
    {
        // This implementation uses Gaussian elimination with partial pivoting.
        // It favours robustness over absolute performance and works for any invertible matrix.
        // Build augmented [M | I]
        T aug[4][8]{};
        for (std::size_t r = 0; r < 4; ++r)
        {
            for (std::size_t c = 0; c < 4; ++c) aug[r][c] = m[r][c];
            for (std::size_t c = 0; c < 4; ++c) aug[r][4 + c] = (r == c) ? detail::one<T>() : detail::zero<T>();
        }

        // Forward elimination
        for (std::size_t col = 0; col < 4; ++col)
        {
            // pivot row
            std::size_t piv = col;
            T maxA = (aug[piv][col] < T(0) ? -aug[piv][col] : aug[piv][col]);
            for (std::size_t r = col + 1; r < 4; ++r)
            {
                T v = aug[r][col];
                v = (v < T(0) ? -v : v);
                if (v > maxA)
                {
                    maxA = v;
                    piv = r;
                }
            }
            if (maxA == detail::zero<T>()) { return std::nullopt; } // singular

            // swap rows
            if (piv != col)
            {
                for (std::size_t c = 0; c < 8; ++c)
                {
                    const T tmp = aug[piv][c];
                    aug[piv][c] = aug[col][c];
                    aug[col][c] = tmp;
                }
            }

            const T pivot = aug[col][col];
            const T invp = detail::one<T>() / pivot;

            // normalize pivot row
            for (std::size_t c = 0; c < 8; ++c) aug[col][c] *= invp;

            // eliminate other rows
            for (std::size_t r = 0; r < 4; ++r)
            {
                if (r == col) continue;
                const T f = aug[r][col];
                if (f == detail::zero<T>()) continue;
                for (std::size_t c = 0; c < 8; ++c) aug[r][c] -= f * aug[col][c];
            }
        }

        // Extract inverse
        Matrix<T, 4, 4> inv{};
        for (std::size_t r = 0; r < 4; ++r)
            for (std::size_t c = 0; c < 4; ++c)
                inv[r][c] = aug[r][4 + c];
        return inv;
    }

    template <typename S, typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<S, Rows, Cols> cast(const Matrix<T, Rows, Cols>& m) noexcept
    {
        Matrix<S, Rows, Cols> result;
        for (std::size_t r = 0; r < Rows; ++r)
        {
            result[r] = cast<T, Cols, S>(m[r]);
        }
        return result;
    }

    using mat2 = Matrix<float, 2, 2>;
    using mat3 = Matrix<float, 3, 3>;
    using mat4 = Matrix<float, 4, 4>;

    using dmat3 = Matrix<double, 3, 3>;
    using dmat4 = Matrix<double, 4, 4>;
} // namespace engine::math
