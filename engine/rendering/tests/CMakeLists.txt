add_executable(engine_rendering_tests
    test_module.cpp
    test_frame_graph.cpp
    test_forward_pipeline.cpp
    test_backend_adapters.cpp
)

target_link_libraries(engine_rendering_tests
    PRIVATE
        engine::project_options
        engine_rendering
        GTest::gtest_main
)

add_test(NAME engine_rendering_tests COMMAND engine_rendering_tests)
