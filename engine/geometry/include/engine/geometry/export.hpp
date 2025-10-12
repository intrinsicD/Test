#pragma once

#if defined(_WIN32)
#  if defined(ENGINE_GEOMETRY_EXPORTS)
#    define ENGINE_GEOMETRY_API __declspec(dllexport)
#  else
#    define ENGINE_GEOMETRY_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_GEOMETRY_API
#endif

