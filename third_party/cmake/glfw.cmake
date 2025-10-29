include_guard(GLOBAL)

# Public contract (cache so parents can inspect)
set(ENGINE_GLFW_TARGET_NAME "" CACHE STRING "Resolved GLFW target name")
set(ENGINE_GLFW_AVAILABLE OFF CACHE BOOL "Is GLFW available")

# Respect top-level switch
if(NOT DEFINED ENGINE_ENABLE_GLFW OR NOT ENGINE_ENABLE_GLFW)
    message(STATUS "GLFW support disabled; platform module will rely on mock backend only.")
    return()
endif()

# On Linux, ensure Xrandr headers exist if building X11 backend
if(UNIX AND NOT APPLE)
    find_path(_GLFW_XRANDR_INCLUDE "X11/extensions/Xrandr.h")
    if(NOT _GLFW_XRANDR_INCLUDE)
        message(WARNING "Xrandr headers not found; disabling GLFW support. Install 'libxrandr-dev' to enable it.")
        set(ENGINE_ENABLE_GLFW OFF CACHE BOOL "Fetch and build GLFW to provide the GLFW window backend" FORCE)
        return()
    endif()
endif()

set(_LOCAL_GLFW_DIR "${THIRD_PARTY_DIR}/glfw")
set(_GLFW_GIT_TAG 3.4)

# Trim glfw extras
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Linux backends
option(ENGINE_GLFW_BUILD_X11 "Enable the GLFW X11 backend" ON)
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_X11 ${ENGINE_GLFW_BUILD_X11} CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()

# Populate
if(EXISTS "${_LOCAL_GLFW_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_GLFW_DIR}" "${CMAKE_BINARY_DIR}/third_party/glfw")
else()
    FetchContent_Declare(glfw_external
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG ${_GLFW_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw_external)
endif()

# Resolve target and publish availability
if(TARGET glfw)
    set(ENGINE_GLFW_TARGET_NAME glfw CACHE STRING "Resolved GLFW target name" FORCE)
    set(ENGINE_GLFW_AVAILABLE ON CACHE BOOL "Is GLFW available" FORCE)
elseif(TARGET glfw_shared)
    set(ENGINE_GLFW_TARGET_NAME glfw_shared CACHE STRING "Resolved GLFW target name" FORCE)
    set(ENGINE_GLFW_AVAILABLE ON CACHE BOOL "Is GLFW available" FORCE)
else()
    message(WARNING "GLFW requested but no target available after configuration; disabling GLFW support.")
    set(ENGINE_ENABLE_GLFW OFF CACHE BOOL "Fetch and build GLFW to provide the GLFW window backend" FORCE)
    set(ENGINE_GLFW_TARGET_NAME "" CACHE STRING "Resolved GLFW target name" FORCE)
    set(ENGINE_GLFW_AVAILABLE OFF CACHE BOOL "Is GLFW available" FORCE)
endif()

unset(_LOCAL_GLFW_DIR)
unset(_GLFW_GIT_TAG)
unset(_GLFW_XRANDR_INCLUDE)
