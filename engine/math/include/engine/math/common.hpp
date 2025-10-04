#pragma once

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <limits>

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
} // namespace engine::math
