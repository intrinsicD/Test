add_executable(engine_animation_tests
    test_module.cpp
)

set_target_properties(engine_animation_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_animation_tests
    PRIVATE
        engine_animation
        GTest::gtest_main
)

add_test(NAME engine_animation_tests COMMAND engine_animation_tests)
