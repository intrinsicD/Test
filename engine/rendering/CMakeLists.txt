set(target_name engine_rendering)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_RENDERING_EXPORTS
)

# Rendering depends on core platform layers.
target_link_libraries(${target_name}
    PUBLIC
        engine_core
        engine_platform
)
