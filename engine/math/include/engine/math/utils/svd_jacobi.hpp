#pragma once

#include "engine/math/matrix.hpp"

namespace engine::math::utils
{
    // Helper: One-sided Jacobi SVD for column-major matrices
    // More stable for the column-major layout
    template <typename T, std::size_t Rows, std::size_t Cols>
    struct SVDResult
    {
        Matrix<T, Rows, Rows> U; // Left singular vectors
        Vector<T, (Rows < Cols ? Rows : Cols)> S; // Singular values (diagonal)
        Matrix<T, Cols, Cols> V; // Right singular vectors
    };

    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE SVDResult<T, Rows, Cols> svd_one_sided_jacobi(const Matrix<T, Rows, Cols>& A,
                                                           std::size_t max_iterations = 100) noexcept
    {
        constexpr std::size_t MinDim = (Rows < Cols) ? Rows : Cols;
        constexpr T epsilon = T(1e-15);
        constexpr T tolerance = T(1e-10);

        SVDResult<T, Rows, Cols> result;

        // Initialize V to identity, U will be computed from A*V
        result.V = identity_matrix<T, Cols>();

        // Working matrix: copy columns of A
        Matrix<T, Rows, Cols> B = A;

        // One-sided Jacobi: orthogonalize columns of B
        for (std::size_t sweep = 0; sweep < max_iterations; ++sweep)
        {
            bool converged = true;

            // Process all column pairs
            for (std::size_t i = 0; i < Cols - 1; ++i)
            {
                for (std::size_t j = i + 1; j < Cols; ++j)
                {
                    // Compute column dot products: a = col_i^T * col_i, b = col_i^T * col_j, c = col_j^T * col_j
                    T a = detail::zero<T>(), b = detail::zero<T>(), c = detail::zero<T>();
                    for (std::size_t k = 0; k < Rows; ++k)
                    {
                        T bi = B.columns[i][k];
                        T bj = B.columns[j][k];
                        a += bi * bi;
                        b += bi * bj;
                        c += bj * bj;
                    }

                    // Check if columns are already orthogonal
                    T off = utils::abs(b) / utils::sqrt(a * c + epsilon);
                    if (off > tolerance)
                    {
                        converged = false;

                        // Compute Jacobi rotation to orthogonalize
                        T zeta = (c - a) / (static_cast<T>(2) * b);
                        T t = (zeta >= detail::zero<T>())
                                  ? (detail::one<T>() / (zeta + utils::sqrt(detail::one<T>() + zeta * zeta)))
                                  : (detail::one<T>() / (zeta - utils::sqrt(detail::one<T>() + zeta * zeta)));

                        T cos_theta = detail::one<T>() / utils::sqrt(detail::one<T>() + t * t);
                        T sin_theta = cos_theta * t;

                        // Apply Givens rotation to columns i and j of B
                        for (std::size_t k = 0; k < Rows; ++k)
                        {
                            T bi = B.columns[i][k];
                            T bj = B.columns[j][k];
                            B.columns[i][k] = cos_theta * bi - sin_theta * bj;
                            B.columns[j][k] = sin_theta * bi + cos_theta * bj;
                        }

                        // Accumulate rotation in V
                        for (std::size_t k = 0; k < Cols; ++k)
                        {
                            T vi = result.V[k][i];
                            T vj = result.V[k][j];
                            result.V[k][i] = cos_theta * vi - sin_theta * vj;
                            result.V[k][j] = sin_theta * vi + cos_theta * vj;
                        }
                    }
                }
            }

            if (converged) break;
        }

        // Extract singular values and compute U
        for (std::size_t i = 0; i < MinDim; ++i)
        {
            // Compute norm of column i
            T norm = detail::zero<T>();
            for (std::size_t k = 0; k < Rows; ++k)
            {
                norm += B.columns[i][k] * B.columns[i][k];
            }
            result.S[i] = utils::sqrt(norm);
        }

        // Initialize U todetail::zero
        result.U = Matrix<T, Rows, Rows>{};

        // Compute U = B * V^T / S (or U columns = B columns normalized)
        for (std::size_t i = 0; i < MinDim; ++i)
        {
            if (result.S[i] > epsilon)
            {
                T inv_s = detail::one<T>() / result.S[i];
                for (std::size_t k = 0; k < Rows; ++k)
                {
                    result.U[k][i] = B.columns[i][k] * inv_s;
                }
            }
            else
            {
                // Handledetail::zero singular value - set to arbitrary unit vector
                result.U[i < Rows ? i : 0][i] = detail::one<T>();
            }
        }

        // Fill remaining columns of U with orthogonal vectors if Rows > MinDim
        if constexpr (Rows > Cols)
        {
            for (std::size_t i = Cols; i < Rows; ++i)
            {
                // Simple Gram-Schmidt to find orthogonal vector
                Vector<T, Rows> candidate{};
                candidate[i] = detail::one<T>();

                // Orthogonalize against existing U columns
                for (std::size_t j = 0; j < i; ++j)
                {
                    T dot_prod = detail::zero<T>();
                    for (std::size_t k = 0; k < Rows; ++k)
                    {
                        dot_prod += candidate[k] * result.U[k][j];
                    }
                    for (std::size_t k = 0; k < Rows; ++k)
                    {
                        candidate[k] -= dot_prod * result.U[k][j];
                    }
                }

                // Normalize
                T norm = detail::zero<T>();
                for (std::size_t k = 0; k < Rows; ++k)
                {
                    norm += candidate[k] * candidate[k];
                }
                T inv_norm = (norm > epsilon) ? (detail::one<T>() / utils::sqrt(norm)) : detail::one<T>();
                for (std::size_t k = 0; k < Rows; ++k)
                {
                    result.U[k][i] = candidate[k] * inv_norm;
                }
            }
        }

        // Sort singular values in descending order
        for (std::size_t i = 0; i < MinDim - 1; ++i)
        {
            std::size_t max_idx = i;
            for (std::size_t j = i + 1; j < MinDim; ++j)
            {
                if (result.S[j] > result.S[max_idx]) max_idx = j;
            }

            if (max_idx != i)
            {
                // Swap singular values
                T tmp = result.S[i];
                result.S[i] = result.S[max_idx];
                result.S[max_idx] = tmp;

                // Swap columns of U
                for (std::size_t k = 0; k < Rows; ++k)
                {
                    T tmp_u = result.U[k][i];
                    result.U[k][i] = result.U[k][max_idx];
                    result.U[k][max_idx] = tmp_u;
                }

                // Swap columns of V
                for (std::size_t k = 0; k < Cols; ++k)
                {
                    T tmp_v = result.V[k][i];
                    result.V[k][i] = result.V[k][max_idx];
                    result.V[k][max_idx] = tmp_v;
                }
            }
        }

        return result;
    }
} // namespace engine::math::utils
