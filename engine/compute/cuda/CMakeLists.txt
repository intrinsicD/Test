set(target_name engine_compute_cuda)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
