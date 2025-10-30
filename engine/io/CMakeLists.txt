set(target_name engine_io)

add_library(${target_name} STATIC
        src/api.cpp
        src/animation_importer.cpp
        src/geometry_io_registry.cpp
        src/geometry_io.cpp
        src/telemetry.cpp
)

set_target_properties(${target_name}
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON)

set(geometry_signature_db "${CMAKE_CURRENT_SOURCE_DIR}/signatures/geometry_signatures.json")
cmake_path(CONVERT "${geometry_signature_db}" TO_CMAKE_PATH_LIST geometry_signature_db_normalized)

engine_apply_module_defaults(${target_name}
        PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Ensure geometry headers are visible to this target without creating a link dependency
# to avoid a circular dependency between engine_io and engine_geometry.
target_include_directories(${target_name}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../geometry/include
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

# Do not link engine_geometry here to avoid circular dependency with engine_geometry linking engine_io.
# Consumers (modules/tests) should link both engine_io and engine_geometry.

target_link_libraries(${target_name}
        PUBLIC
        engine_core
        engine_animation
        # engine_geometry intentionally not linked to avoid cycle; final executables/tests must link engine_geometry
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
