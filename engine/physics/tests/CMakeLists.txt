add_executable(engine_physics_tests
    test_module.cpp
)

set_target_properties(engine_physics_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_physics_tests
    PRIVATE
        engine_physics
        GTest::gtest_main
)

add_test(NAME engine_physics_tests COMMAND engine_physics_tests)
