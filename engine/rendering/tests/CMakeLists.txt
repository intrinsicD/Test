set(ENGINE_RENDERING_TEST_SOURCES
        test_module.cpp
        test_frame_graph.cpp
        test_camera.cpp
        test_forward_pipeline.cpp
        test_opengl_command_encoder.cpp
        test_opengl_runtime_adapter.cpp
        test_opengl_render_resource_provider.cpp
        test_opengl_resource_provider.cpp
        test_research_baseline.cpp
        test_backend_adapters.cpp
        test_backend_validation.cpp
)

if (ENGINE_RENDERING_VULKAN_AVAILABLE)
    list(APPEND ENGINE_RENDERING_TEST_SOURCES
            test_vulkan_resource_translation.cpp
    )
endif ()

add_executable(engine_rendering_tests
        ${ENGINE_RENDERING_TEST_SOURCES}
)

target_link_libraries(engine_rendering_tests
        PRIVATE
        engine::project_options
        engine_geometry
        engine_rendering
        GTest::gtest_main
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(engine_rendering_tests PRIVATE -Wl,--no-as-needed)
endif ()

add_test(NAME engine_rendering_tests COMMAND engine_rendering_tests)
