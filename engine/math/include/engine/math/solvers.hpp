#pragma once

#include "matrix.hpp"
#include "vector.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <type_traits>

namespace engine::math::solvers
{
    namespace detail
    {
        template <typename T>
        ENGINE_MATH_INLINE T default_linear_tolerance() noexcept
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
            {
                return T(1e-6);
            }
            else if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            {
                return T(1e-12);
            }
            else if constexpr (std::is_same_v<std::remove_cv_t<T>, long double>)
            {
                return T(1e-15);
            }
            else
            {
                return T(0);
            }
        }

        template <typename T>
        ENGINE_MATH_INLINE T default_polynomial_tolerance() noexcept
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
            {
                return T(1e-6);
            }
            else if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            {
                return T(1e-12);
            }
            else if constexpr (std::is_same_v<std::remove_cv_t<T>, long double>)
            {
                return T(1e-15);
            }
            else
            {
                return T(0);
            }
        }

        template <typename T>
        using promoted_t = std::conditional_t<std::is_same_v<std::remove_cv_t<T>, long double>, long double, long
                                              double>;

        template <typename T>
        ENGINE_MATH_INLINE promoted_t<T> abs(promoted_t<T> value) noexcept
        {
            return value < promoted_t<T>(0) ? -value : value;
        }

        template <typename T>
        ENGINE_MATH_INLINE promoted_t<T> clamp(promoted_t<T> value,
                                               promoted_t<T> min_value,
                                               promoted_t<T> max_value) noexcept
        {
            return value < min_value ? min_value : (value > max_value ? max_value : value);
        }
    } // namespace detail

    template <typename T, std::size_t N>
    [[nodiscard]] ENGINE_MATH_INLINE std::optional<Vector<T, N>> try_solve_linear_system(
        const Matrix<T, N, N>& A,
        const Vector<T, N>& b,
        T tolerance = detail::default_linear_tolerance<T>()) noexcept
    {
        using Scalar = detail::promoted_t<T>;

        std::array<std::array<Scalar, N>, N> matrix{};
        std::array<Scalar, N> rhs{};

        for (std::size_t r = 0; r < N; ++r)
        {
            rhs[r] = static_cast<Scalar>(b[r]);
            for (std::size_t c = 0; c < N; ++c)
            {
                matrix[r][c] = static_cast<Scalar>(A[r][c]);
            }
        }

        const Scalar pivot_threshold = static_cast<Scalar>(tolerance);

        for (std::size_t pivot = 0; pivot < N; ++pivot)
        {
            std::size_t best_row = pivot;
            Scalar best_value = detail::abs<T>(matrix[pivot][pivot]);

            for (std::size_t candidate = pivot + 1; candidate < N; ++candidate)
            {
                const Scalar value = detail::abs<T>(matrix[candidate][pivot]);
                if (value > best_value)
                {
                    best_value = value;
                    best_row = candidate;
                }
            }

            if (best_value <= pivot_threshold)
            {
                return std::nullopt;
            }

            if (best_row != pivot)
            {
                std::swap(matrix[pivot], matrix[best_row]);
                std::swap(rhs[pivot], rhs[best_row]);
            }

            const Scalar pivot_inv = Scalar(1) / matrix[pivot][pivot];
            for (std::size_t c = pivot; c < N; ++c)
            {
                matrix[pivot][c] *= pivot_inv;
            }
            rhs[pivot] *= pivot_inv;

            for (std::size_t row = 0; row < N; ++row)
            {
                if (row == pivot) continue;
                const Scalar factor = matrix[row][pivot];
                if (detail::abs<T>(factor) <= Scalar(0))
                {
                    continue;
                }

                for (std::size_t c = pivot; c < N; ++c)
                {
                    matrix[row][c] -= factor * matrix[pivot][c];
                }
                rhs[row] -= factor * rhs[pivot];
            }
        }

        Vector<T, N> solution{};
        for (std::size_t i = 0; i < N; ++i)
        {
            solution[i] = static_cast<T>(rhs[i]);
        }
        return solution;
    }

    template <typename T>
    [[nodiscard]] ENGINE_MATH_INLINE std::size_t solve_quadratic(
        T a,
        T b,
        T c,
        std::array<T, 2>& roots,
        T tolerance = detail::default_polynomial_tolerance<T>()) noexcept
    {
        using Scalar = detail::promoted_t<T>;

        const Scalar da = static_cast<Scalar>(a);
        const Scalar db = static_cast<Scalar>(b);
        const Scalar dc = static_cast<Scalar>(c);
        const Scalar eps = static_cast<Scalar>(tolerance);

        if (detail::abs<T>(da) <= eps)
        {
            if (detail::abs<T>(db) <= eps)
            {
                return 0;
            }

            roots[0] = static_cast<T>(-dc / db);
            return 1;
        }

        const Scalar discriminant = db * db - Scalar(4) * da * dc;

        if (discriminant < -eps)
        {
            return 0;
        }

        if (detail::abs<T>(discriminant) <= eps)
        {
            const Scalar root = -db / (Scalar(2) * da);
            roots[0] = static_cast<T>(root);
            return 1;
        }

        const Scalar sqrt_discriminant = std::sqrt(discriminant);
        const Scalar sign_b = db >= Scalar(0) ? Scalar(1) : Scalar(-1);
        const Scalar q = -Scalar(0.5) * (db + sign_b * sqrt_discriminant);

        Scalar root0 = q / da;
        Scalar root1 = dc / q;

        if (root0 > root1)
        {
            std::swap(root0, root1);
        }

        roots[0] = static_cast<T>(root0);
        roots[1] = static_cast<T>(root1);
        return 2;
    }

    template <typename T>
    [[nodiscard]] ENGINE_MATH_INLINE std::size_t solve_cubic(
        T a,
        T b,
        T c,
        T d,
        std::array<T, 3>& roots,
        T tolerance = detail::default_polynomial_tolerance<T>()) noexcept
    {
        using Scalar = detail::promoted_t<T>;

        const Scalar da = static_cast<Scalar>(a);
        const Scalar db = static_cast<Scalar>(b);
        const Scalar dc = static_cast<Scalar>(c);
        const Scalar dd = static_cast<Scalar>(d);
        const Scalar eps = static_cast<Scalar>(tolerance);

        if (detail::abs<T>(da) <= eps)
        {
            std::array<T, 2> quadratic_roots{};
            const std::size_t count = solve_quadratic(static_cast<T>(b), static_cast<T>(c), static_cast<T>(d),
                                                      quadratic_roots, tolerance);
            for (std::size_t i = 0; i < count; ++i)
            {
                roots[i] = quadratic_roots[i];
            }
            return count;
        }

        const Scalar inv_a = Scalar(1) / da;
        const Scalar bb = db * inv_a;
        const Scalar cc = dc * inv_a;
        const Scalar dd_norm = dd * inv_a;

        const Scalar third = Scalar(1) / Scalar(3);
        const Scalar p = cc - bb * bb * third;
        const Scalar q = Scalar(2) * bb * bb * bb / Scalar(27) - bb * cc / Scalar(3) + dd_norm;

        const Scalar half_q = q / Scalar(2);
        const Scalar p_third = p / Scalar(3);
        const Scalar discriminant = half_q * half_q + p_third * p_third * p_third;

        const Scalar shift = bb * third;

        if (discriminant > eps)
        {
            const Scalar sqrt_disc = std::sqrt(discriminant);
            const Scalar u = std::cbrt(-half_q + sqrt_disc);
            const Scalar v = std::cbrt(-half_q - sqrt_disc);
            const Scalar root = u + v - shift;
            roots[0] = static_cast<T>(root);
            return 1;
        }

        if (detail::abs<T>(discriminant) <= eps)
        {
            const Scalar u = std::cbrt(-half_q);
            const Scalar root1 = Scalar(2) * u - shift;
            const Scalar root2 = -u - shift;

            roots[0] = static_cast<T>(root1);
            roots[1] = static_cast<T>(root2);
            if (detail::abs<T>(root1 - root2) <= eps)
            {
                return 1;
            }
            return 2;
        }

        const Scalar cos_argument = detail::clamp<T>(-half_q / std::sqrt(-(p_third * p_third * p_third)), Scalar(-1),
                                                     Scalar(1));
        const Scalar phi = std::acos(cos_argument);
        const Scalar two_sqrt = Scalar(2) * std::sqrt(-p_third);

        std::array<Scalar, 3> cubic_roots{
            two_sqrt * std::cos(phi * third) - shift,
            two_sqrt * std::cos((phi + Scalar(2) * std::numbers::pi_v<Scalar>) * third) - shift,
            two_sqrt * std::cos((phi + Scalar(4) * std::numbers::pi_v<Scalar>) * third) - shift
        };

        std::sort(cubic_roots.begin(), cubic_roots.end());
        for (std::size_t i = 0; i < 3; ++i)
        {
            roots[i] = static_cast<T>(cubic_roots[i]);
        }
        return 3;
    }
} // namespace engine::math::solvers