cmake_minimum_required(VERSION 3.20)

project(Modular3GEngine LANGUAGES CXX C)


set(ENGINE_MODULES
    math
    animation
    assets
    compute
    core
    geometry
    io
    physics
    platform
    rendering
    scene
    runtime
)

option(ENGINE_ENABLE_CUDA "Enable CUDA support for compute module targets" OFF)

set(_engine_window_backend_options GLFW SDL MOCK)
set(ENGINE_WINDOW_BACKEND "GLFW" CACHE STRING
    "Default window backend (GLFW, SDL, MOCK)")
set_property(CACHE ENGINE_WINDOW_BACKEND PROPERTY STRINGS ${_engine_window_backend_options})
string(TOUPPER "${ENGINE_WINDOW_BACKEND}" _engine_window_backend_normalised)
list(FIND _engine_window_backend_options "${_engine_window_backend_normalised}" _engine_window_backend_index)
if(_engine_window_backend_index EQUAL -1)
    string(REPLACE ";" ", " _engine_window_backend_valid "${_engine_window_backend_options}")
    message(FATAL_ERROR
        "ENGINE_WINDOW_BACKEND must be one of: ${_engine_window_backend_valid}. "
        "Received '${ENGINE_WINDOW_BACKEND}'.")
endif()
set(ENGINE_WINDOW_BACKEND "${_engine_window_backend_normalised}" CACHE STRING
    "Default window backend (GLFW, SDL, MOCK)" FORCE)
unset(_engine_window_backend_index)
unset(_engine_window_backend_normalised)
unset(_engine_window_backend_valid)
unset(_engine_window_backend_options)

set(ENGINE_OPTIONAL_SUBSYSTEMS
    animation
    assets
    compute
    compute_cuda
    core
    geometry
    io
    physics
    platform
    rendering
    scene
)

foreach(subsystem IN LISTS ENGINE_OPTIONAL_SUBSYSTEMS)
    string(TOUPPER "${subsystem}" subsystem_upper)
    set(option_default ON)
    if(subsystem STREQUAL "compute_cuda")
        set(option_default ${ENGINE_ENABLE_CUDA})
    endif()
    option(ENGINE_ENABLE_${subsystem_upper} "Enable ${subsystem} subsystem integration" ${option_default})
endforeach()

if(NOT ENGINE_ENABLE_CUDA AND ENGINE_ENABLE_COMPUTE_CUDA)
    message(WARNING "ENGINE_ENABLE_COMPUTE_CUDA requested but ENGINE_ENABLE_CUDA=OFF; disabling compute CUDA subsystem integration.")
    set(ENGINE_ENABLE_COMPUTE_CUDA OFF CACHE BOOL "Enable compute_cuda subsystem integration" FORCE)
endif()

foreach(subsystem IN LISTS ENGINE_OPTIONAL_SUBSYSTEMS)
    string(TOUPPER "${subsystem}" subsystem_upper)
    if(ENGINE_ENABLE_${subsystem_upper})
        add_compile_definitions(ENGINE_ENABLE_${subsystem_upper}=1)
    else()
        add_compile_definitions(ENGINE_ENABLE_${subsystem_upper}=0)
    endif()
endforeach()

option(BUILD_SHARED_LIBS "Build libraries as shared" ON)
option(ENGINE_ENABLE_PYTHON "Enable helpers for Python interoperability" ON)

if(ENGINE_ENABLE_PYTHON AND NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "Python interoperability requires BUILD_SHARED_LIBS=ON. Set ENGINE_ENABLE_PYTHON=OFF to build static libraries.")
endif()

add_library(engine_project_options INTERFACE)
target_compile_features(engine_project_options INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(engine_project_options INTERFACE -stdlib=libc++)
    target_link_options(engine_project_options INTERFACE -stdlib=libc++)
    target_link_libraries(engine_project_options INTERFACE c++)
endif()

add_library(engine_module_headers INTERFACE)

add_library(engine::project_options ALIAS engine_project_options)
add_library(engine::headers ALIAS engine_module_headers)

target_link_libraries(engine_module_headers INTERFACE engine::project_options)

function(engine_apply_module_defaults target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "engine_apply_module_defaults called with unknown target '${target}'")
    endif()

    set(options)
    set(one_value_args)
    set(multi_value_args PUBLIC_INCLUDE_DIRS PRIVATE_INCLUDE_DIRS)
    cmake_parse_arguments(ENGINE_MODULE "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ENGINE_MODULE_PUBLIC_INCLUDE_DIRS)
        target_include_directories(engine_module_headers INTERFACE
            ${ENGINE_MODULE_PUBLIC_INCLUDE_DIRS}
        )
    endif()

    get_target_property(_target_type ${target} TYPE)

    if(_target_type STREQUAL "INTERFACE_LIBRARY")
        if(ENGINE_MODULE_PUBLIC_INCLUDE_DIRS)
            target_include_directories(${target}
                INTERFACE
                    ${ENGINE_MODULE_PUBLIC_INCLUDE_DIRS}
            )
        endif()

        target_link_libraries(${target}
            INTERFACE
                engine::project_options
        )
    else()
        if(ENGINE_MODULE_PUBLIC_INCLUDE_DIRS)
            target_include_directories(${target}
                PUBLIC
                    ${ENGINE_MODULE_PUBLIC_INCLUDE_DIRS}
            )
        endif()

        if(ENGINE_MODULE_PRIVATE_INCLUDE_DIRS)
            target_include_directories(${target}
                PRIVATE
                    ${ENGINE_MODULE_PRIVATE_INCLUDE_DIRS}
            )
        endif()

        target_link_libraries(${target}
            PUBLIC
                engine::project_options
        )
    endif()
endfunction()

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
    add_subdirectory(engine/tests/integration)
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

    option(ENGINE_GLFW_BUILD_X11 "Enable the GLFW X11 backend" ON)
    if(ENGINE_GLFW_BUILD_X11)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    else()
        set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    endif()

    FetchContent_Declare(glfw_external
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw_external)
endif()

add_subdirectory(engine)
