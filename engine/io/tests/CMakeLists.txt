add_executable(engine_io_tests
    geometry_io_registry_tests.cpp
    test_module.cpp
    test_geometry_io.cpp
    test_animation_importer.cpp
)

target_link_libraries(engine_io_tests
    PRIVATE
        engine::project_options
        engine_io
        GTest::gtest_main
)

add_test(NAME engine_io_tests COMMAND engine_io_tests)
