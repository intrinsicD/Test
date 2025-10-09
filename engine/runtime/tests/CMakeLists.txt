add_executable(engine_runtime_tests
    test_module.cpp
)

target_link_libraries(engine_runtime_tests
    PRIVATE
        engine::project_options
        engine_runtime
        GTest::gtest_main
)

add_test(NAME engine_runtime_tests COMMAND engine_runtime_tests)
