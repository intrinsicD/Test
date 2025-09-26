#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_SCENE_EXPORTS)
#    define ENGINE_SCENE_API __declspec(dllexport)
#  else
#    define ENGINE_SCENE_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_SCENE_API
#endif

namespace engine::scene {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::scene

extern "C" ENGINE_SCENE_API const char* engine_scene_module_name() noexcept;
