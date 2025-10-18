set(target_name engine_integration_tests)

add_executable(${target_name}
    test_runtime_integration.cpp
)

# Allow reuse of rendering test utilities for command encoder recording helpers.
target_include_directories(${target_name}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/engine/rendering/tests
)

target_link_libraries(${target_name}
    PRIVATE
        engine::project_options
        engine_runtime
        GTest::gtest_main
)

add_test(NAME engine_integration_tests COMMAND $<TARGET_FILE:${target_name}>)

set_property(TEST engine_integration_tests PROPERTY LABELS integration)
set_tests_properties(engine_integration_tests
    PROPERTIES
        ENVIRONMENT "ENGINE_PLATFORM_WINDOW_BACKEND=mock;ENGINE_WINDOW_BACKEND=MOCK"
        TIMEOUT 90
)
