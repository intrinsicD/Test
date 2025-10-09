add_executable(engine_math_tests
    test_math.cpp
)

target_link_libraries(engine_math_tests
    PRIVATE
        engine::project_options
        engine_math
        GTest::gtest_main
)

add_test(NAME engine_math_tests COMMAND engine_math_tests)
