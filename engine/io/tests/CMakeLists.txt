add_executable(engine_io_tests
    test_module.cpp
    test_geometry_io.cpp
)

set_target_properties(engine_io_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_io_tests
    PRIVATE
        engine_io
        GTest::gtest_main
)

add_test(NAME engine_io_tests COMMAND engine_io_tests)
