set(target_name scene_hierarchy_diagnostics_sample)

add_executable(${target_name}
    hierarchy_diagnostics_sample.cpp
)

target_link_libraries(${target_name}
    PRIVATE
        engine::project_options
        engine_scene
)

set_target_properties(${target_name}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/scene"
)
