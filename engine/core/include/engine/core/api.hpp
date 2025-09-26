#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_CORE_EXPORTS)
#    define ENGINE_CORE_API __declspec(dllexport)
#  else
#    define ENGINE_CORE_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_CORE_API
#endif

namespace engine::core {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::core

extern "C" ENGINE_CORE_API const char* engine_core_module_name() noexcept;
