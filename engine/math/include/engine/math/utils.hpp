#pragma once

#include "engine/math/common.hpp"

namespace engine::math::utils
{
    template <typename T>
    ENGINE_MATH_INLINE T clamp(T v, T lo, T hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

    template <typename T>
    ENGINE_MATH_INLINE T abs(T v) noexcept { return v < T(0) ? -v : v; }

    template <typename T>
    ENGINE_MATH_INLINE T max(T a, T b) noexcept { return a < b ? b : a; }

    template <typename T>
    ENGINE_MATH_INLINE T max(T a, T b, T c) noexcept { return max(max(a, b), c); }

    template <typename T>
    ENGINE_MATH_INLINE T min(T a, T b) noexcept { return a < b ? a : b; }

    template <typename T>
    ENGINE_MATH_INLINE T min(T a, T b, T c) noexcept { return min(min(a, b), c); }

    template <class T>
    ENGINE_MATH_INLINE T sqrt(T x) { return static_cast<T>(::sqrt(static_cast<double>(x))); }

    template <typename T>
    ENGINE_MATH_INLINE bool nearly_equal(T a, T b, T epsilon = T(1e-6)) { return utils::abs(a - b) <= epsilon; }
} // namespace engine::math::utils
