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

target_link_libraries(${target_name}
    PUBLIC
        engine_math
)

if(ENGINE_ENABLE_CUDA)
    add_subdirectory(cuda)
endif()

if(TARGET engine_compute_cuda AND ENGINE_ENABLE_COMPUTE_CUDA)
    target_link_libraries(${target_name}
        PUBLIC
            engine_compute_cuda
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
