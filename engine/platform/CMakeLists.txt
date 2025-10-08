set(target_name engine_platform)

add_library(${target_name}
    src/api.cpp
    src/window_system.cpp
    src/windowing/window_base.cpp
    src/windowing/mock_window.cpp
    src/windowing/glfw_window.cpp
    src/windowing/sdl_window.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_PLATFORM_EXPORTS
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

