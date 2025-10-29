include_guard(GLOBAL)

set(_LOCAL_GTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
set(_GOOGLETEST_GIT_TAG "v1.15.2")

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "Match engine runtime CRT linkage" FORCE)

if(EXISTS "${_LOCAL_GTEST_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_GTEST_DIR}" "${CMAKE_BINARY_DIR}/third_party/googletest")
else()
    FetchContent_Declare(googletest_external
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG ${_GOOGLETEST_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest_external)
endif()

if(NOT TARGET gtest OR NOT TARGET gtest_main)
    message(FATAL_ERROR "Upstream Googletest targets were not created as expected")
endif()

target_compile_features(gtest  PUBLIC cxx_std_20)
target_link_libraries(gtest PRIVATE engine::project_options)
target_link_libraries(gtest_main PRIVATE engine::project_options)

# Disable warnings while compiling GoogleTest itself
target_compile_options(gtest PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W0>
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-w>
)
target_compile_options(gtest_main PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W0>
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-w>
)

set_target_properties(gtest gtest_main PROPERTIES COMPILE_WARNING_AS_ERROR OFF)

if(NOT TARGET GTest::gtest)
    add_library(GTest::gtest ALIAS gtest)
endif()

if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()

unset(_LOCAL_GTEST_DIR)
unset(_GOOGLETEST_GIT_TAG)