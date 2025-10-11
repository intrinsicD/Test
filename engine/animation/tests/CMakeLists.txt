add_executable(engine_animation_tests
    test_module.cpp
    test_clip_serialization.cpp
)

target_link_libraries(engine_animation_tests
    PRIVATE
        engine::project_options
        engine_animation
        GTest::gtest_main
)

add_test(NAME engine_animation_tests COMMAND engine_animation_tests)
