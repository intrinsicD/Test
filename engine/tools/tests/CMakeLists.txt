add_executable(test_tools_module
        test_module.cpp
        test_profiler.cpp
        test_experiment_sandbox.cpp)

target_link_libraries(test_tools_module
        PRIVATE
        engine_tools
        gtest_main)

add_test(NAME test_tools_module COMMAND test_tools_module)
