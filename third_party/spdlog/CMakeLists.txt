add_library(spdlog_header_only INTERFACE)

target_include_directories(spdlog_header_only
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(spdlog_header_only INTERFACE cxx_std_20)

add_library(spdlog::spdlog_header_only ALIAS spdlog_header_only)
