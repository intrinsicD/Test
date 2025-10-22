#pragma once

#include "svd_jacobi.hpp"
#include "engine/math/utils/utils.hpp"

namespace engine::math::utils
{
    // SVD-based pseudoinverse (for rank-deficient matrices)
    template <typename T, std::size_t Rows, std::size_t Cols>
    ENGINE_MATH_INLINE Matrix<T, Cols, Rows> pseudoinverse_svd(
        const Matrix<T, Rows, Cols>& A,
        T tolerance = T(1e-10)) noexcept
    {
        constexpr std::size_t MinDim = (Rows < Cols) ? Rows : Cols;

        const auto svd = svd_one_sided_jacobi<T, Rows, Cols>(A);

        Matrix<T, Cols, Rows> sigma_plus{};

        T max_sigma = detail::zero<T>();
        for (std::size_t i = 0; i < MinDim; ++i)
        {
            max_sigma = utils::max(max_sigma, svd.S[i]);
        }

        if (max_sigma == detail::zero<T>())
        {
            return sigma_plus;
        }

        const T cutoff = tolerance * static_cast<T>((Rows > Cols) ? Rows : Cols) * max_sigma;

        for (std::size_t i = 0; i < MinDim; ++i)
        {
            const T sigma = svd.S[i];
            if (sigma > cutoff)
            {
                sigma_plus[i][i] = detail::one<T>() / sigma;
            }
        }

        return svd.V * sigma_plus * transpose(svd.U);
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
