add_executable(engine_math_tests
        test_math.cpp
        test_conversion_telemetry.cpp
        test_solvers.cpp
)

target_link_libraries(engine_math_tests
        PRIVATE
        engine::project_options
        engine_math
        GTest::gtest_main
)

add_test(NAME engine_math_tests COMMAND engine_math_tests)

add_executable(engine_math_simd_tests
        test_math_simd.cpp
)

target_link_libraries(engine_math_simd_tests
        PRIVATE
        engine::project_options
        engine_math
        GTest::gtest_main
)

add_test(NAME engine_math_simd_tests COMMAND engine_math_simd_tests)
