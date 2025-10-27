set(target_name engine_io)

add_library(${target_name}
        src/api.cpp
        src/animation_importer.cpp
        src/geometry_io_registry.cpp
        src/geometry_io.cpp
        src/telemetry.cpp
)

set(geometry_signature_db "${CMAKE_CURRENT_SOURCE_DIR}/signatures/geometry_signatures.json")
cmake_path(CONVERT "${geometry_signature_db}" TO_CMAKE_PATH_LIST geometry_signature_db_normalized)

engine_apply_module_defaults(${target_name}
        PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
        PRIVATE
        ENGINE_IO_EXPORTS
        ENGINE_IO_GEOMETRY_SIGNATURE_DB_PATH="${geometry_signature_db_normalized}"
)

if (BUILD_TESTING)
    target_compile_definitions(${target_name}
            PRIVATE
            ENGINE_IO_ENABLE_SIGNATURE_TEST_HOOKS
    )
endif ()

target_link_libraries(${target_name}
        PUBLIC
        engine_core
        engine_geometry
        engine_animation
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
