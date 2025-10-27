# Examples demonstrating engine capabilities

# Geometry Viewer - demonstrates research baseline rendering
add_executable(geometry_viewer
    geometry_viewer.cpp
)

target_link_libraries(geometry_viewer
    PRIVATE
        engine_rendering
        engine_scene
        engine_math
        engine_assets
)

# Install examples to bin directory
install(TARGETS geometry_viewer
    RUNTIME DESTINATION bin/examples
)

