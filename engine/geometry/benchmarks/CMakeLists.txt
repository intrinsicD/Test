add_executable(engine_geometry_benchmarks
    normal_recompute_benchmark.cpp
)

target_link_libraries(engine_geometry_benchmarks
    PRIVATE
        engine::project_options
        engine_geometry
)

add_test(
    NAME geometry_normals_benchmark
    COMMAND engine_geometry_benchmarks
        --resolution 256
        --iterations 128
        --output "${CMAKE_CURRENT_BINARY_DIR}/geometry_normals_benchmark.json"
)

set_tests_properties(geometry_normals_benchmark
    PROPERTIES
        PASS_REGULAR_EXPRESSION "\"benchmark\":\"geometry_normal_recompute\""
)
