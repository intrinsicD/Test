include(FetchContent)

set(GOOGLETEST_GIT_TAG "v1.15.2")

set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock build" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest installation" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "Match engine runtime CRT linkage" FORCE)

FetchContent_Declare(googletest_external
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG ${GOOGLETEST_GIT_TAG}
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(googletest_external)

if(NOT TARGET gtest OR NOT TARGET gtest_main)
    message(FATAL_ERROR "Upstream Googletest targets were not created as expected")
endif()

target_compile_features(gtest PUBLIC cxx_std_20)
target_link_libraries(gtest PUBLIC engine::project_options)

target_link_libraries(gtest_main PUBLIC engine::project_options)

if(NOT TARGET GTest::gtest)
    add_library(GTest::gtest ALIAS gtest)
endif()

if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()
