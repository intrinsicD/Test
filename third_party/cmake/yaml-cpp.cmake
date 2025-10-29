include_guard(GLOBAL)

# yaml-cpp (YAML parser for configuration manifests)
set(_LOCAL_YAMLCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp")
set(_YAMLCPP_GIT_TAG 0.8.0)
# Configure yaml-cpp options before population
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Allow old subprojects (cmake<3.5)" FORCE)

if (EXISTS "${_LOCAL_YAMLCPP_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_YAMLCPP_DIR}" "${CMAKE_BINARY_DIR}/third_party/yaml-cpp")
else ()
    FetchContent_Declare(yaml_cpp_external
            GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
            GIT_TAG ${_YAMLCPP_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(yaml_cpp_external)
endif ()

if (TARGET yaml-cpp)
    target_compile_features(yaml-cpp PUBLIC cxx_std_17)
    # Make sure yaml-cpp inherits our project options (e.g., -stdlib=libc++)
    if (TARGET engine::project_options)
        target_link_libraries(yaml-cpp PUBLIC engine::project_options)
    endif ()
endif ()

if (NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif ()

unset(_LOCAL_YAMLCPP_DIR)
unset(_YAMLCPP_GIT_TAG)
unset(CMAKE_POLICY_VERSION_MINIMUM)