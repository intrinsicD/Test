set(target_name engine_compute_cuda)

add_library(${target_name}
    src/api.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_COMPUTE_CUDA_EXPORTS
)

target_link_libraries(${target_name}
    PUBLIC
        engine_math
)
