set(target_name engine_io)

add_library(${target_name}
    src/api.cpp
    src/geometry_io.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_IO_EXPORTS
)

target_link_libraries(${target_name}
    PUBLIC
        engine_geometry
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
