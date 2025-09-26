#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_PHYSICS_EXPORTS)
#    define ENGINE_PHYSICS_API __declspec(dllexport)
#  else
#    define ENGINE_PHYSICS_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_PHYSICS_API
#endif

namespace engine::physics {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::physics

extern "C" ENGINE_PHYSICS_API const char* engine_physics_module_name() noexcept;
