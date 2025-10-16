set(target_name engine_io)

add_library(${target_name}
    src/api.cpp
    src/animation_importer.cpp
    src/geometry_io_registry.cpp
    src/geometry_io.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_IO_EXPORTS
)

target_link_libraries(${target_name}
    PUBLIC
        engine_core
        engine_geometry
        engine_animation
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
