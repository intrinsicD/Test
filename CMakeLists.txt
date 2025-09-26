cmake_minimum_required(VERSION 3.20)

project(Modular3GEngine LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(engine)
