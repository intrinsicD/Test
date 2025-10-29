add_executable(test_tools_module
        test_module.cpp
        test_profiler.cpp
        test_experiment_sandbox.cpp
        test_sandbox_configuration_loader.cpp
        test_benchmark_runner.cpp)

target_link_libraries(test_tools_module
        PRIVATE
        engine::project_options
        engine_tools
        yaml-cpp::yaml-cpp
        gtest_main)

add_test(NAME test_tools_module COMMAND test_tools_module)
