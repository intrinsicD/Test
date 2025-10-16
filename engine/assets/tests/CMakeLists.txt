add_executable(engine_assets_tests
    test_assets.cpp
    test_async.cpp
    test_module.cpp
)

target_link_libraries(engine_assets_tests
    PRIVATE
        engine::project_options
        engine_assets
        GTest::gtest_main
)

add_test(NAME engine_assets_tests COMMAND engine_assets_tests)
