add_executable(engine_platform_tests
    test_module.cpp
)

set_target_properties(engine_platform_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_platform_tests
    PRIVATE
        engine_platform
        GTest::gtest_main
)

add_test(NAME engine_platform_tests COMMAND engine_platform_tests)
