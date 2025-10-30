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

# Prototype Viewer - interactive 3D model viewer with drag-and-drop
add_executable(prototype_viewer
        prototype_viewer.cpp
)

target_link_libraries(prototype_viewer
        PRIVATE
        engine_rendering
        engine_scene
        engine_math
        engine_assets
        engine_platform
        engine_tools
        glfw
        imgui
        glad_gl_core
)

# Install examples to bin directory
install(TARGETS geometry_viewer prototype_viewer
        RUNTIME DESTINATION bin/examples
)

