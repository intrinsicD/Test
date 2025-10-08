cmake_minimum_required(VERSION 3.20)

project(Modular3GEngine LANGUAGES CXX C)

option(BUILD_SHARED_LIBS "Build libraries as shared" ON)
option(ENGINE_ENABLE_PYTHON "Enable helpers for Python interoperability" ON)

if(ENGINE_ENABLE_PYTHON AND NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "Python interoperability requires BUILD_SHARED_LIBS=ON. Set ENGINE_ENABLE_PYTHON=OFF to build static libraries.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use libc++ only when compiling with Clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

include(CTest)
include(FetchContent)

find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_Interpreter_FOUND)
    add_custom_target(docs
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/validate_docs.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating documentation links"
        VERBATIM)
else()
    message(WARNING "Python3 interpreter not found; the docs validation target is disabled.")
endif()

if(BUILD_TESTING)
    add_subdirectory(third_party/googletest)
endif()

set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

set(ENTT_DIR "${THIRD_PARTY_DIR}/entt")
if(EXISTS "${ENTT_DIR}/CMakeLists.txt")
    add_subdirectory(${ENTT_DIR})
else()
    FetchContent_Declare(entt_external
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(entt_external)
endif()

add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/imgui)

set(GLFW_DIR "${THIRD_PARTY_DIR}/glfw")
if(EXISTS "${GLFW_DIR}/CMakeLists.txt")
    add_subdirectory(${GLFW_DIR})
else()
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

    FetchContent_Declare(glfw_external
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw_external)
endif()

add_subdirectory(engine)
