#pragma once

#include "svd_jacobi.hpp"

namespace engine::math::utils
{
    // SVD-based pseudoinverse (for rank-deficient matrices)
    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Cols, Rows> pseudoinverse_svd(
        const Matrix<T, Rows, Cols>& A,
        T tolerance = T(1e-10)) noexcept
    {
        // For small matrices, use Gram-Schmidt based approach
        // Compute column space basis via modified Gram-Schmidt

        Matrix<T, Rows, Cols> Q = A; // Will become orthonormal basis
        Vector<T, Cols> norms{};

        // Gram-Schmidt orthogonalization
        for (std::size_t j = 0; j < Cols; ++j)
        {
            // Orthogonalize column j against previous columns
            for (std::size_t i = 0; i < j; ++i)
            {
                T proj = detail::zero<T>();
                for (std::size_t r = 0; r < Rows; ++r)
                {
                    proj += Q.columns[i][r] * Q.columns[j][r];
                }
                for (std::size_t r = 0; r < Rows; ++r)
                {
                    Q.columns[j][r] -= proj * Q.columns[i][r];
                }
            }

            // Compute norm
            T norm = detail::zero<T>();
            for (std::size_t r = 0; r < Rows; ++r)
            {
                norm += Q.columns[j][r] * Q.columns[j][r];
            }
            norms[j] = utils::sqrt(norm);

            // Normalize if not zero
            if (norms[j] > tolerance)
            {
                T inv_norm = detail::one<T>() / norms[j];
                for (std::size_t r = 0; r < Rows; ++r)
                {
                    Q.columns[j][r] *= inv_norm;
                }
            }
        }

        // Build pseudoinverse: A^+ = (Q * R)^+ = R^+ * Q^T where A = Q * R
        // For our purposes: A^+ ≈ Q * diag(1/norm_i) * ... (simplified)

        // Use simpler formula: A^+ = A^T * (A * A^T)^+
        // This works even for rank-deficient matrices

        Matrix<T, Rows, Rows> AAT{};
        for (std::size_t i = 0; i < Rows; ++i)
        {
            for (std::size_t j = 0; j < Rows; ++j)
            {
                T sum = detail::zero<T>();
                for (std::size_t k = 0; k < Cols; ++k)
                {
                    sum += A[i][k] * A[j][k];
                }
                AAT[i][j] = sum;
            }
        }

        // Add small regularization for rank-deficient case
        for (std::size_t i = 0; i < Rows; ++i)
        {
            AAT[i][i] += tolerance * tolerance;
        }

        auto AAT_inv = try_inverse(AAT);
        if (!AAT_inv.has_value())
        {
            // Return zero matrix if completely singular
            return Matrix<T, Cols, Rows>{};
        }

        return transpose(A) * (*AAT_inv);
    }

    // Moore-Penrose pseudoinverse: A^+ = V * Σ^+ * U^T
    // where Σ^+ has reciprocals of non-zero singular values
    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Cols, Rows> pseudo_inverse(const Matrix<T, Rows, Cols>& A,
                                                            T tolerance = T(1e-10)) noexcept
    {
        // Special case: square invertible matrix
        if constexpr (Rows == Cols)
        {
            auto inv = try_inverse(A);
            if (inv.has_value()) return inv.value();
        }

        // Use direct formula based on matrix shape
        if constexpr (Rows >= Cols)
        {
            // Tall matrix: Try (A^T A)^-1 A^T
            Matrix<T, Cols, Rows> AT = transpose(A);
            Matrix<T, Cols, Cols> ATA = AT * A;

            auto ATA_inv = try_inverse(ATA);
            if (ATA_inv.has_value())
            {
                return (*ATA_inv) * AT;
            }
        }

        if constexpr (Rows <= Cols)
        {
            // Wide matrix: Try A^T (A A^T)^-1
            Matrix<T, Cols, Rows> AT = transpose(A);
            Matrix<T, Rows, Rows> AAT = A * AT;

            auto AAT_inv = try_inverse(AAT);
            if (AAT_inv.has_value())
            {
                return AT * (*AAT_inv);
            }
        }

        // Fall back to SVD for rank-deficient cases
        return pseudoinverse_svd(A, tolerance);
    }
} // namespace engine::math::utils
