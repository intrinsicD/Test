#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_ANIMATION_EXPORTS)
#    define ENGINE_ANIMATION_API __declspec(dllexport)
#  else
#    define ENGINE_ANIMATION_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_ANIMATION_API
#endif

namespace engine::animation {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::animation

extern "C" ENGINE_ANIMATION_API const char* engine_animation_module_name() noexcept;
