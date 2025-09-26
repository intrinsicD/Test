#pragma once

#include <string_view>

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

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept;
