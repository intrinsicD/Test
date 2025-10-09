set(target_name engine_rendering)

add_library(${target_name}
    src/api.cpp
    src/frame_graph.cpp
    src/forward_pipeline.cpp
    src/material_system.cpp
    resources/recording_gpu_resource_provider.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_RENDERING_EXPORTS
)

# Rendering depends on core platform layers.
target_link_libraries(${target_name}
    PUBLIC
        engine_core
        engine_assets
        engine_platform
        engine_scene
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
