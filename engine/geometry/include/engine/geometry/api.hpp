#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_GEOMETRY_EXPORTS)
#    define ENGINE_GEOMETRY_API __declspec(dllexport)
#  else
#    define ENGINE_GEOMETRY_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_GEOMETRY_API
#endif

namespace engine::geometry {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::geometry

extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept;
