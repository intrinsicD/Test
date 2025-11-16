set(target_name engine_tools)

add_library(${target_name}
        src/api.cpp
        src/editor/asset_browser_panel.cpp
        src/editor/performance_metrics_panel.cpp
        src/editor/telemetry_visualization_panel.cpp
        src/editor/runtime_panel_bridge.cpp
        src/editor/scene_hierarchy_panel.cpp
        src/profiling/profiler.cpp
        src/imgui_helpers.cpp
        src/imgui/panel_registry.cpp
        src/sandbox/benchmark_runner.cpp
        src/sandbox/configuration_loader.cpp
        src/sandbox/experiment_sandbox.cpp
)

engine_apply_module_defaults(${target_name}
        PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
        PRIVATE
        ENGINE_TOOLS_EXPORTS
)

target_link_libraries(${target_name}
        PUBLIC
        engine_core
        engine_runtime
        imgui::imgui
        spdlog::spdlog_header_only
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

# Add examples subdirectory
add_subdirectory(examples)
