add_executable(engine_scene_tests
    test_components.cpp
    test_module.cpp
    test_systems.cpp
)

set_target_properties(engine_scene_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_scene_tests
    PRIVATE
        engine_scene
        GTest::gtest_main
)

add_test(NAME engine_scene_tests COMMAND engine_scene_tests)
