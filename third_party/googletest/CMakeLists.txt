add_library(gtest INTERFACE)

target_include_directories(gtest
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(gtest INTERFACE cxx_std_20)

target_link_libraries(gtest
        INTERFACE
        engine::project_options)

add_library(gtest_main STATIC
        src/gtest_main.cpp
)

target_link_libraries(gtest_main
        PUBLIC
        gtest
)


target_compile_features(gtest_main PUBLIC cxx_std_20)

add_library(GTest::gtest ALIAS gtest)
add_library(GTest::gtest_main ALIAS gtest_main)
