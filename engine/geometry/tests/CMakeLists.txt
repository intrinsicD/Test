add_executable(engine_geometry_tests
    test_module.cpp
)

set_target_properties(engine_geometry_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_geometry_tests
    PRIVATE
        engine_geometry
        GTest::gtest_main
)

add_test(NAME engine_geometry_tests COMMAND engine_geometry_tests)
