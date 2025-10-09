add_executable(engine_compute_cuda_tests
    test_module.cpp
)

target_link_libraries(engine_compute_cuda_tests
    PRIVATE
        engine::project_options
        engine_compute_cuda
        GTest::gtest_main
)

add_test(NAME engine_compute_cuda_tests COMMAND engine_compute_cuda_tests)
