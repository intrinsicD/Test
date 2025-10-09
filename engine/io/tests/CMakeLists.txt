add_executable(engine_io_tests
    test_module.cpp
    test_geometry_io.cpp
)

target_link_libraries(engine_io_tests
    PRIVATE
        engine::project_options
        engine_io
        GTest::gtest_main
)

add_test(NAME engine_io_tests COMMAND engine_io_tests)
