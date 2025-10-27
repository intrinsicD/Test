#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_TOOLS_EXPORTS)
#    define ENGINE_TOOLS_API __declspec(dllexport)
#  else
#    define ENGINE_TOOLS_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_TOOLS_API
#endif

namespace engine::tools
{
    [[nodiscard]] ENGINE_TOOLS_API std::string_view module_name() noexcept;
} // namespace engine::tools

extern "C" ENGINE_TOOLS_API const char* engine_tools_module_name() noexcept;