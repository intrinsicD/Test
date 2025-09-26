set(target_name engine_physics)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_PHYSICS_EXPORTS
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
