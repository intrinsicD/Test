set(target_name engine_animation_benchmark_driver)

add_executable(${target_name}
        benchmark_driver.cpp
)

target_link_libraries(${target_name}
        PRIVATE
        engine::project_options
        engine_animation
        engine_compute
)

set_target_properties(${target_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/animation"
)

