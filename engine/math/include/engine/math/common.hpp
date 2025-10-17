#pragma once

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <limits>
#include <numbers>

#ifndef ENGINE_MATH_HD
#    if defined(__CUDACC__)
#        define ENGINE_MATH_HD __host__ __device__
#    else
#        define ENGINE_MATH_HD
#    endif
#endif

#ifndef ENGINE_MATH_INLINE
#    define ENGINE_MATH_INLINE constexpr ENGINE_MATH_HD inline
#endif

namespace engine::math {
    namespace detail {
        template<typename T>
        ENGINE_MATH_INLINE T zero() noexcept {
            return T(0);
        }

        template<typename T>
        ENGINE_MATH_INLINE T one() noexcept {
            return T(1);
        }

        template<typename T>
        ENGINE_MATH_INLINE T infinity() noexcept {
            return T(std::numeric_limits<T>::infinity());
        }
    } // namespace detail

    template<typename T>
    ENGINE_MATH_INLINE auto radians(T degrees) noexcept
    {
        using value_type = std::remove_cvref_t<T>;
        using return_type = std::conditional_t<std::is_floating_point_v<value_type>, value_type, double>;

        return static_cast<return_type>(degrees) *
               (std::numbers::pi_v<return_type> / static_cast<return_type>(180));
    }
} // namespace engine::math
