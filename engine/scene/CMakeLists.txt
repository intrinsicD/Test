set(target_name engine_scene)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_SCENE_EXPORTS
)

# Scene graph relies on core systems.
target_link_libraries(${target_name}
    PUBLIC
        engine_core
)
