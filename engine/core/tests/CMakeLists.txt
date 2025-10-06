add_executable(engine_core_tests
    test_module.cpp
    ecs_registry_tests.cpp
)

set_target_properties(engine_core_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_core_tests
    PRIVATE
        engine_core
        GTest::gtest_main
)

add_test(NAME engine_core_tests COMMAND engine_core_tests)
