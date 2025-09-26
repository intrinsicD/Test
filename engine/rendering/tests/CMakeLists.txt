add_executable(engine_rendering_tests
    test_module.cpp
)

set_target_properties(engine_rendering_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_rendering_tests
    PRIVATE
        engine_rendering
        GTest::gtest_main
)

add_test(NAME engine_rendering_tests COMMAND engine_rendering_tests)
