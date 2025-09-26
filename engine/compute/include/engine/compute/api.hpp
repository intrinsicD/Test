#pragma once

#include <string_view>

#include "engine/math/math.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_COMPUTE_EXPORTS)
#    define ENGINE_COMPUTE_API __declspec(dllexport)
#  else
#    define ENGINE_COMPUTE_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_COMPUTE_API
#endif

namespace engine::compute {

[[nodiscard]] std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_COMPUTE_API math::mat4 identity_transform() noexcept;

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept;
