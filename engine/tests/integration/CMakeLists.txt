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
        yaml-cpp::yaml-cpp
)

add_test(NAME engine_integration_tests COMMAND $<TARGET_FILE:${target_name}>)

set_property(TEST engine_integration_tests PROPERTY LABELS integration)
set_tests_properties(engine_integration_tests
        PROPERTIES
        ENVIRONMENT "ENGINE_PLATFORM_WINDOW_BACKEND=mock;ENGINE_WINDOW_BACKEND=MOCK"
        TIMEOUT 90
)

if(Python3_Interpreter_FOUND)
    set(_ai004_geometry_summary "${CMAKE_BINARY_DIR}/artifacts/ai004/geometry-baseline-summary.json")
    add_test(NAME runtime_prototype_harness_geometry_case_study
            COMMAND ${Python3_EXECUTABLE}
                    -m scripts.prototyping.run_prototype_harness
                    --case-study geometry-baseline
                    --dry-run
                    --require-schema
                    --summary-json ${_ai004_geometry_summary}
    )
    set_property(TEST runtime_prototype_harness_geometry_case_study PROPERTY LABELS integration)
    set_tests_properties(runtime_prototype_harness_geometry_case_study
            PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            ENVIRONMENT "ENGINE_PLATFORM_WINDOW_BACKEND=mock;ENGINE_WINDOW_BACKEND=MOCK;PYTHONUNBUFFERED=1"
            PASS_REGULAR_EXPRESSION "Dry run summary"
            FAIL_REGULAR_EXPRESSION "error:"
            TIMEOUT 120
    )

    set(_ai004_rendering_summary "${CMAKE_BINARY_DIR}/artifacts/ai004/rendering-debug-summary.json")
    add_test(NAME runtime_prototype_harness_rendering_case_study
            COMMAND ${Python3_EXECUTABLE}
                    -m scripts.prototyping.run_prototype_harness
                    --case-study rendering-debug
                    --dry-run
                    --require-schema
                    --summary-json ${_ai004_rendering_summary}
    )
    set_property(TEST runtime_prototype_harness_rendering_case_study PROPERTY LABELS integration)
    set_tests_properties(runtime_prototype_harness_rendering_case_study
            PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            ENVIRONMENT "ENGINE_PLATFORM_WINDOW_BACKEND=mock;ENGINE_WINDOW_BACKEND=MOCK;PYTHONUNBUFFERED=1"
            PASS_REGULAR_EXPRESSION "Dry run summary"
            FAIL_REGULAR_EXPRESSION "error:"
            TIMEOUT 120
    )
else()
    message(WARNING "Python3 interpreter not found; skipping runtime prototype harness case study integration tests.")
endif()

set(_runtime_harness_summary "${CMAKE_BINARY_DIR}/artifacts/ai004/runtime-sample-summary.json")
add_test(NAME runtime_prototype_harness_sample_dry_run
        COMMAND $<TARGET_FILE:runtime_prototype_harness>
                --config ${CMAKE_SOURCE_DIR}/docs/examples/ai004_sample.json
                --dry-run
                --require-schema
                --summary-json ${_runtime_harness_summary}
)
set_property(TEST runtime_prototype_harness_sample_dry_run PROPERTY LABELS integration)
set_tests_properties(runtime_prototype_harness_sample_dry_run
        PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT "ENGINE_PLATFORM_WINDOW_BACKEND=mock;ENGINE_WINDOW_BACKEND=MOCK"
        PASS_REGULAR_EXPRESSION "dispatch count: 0"
        FAIL_REGULAR_EXPRESSION "error:"
        TIMEOUT 60
)
