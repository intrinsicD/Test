add_executable(engine_rendering_frame_graph_benchmark
        frame_graph_cache_benchmark.cpp
)

target_link_libraries(engine_rendering_frame_graph_benchmark
        PRIVATE
        engine::project_options
        engine_rendering
)

add_test(
        NAME rendering_frame_graph_cache_benchmark
        COMMAND engine_rendering_frame_graph_benchmark
        --passes 16
        --iterations 32
        --output "${CMAKE_CURRENT_BINARY_DIR}/rendering_frame_graph_cache_benchmark.json"
)

set_tests_properties(rendering_frame_graph_cache_benchmark
        PROPERTIES
        PASS_REGULAR_EXPRESSION "\\\"benchmark\\\":\\\"rendering_frame_graph_compile_cache\\\""
)
