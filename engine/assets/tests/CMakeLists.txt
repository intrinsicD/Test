add_executable(engine_assets_tests
    test_assets.cpp
    test_module.cpp
)

set_target_properties(engine_assets_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_assets_tests
    PRIVATE
        engine_assets
        GTest::gtest_main
)

add_test(NAME engine_assets_tests COMMAND engine_assets_tests)
