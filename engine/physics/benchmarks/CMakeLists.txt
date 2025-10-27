add_executable(engine_physics_benchmarks
        collision_benchmark.cpp
)

target_link_libraries(engine_physics_benchmarks
        PRIVATE
        engine::project_options
        engine_physics
)

add_test(
        NAME physics_collision_benchmark
        COMMAND engine_physics_benchmarks
        --bodies 128
        --steps 512
        --dt 0.0166667
        --seed 1337
        --output "${CMAKE_CURRENT_BINARY_DIR}/physics_collision_benchmark.json"
)

set_tests_properties(physics_collision_benchmark
        PROPERTIES
        PASS_REGULAR_EXPRESSION "\"benchmark\":\"physics_collision_throughput\""
)
