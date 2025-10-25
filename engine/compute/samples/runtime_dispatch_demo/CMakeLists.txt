set(target_name engine_compute_runtime_sample)

add_executable(${target_name}
    runtime_dispatch_demo.cpp
    workload_configuration.cpp
)

target_link_libraries(${target_name}
    PRIVATE
        engine::project_options
        engine_runtime
)

set_target_properties(${target_name}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/compute"
)
