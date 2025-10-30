add_library(engine_geometry_remesh_cli
        remesh_cli.cpp
)

target_include_directories(engine_geometry_remesh_cli
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_geometry_remesh_cli
        PUBLIC
        engine::project_options
        engine_geometry
)

add_executable(geometry_remesh
        geometry_remesh.cpp
)

target_link_libraries(geometry_remesh
        PRIVATE
        engine::project_options
        engine_geometry_remesh_cli
        engine_geometry_io
)

set_target_properties(geometry_remesh
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/geometry"
)
