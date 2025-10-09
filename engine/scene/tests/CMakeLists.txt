add_executable(engine_scene_tests
    test_components.cpp
    test_module.cpp
    test_systems.cpp
    test_serialization.cpp
)

target_link_libraries(engine_scene_tests
    PRIVATE
        engine::project_options
        engine_scene
        GTest::gtest_main
)

add_test(NAME engine_scene_tests COMMAND engine_scene_tests)
