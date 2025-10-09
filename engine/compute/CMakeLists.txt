set(target_name engine_compute)

add_library(${target_name}
    src/api.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_COMPUTE_EXPORTS
)

add_subdirectory(cuda)

target_link_libraries(${target_name}
    PUBLIC
        engine_math
        engine_compute_cuda
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
