#pragma once

#include <string_view>

#if defined(_WIN32)
#  if defined(ENGINE_IO_EXPORTS)
#    define ENGINE_IO_API __declspec(dllexport)
#  else
#    define ENGINE_IO_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_IO_API
#endif

namespace engine::io {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::io

extern "C" ENGINE_IO_API const char* engine_io_module_name() noexcept;
