set(target_name engine_platform)

add_library(${target_name}
    src/api.cpp
    src/filesystem/filesystem.cpp
        src/windowing/window_console.cpp
        src/windowing/window_system.cpp
    src/windowing/window_base.cpp
    src/windowing/mock_window.cpp
    src/windowing/glfw_window.cpp
    src/windowing/sdl_window.cpp
    src/input/input_state.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_PLATFORM_EXPORTS
        ENGINE_PLATFORM_DEFAULT_BACKEND="${ENGINE_WINDOW_BACKEND}"
)

if(TARGET glfw)
    target_link_libraries(${target_name}
        PUBLIC
            glfw
    )
elseif(TARGET glfw_shared)
    target_link_libraries(${target_name}
        PUBLIC
            glfw_shared
    )
else()
    message(FATAL_ERROR "GLFW target not available. Ensure third_party/glfw is fetched during configuration.")
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

