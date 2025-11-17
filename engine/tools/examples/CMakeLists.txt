# Examples demonstrating engine capabilities

# Geometry Viewer - demonstrates research baseline rendering
set(_geometry_viewer_requirements_met TRUE)
set(_geometry_viewer_missing_targets "")

if(NOT TARGET glfw)
    set(_geometry_viewer_requirements_met FALSE)
    list(APPEND _geometry_viewer_missing_targets "glfw")
endif()

if(NOT TARGET glad::gl_core)
    set(_geometry_viewer_requirements_met FALSE)
    list(APPEND _geometry_viewer_missing_targets "glad::gl_core")
endif()

if(_geometry_viewer_requirements_met)
    add_executable(geometry_viewer
            geometry_viewer.cpp
    )

    target_link_libraries(geometry_viewer
            PRIVATE
            engine_core
            engine_runtime
            engine_rendering
            engine_tools
            engine_scene
            engine_math
            engine_assets
            engine_platform
            glfw
            glad::gl_core
    )

    # Install examples to bin directory
    install(TARGETS geometry_viewer
            RUNTIME DESTINATION bin/examples
    )
else()
    list(JOIN _geometry_viewer_missing_targets ", " _geometry_viewer_missing_targets_joined)
    message(STATUS "Skipping geometry_viewer example (missing targets: ${_geometry_viewer_missing_targets_joined}).")
    unset(_geometry_viewer_missing_targets_joined)
endif()

unset(_geometry_viewer_requirements_met)
unset(_geometry_viewer_missing_targets)

# Procedural Cube Example - demonstrates geometry API for creating meshes
add_executable(procedural_cube_example
        procedural_cube_example.cpp
)

target_link_libraries(procedural_cube_example
        PRIVATE
        engine_geometry
        engine_math
)

install(TARGETS procedural_cube_example
        RUNTIME DESTINATION bin/examples
)
