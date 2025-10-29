include_guard(GLOBAL)

# spdlog (submodule first, fallback to FetchContent)
set(_LOCAL_SPDLOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spdlog")
set(_SPDLOG_GIT_TAG v1.13.0)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS "${_LOCAL_SPDLOG_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_SPDLOG_DIR}" "${CMAKE_BINARY_DIR}/third_party/spdlog")
else()
    FetchContent_Declare(spdlog_external
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG ${_SPDLOG_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog_external)
endif()

#target_compile_definitions(spdlog PUBLIC SPDLOG_FMT_RUNTIME)

# Provide a compatibility header-only alias if only spdlog::spdlog exists
if(TARGET spdlog::spdlog AND NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog_header_only INTERFACE)
    target_link_libraries(spdlog_header_only INTERFACE spdlog::spdlog)
    add_library(spdlog::spdlog_header_only ALIAS spdlog_header_only)
endif()

unset(_LOCAL_SPDLOG_DIR)
unset(_SPDLOG_GIT_TAG)