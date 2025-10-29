set(target_name runtime_prototype_harness)

add_executable(${target_name}
        prototype_harness.cpp
)

target_link_libraries(${target_name}
        PRIVATE
        engine::project_options
        engine_runtime
)

set_target_properties(${target_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/runtime"
)
