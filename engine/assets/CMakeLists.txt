set(target_name engine_assets)

add_library(${target_name}
    src/api.cpp
    src/graph_asset.cpp
    src/material_asset.cpp
    src/mesh_asset.cpp
    src/point_cloud_asset.cpp
    src/shader_asset.cpp
    src/texture_asset.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_ASSETS_EXPORTS
)

target_link_libraries(${target_name}
    PUBLIC
        engine_io
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
