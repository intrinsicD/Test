set(target_name engine_platform)

set(platform_sources
    src/api.cpp
    src/filesystem/filesystem.cpp
    src/windowing/window_console.cpp
    src/windowing/window_system.cpp
    src/windowing/window_base.cpp
    src/windowing/mock_window.cpp
    src/windowing/sdl_window.cpp
    src/input/input_state.cpp
)

set(_engine_platform_has_glfw 0)
set(_engine_glfw_link_target "")
if(TARGET glfw)
    set(_engine_platform_has_glfw 1)
    set(_engine_glfw_link_target glfw)
    list(APPEND platform_sources src/windowing/glfw_window.cpp)
elseif(TARGET glfw_shared)
    set(_engine_platform_has_glfw 1)
    set(_engine_glfw_link_target glfw_shared)
    list(APPEND platform_sources src/windowing/glfw_window.cpp)
else()
    list(APPEND platform_sources src/windowing/glfw_window_stub.cpp)
endif()

add_library(${target_name} ${platform_sources})

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_PLATFORM_EXPORTS
        ENGINE_PLATFORM_DEFAULT_BACKEND="${ENGINE_WINDOW_BACKEND}"
        ENGINE_PLATFORM_HAS_SDL=1
        ENGINE_PLATFORM_HAS_GLFW=${_engine_platform_has_glfw}
)

if(_engine_platform_has_glfw)
    target_link_libraries(${target_name}
        PUBLIC
            ${_engine_glfw_link_target}
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

