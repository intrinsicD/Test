# Centralized third_party dependency management

cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# Check for Python and Jinja2 (required by glad2 code generator)
find_package(Python3 COMPONENTS Interpreter QUIET)
set(GLAD_CAN_GENERATE OFF)
if(Python3_Interpreter_FOUND)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import jinja2"
        RESULT_VARIABLE _JINJA2_STATUS
        OUTPUT_QUIET ERROR_QUIET
    )
    if(_JINJA2_STATUS EQUAL 0)
        set(GLAD_CAN_GENERATE ON)
    endif()
endif()

# Provide this for consumers
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to third_party directory" FORCE)

# Option defined at top level; keep here only if not already defined
if(NOT DEFINED ENGINE_ENABLE_GLFW)
    option(ENGINE_ENABLE_GLFW "Fetch and build GLFW to provide the GLFW window backend" OFF)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/glfw.cmake") # sets ENGINE_GLFW_AVAILABLE and ENGINE_GLFW_TARGET_NAME
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/spdlog.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/yaml-cpp.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/entt.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/imgui.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/glad.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/vma.cmake")

# Googletest (tests only)
if(BUILD_TESTING)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/googletest.cmake")
endif()
