add_executable(engine_compute_tests
    test_module.cpp
)

set_target_properties(engine_compute_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_compute_tests
    PRIVATE
        engine_compute
        GTest::gtest_main
)

add_test(NAME engine_compute_tests COMMAND engine_compute_tests)
