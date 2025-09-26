set(target_name engine_runtime)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${target_name}
    PUBLIC
        engine_animation
        engine_assets
        engine_compute
        engine_compute_cuda
        engine_core
        engine_geometry
        engine_io
        engine_math
        engine_physics
        engine_platform
        engine_rendering
        engine_scene
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_RUNTIME_EXPORTS
)
