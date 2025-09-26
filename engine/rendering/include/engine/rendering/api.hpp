#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_RENDERING_EXPORTS)
#    define ENGINE_RENDERING_API __declspec(dllexport)
#  else
#    define ENGINE_RENDERING_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_RENDERING_API
#endif

namespace engine::rendering {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::rendering

extern "C" ENGINE_RENDERING_API const char* engine_rendering_module_name() noexcept;
