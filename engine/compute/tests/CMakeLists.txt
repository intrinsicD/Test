add_executable(engine_compute_tests
        test_module.cpp
        test_runtime_dispatch_queue_assignment.cpp
        test_runtime_dispatch_workload_configuration.cpp
        ../samples/runtime_dispatch_demo/workload_configuration.cpp
)

target_link_libraries(engine_compute_tests
        PRIVATE
        engine::project_options
        engine_compute
        engine_runtime
        GTest::gtest_main
)

target_include_directories(engine_compute_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../samples
        ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/include
)

add_test(NAME engine_compute_tests COMMAND engine_compute_tests)
