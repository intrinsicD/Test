set(target_name engine_compute)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(cuda)

target_link_libraries(${target_name}
    PUBLIC
        engine_compute_cuda
)
