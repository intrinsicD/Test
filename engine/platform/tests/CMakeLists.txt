add_executable(engine_platform_tests
    test_module.cpp
    window_console_tests.cpp
    input_state_tests.cpp
)

target_link_libraries(engine_platform_tests
    PRIVATE
        engine::project_options
        engine_platform
        GTest::gtest_main
)

add_test(NAME engine_platform_tests COMMAND engine_platform_tests)

add_executable(engine_platform_window_app
    window_test_app.cpp
)

target_link_libraries(engine_platform_window_app
    PRIVATE
        engine::project_options
        engine_platform
)
