#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_PLATFORM_EXPORTS)
#    define ENGINE_PLATFORM_API __declspec(dllexport)
#  else
#    define ENGINE_PLATFORM_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_PLATFORM_API
#endif

namespace engine::platform {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::platform

extern "C" ENGINE_PLATFORM_API const char* engine_platform_module_name() noexcept;
