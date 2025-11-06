add_executable(engine_runtime_tests
        test_config_schema.cpp
        test_loop.cpp
        test_module.cpp
        test_opengl_presentation_backend.cpp
        runtime_stage_planner_tests.cpp
        runtime_presentation_surface_tests.cpp
)

target_link_libraries(engine_runtime_tests
        PRIVATE
        engine::project_options
        engine_runtime
        GTest::gtest_main
        yaml-cpp::yaml-cpp
)

target_compile_definitions(engine_runtime_tests
        PRIVATE
        ENGINE_RUNTIME_ENABLE_TEST_API=1
)

add_test(NAME engine_runtime_tests COMMAND engine_runtime_tests)
