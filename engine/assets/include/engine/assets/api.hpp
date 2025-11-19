#pragma once

#include <string_view>

#include "graph_asset.hpp"
#include "material_asset.hpp"
#include "mesh_asset.hpp"
#include "shader_asset.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_ASSETS_EXPORTS)
#    define ENGINE_ASSETS_API __declspec(dllexport)
#  else
#    define ENGINE_ASSETS_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_ASSETS_API
#endif

namespace engine::assets
{
    [[nodiscard]] std::string_view module_name() noexcept;
} // namespace engine::assets

extern "C" ENGINE_ASSETS_API const char* engine_assets_module_name() noexcept;