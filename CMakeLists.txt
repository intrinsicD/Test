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

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(third_party/googletest)
endif()

add_subdirectory(engine)
