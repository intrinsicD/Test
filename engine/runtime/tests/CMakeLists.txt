add_executable(engine_runtime_tests
    test_module.cpp
)

set_target_properties(engine_runtime_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_runtime_tests
    PRIVATE
        engine_runtime
        GTest::gtest_main
)

add_test(NAME engine_runtime_tests COMMAND engine_runtime_tests)
