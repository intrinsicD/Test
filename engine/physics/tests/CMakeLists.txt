add_executable(engine_physics_tests
    test_module.cpp
)

target_link_libraries(engine_physics_tests
    PRIVATE
        engine::project_options
        engine_physics
        GTest::gtest_main
)

add_test(NAME engine_physics_tests COMMAND engine_physics_tests)
