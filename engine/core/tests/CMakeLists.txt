add_executable(engine_core_tests
    test_module.cpp
    ecs_registry_tests.cpp
)

target_link_libraries(engine_core_tests
    PRIVATE
        engine::project_options
        engine_core
        GTest::gtest_main
)

add_test(NAME engine_core_tests COMMAND engine_core_tests)
