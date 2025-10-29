include_guard(GLOBAL)

# GLAD (OpenGL loader) — always via glad2 cmake helper; create a core 4.6 target
# Prefer local glad subdir if present; otherwise fetch helper and generate
set(_LOCAL_GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glad")
set(_GLAD_GIT_TAG v2.0.8)

if(EXISTS "${_LOCAL_GLAD_DIR}/CMakeLists.txt" AND GLAD_CAN_GENERATE)
    add_subdirectory("${_LOCAL_GLAD_DIR}" "${CMAKE_BINARY_DIR}/third_party/glad")
elseif(EXISTS "${_LOCAL_GLAD_DIR}/CMakeLists.txt" AND NOT GLAD_CAN_GENERATE)
    message(WARNING "Python3/Jinja2 not available; skipping GLAD generation from local subdir. Install 'jinja2' for Python to enable glad.")
elseif(GLAD_CAN_GENERATE)
    FetchContent_Declare(glad_external
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG ${_GLAD_GIT_TAG}
            SOURCE_SUBDIR cmake
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glad_external)
    if(NOT TARGET glad_gl_core)
        glad_add_library(glad_gl_core STATIC REPRODUCIBLE API gl:core=4.6)
    endif()
    if(NOT TARGET glad::gl_core)
        add_library(glad::gl_core ALIAS glad_gl_core)
    endif()
    if(UNIX AND NOT APPLE)
        target_link_libraries(glad_gl_core PUBLIC dl)
    endif()
else()
    message(STATUS "GLAD generation disabled due to missing Python3/Jinja2; glad::gl_core will not be available.")
endif()

unset(_LOCAL_GLAD_DIR)
unset(_GLAD_GIT_TAG)