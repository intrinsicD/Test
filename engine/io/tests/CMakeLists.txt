add_executable(engine_io_tests
    geometry_io_registry_tests.cpp
    test_module.cpp
    test_geometry_io.cpp
    test_animation_importer.cpp
    geometry_io_telemetry_tests.cpp
)

target_link_libraries(engine_io_tests
    PRIVATE
        engine::project_options
        engine_io
        GTest::gtest_main
)

add_test(NAME engine_io_tests COMMAND engine_io_tests)

if(ENGINE_ENABLE_FUZZING)
    add_executable(engine_io_geometry_fuzz
        geometry_io_detection_fuzz.cpp
    )

    target_link_libraries(engine_io_geometry_fuzz
        PRIVATE
            engine::project_options
            engine_io
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(engine_io_geometry_fuzz PRIVATE -fsanitize=fuzzer,address)
        target_link_options(engine_io_geometry_fuzz PRIVATE -fsanitize=fuzzer,address)
        target_compile_definitions(engine_io_geometry_fuzz PRIVATE ENGINE_IO_FUZZ_WITH_LIBFUZZER)
    endif()
endif()
