add_executable(engine_math_tests
    test_math.cpp
)

set_target_properties(engine_math_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_math_tests
    PRIVATE
        engine_math
        GTest::gtest_main
)

add_test(NAME engine_math_tests COMMAND engine_math_tests)
