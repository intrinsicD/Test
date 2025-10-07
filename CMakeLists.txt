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

add_subdirectory(third_party/entt)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/glfw)
add_subdirectory(engine)
