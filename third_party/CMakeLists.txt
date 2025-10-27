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

# GLFW (optional, controlled by ENGINE_ENABLE_GLFW option from top-level)
set(ENGINE_GLFW_TARGET_NAME "" CACHE STRING "Resolved GLFW target name")
set(ENGINE_GLFW_AVAILABLE OFF CACHE BOOL "Is GLFW available")

if(DEFINED ENGINE_ENABLE_GLFW AND ENGINE_ENABLE_GLFW)
    if(UNIX AND NOT APPLE)
        find_path(_GLFW_XRANDR_INCLUDE "X11/extensions/Xrandr.h")
        if(NOT _GLFW_XRANDR_INCLUDE)
            message(WARNING "Xrandr headers not found; disabling GLFW support. Install libxrandr-dev to enable it.")
            set(ENGINE_ENABLE_GLFW OFF CACHE BOOL "Fetch and build GLFW to provide the GLFW window backend" FORCE)
        endif()
    endif()
endif()

if(DEFINED ENGINE_ENABLE_GLFW AND ENGINE_ENABLE_GLFW)
    set(_LOCAL_GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glfw")

    # Disable extras when consuming as submodule or FetchContent
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    # Toggle X11 backend on Linux based on option (default ON)
    option(ENGINE_GLFW_BUILD_X11 "Enable the GLFW X11 backend" ON)
    if(UNIX AND NOT APPLE)
        if(ENGINE_GLFW_BUILD_X11)
            set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
        else()
            set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
        endif()
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    endif()

    if(EXISTS "${_LOCAL_GLFW_DIR}/CMakeLists.txt")
        add_subdirectory("${_LOCAL_GLFW_DIR}" "${CMAKE_BINARY_DIR}/third_party/glfw")
    else()
        FetchContent_Declare(glfw_external
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(glfw_external)
    endif()

    if(TARGET glfw)
        set(ENGINE_GLFW_TARGET_NAME glfw CACHE STRING "Resolved GLFW target name" FORCE)
        set(ENGINE_GLFW_AVAILABLE ON CACHE BOOL "Is GLFW available" FORCE)
    elseif(TARGET glfw_shared)
        set(ENGINE_GLFW_TARGET_NAME glfw_shared CACHE STRING "Resolved GLFW target name" FORCE)
        set(ENGINE_GLFW_AVAILABLE ON CACHE BOOL "Is GLFW available" FORCE)
    else()
        message(WARNING "GLFW support requested but target not available after configuration; disabling GLFW support.")
        set(ENGINE_ENABLE_GLFW OFF CACHE BOOL "Fetch and build GLFW to provide the GLFW window backend" FORCE)
        set(ENGINE_GLFW_TARGET_NAME "" CACHE STRING "Resolved GLFW target name" FORCE)
        set(ENGINE_GLFW_AVAILABLE OFF CACHE BOOL "Is GLFW available" FORCE)
    endif()
else()
    message(STATUS "GLFW support disabled; platform module will rely on mock backend only.")
endif()

# spdlog (submodule first, fallback to FetchContent)
set(_LOCAL_SPDLOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spdlog")
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS "${_LOCAL_SPDLOG_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_SPDLOG_DIR}" "${CMAKE_BINARY_DIR}/third_party/spdlog")
else()
    FetchContent_Declare(spdlog_external
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog_external)
endif()

target_compile_definitions(spdlog PUBLIC SPDLOG_FMT_RUNTIME)

# Provide a compatibility header-only alias if only spdlog::spdlog exists
if(TARGET spdlog::spdlog AND NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog_header_only INTERFACE)
    target_link_libraries(spdlog_header_only INTERFACE spdlog::spdlog)
    add_library(spdlog::spdlog_header_only ALIAS spdlog_header_only)
endif()

# EnTT (header-only)
set(_LOCAL_ENTT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/entt")
if(EXISTS "${_LOCAL_ENTT_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_ENTT_DIR}" "${CMAKE_BINARY_DIR}/third_party/entt")
else()
    FetchContent_Declare(entt_external
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(entt_external)
endif()

# ImGui (submodule or lightweight wrapper, fallback to FetchContent)
set(_LOCAL_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(_IMGUI_CORE_FILENAMES imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp)
set(_IMGUI_BACKEND_FILENAMES backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp)

set(_IMGUI_LOCAL_HAS_CORE TRUE)
foreach(_IMGUI_FILENAME IN LISTS _IMGUI_CORE_FILENAMES)
    if(NOT EXISTS "${_LOCAL_IMGUI_DIR}/${_IMGUI_FILENAME}")
        set(_IMGUI_LOCAL_HAS_CORE FALSE)
        break()
    endif()
endforeach()

set(_IMGUI_LOCAL_HAS_BACKENDS TRUE)
foreach(_IMGUI_FILENAME IN LISTS _IMGUI_BACKEND_FILENAMES)
    if(NOT EXISTS "${_LOCAL_IMGUI_DIR}/${_IMGUI_FILENAME}")
        set(_IMGUI_LOCAL_HAS_BACKENDS FALSE)
        break()
    endif()
endforeach()

if(EXISTS "${_LOCAL_IMGUI_DIR}" AND (EXISTS "${_LOCAL_IMGUI_DIR}/CMakeLists.txt" OR _IMGUI_LOCAL_HAS_CORE))
    if(EXISTS "${_LOCAL_IMGUI_DIR}/CMakeLists.txt")
        add_subdirectory("${_LOCAL_IMGUI_DIR}" "${CMAKE_BINARY_DIR}/third_party/imgui")

        if(TARGET imgui)
            target_include_directories(imgui PRIVATE "${_LOCAL_IMGUI_DIR}")
            target_include_directories(imgui PUBLIC "${_LOCAL_IMGUI_DIR}")
        endif()
    else()
        set(_IMGUI_CORE_SOURCES)
        foreach(_IMGUI_FILENAME IN LISTS _IMGUI_CORE_FILENAMES)
            list(APPEND _IMGUI_CORE_SOURCES "${_LOCAL_IMGUI_DIR}/${_IMGUI_FILENAME}")
        endforeach()
        add_library(imgui STATIC ${_IMGUI_CORE_SOURCES})
        set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_include_directories(imgui PUBLIC "${_LOCAL_IMGUI_DIR}")
        if(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND _IMGUI_LOCAL_HAS_BACKENDS AND TARGET glad::gl_core)
            set(_IMGUI_BACKEND_SOURCES)
            foreach(_IMGUI_FILENAME IN LISTS _IMGUI_BACKEND_FILENAMES)
                list(APPEND _IMGUI_BACKEND_SOURCES "${_LOCAL_IMGUI_DIR}/${_IMGUI_FILENAME}")
            endforeach()
            target_sources(imgui PRIVATE ${_IMGUI_BACKEND_SOURCES})
            target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
            target_include_directories(imgui PUBLIC
                "${_LOCAL_IMGUI_DIR}/backends"
                $<TARGET_PROPERTY:${ENGINE_GLFW_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>
            )
            target_link_libraries(imgui PUBLIC ${ENGINE_GLFW_TARGET_NAME} glad::gl_core)
        elseif(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND _IMGUI_LOCAL_HAS_BACKENDS AND NOT TARGET glad::gl_core)
            message(STATUS "ImGui OpenGL backend skipped (no glad::gl_core); only core ImGui will be built.")
        endif()
    endif()
else()
    FetchContent_Declare(imgui_external
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.5-docking
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui_external)
    if(EXISTS "${imgui_external_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${imgui_external_SOURCE_DIR}" "${imgui_external_BINARY_DIR}")
    else()
        set(_IMGUI_CORE_SOURCES
            ${imgui_external_SOURCE_DIR}/imgui.cpp
            ${imgui_external_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_external_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
        )
        add_library(imgui STATIC ${_IMGUI_CORE_SOURCES})
        set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_include_directories(imgui PUBLIC "${imgui_external_SOURCE_DIR}")
        if(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND TARGET glad::gl_core)
            target_sources(imgui PRIVATE
                ${imgui_external_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            )
            target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
            target_include_directories(imgui PUBLIC
                ${imgui_external_SOURCE_DIR}/backends
                $<TARGET_PROPERTY:${ENGINE_GLFW_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>
            )
            target_link_libraries(imgui PUBLIC ${ENGINE_GLFW_TARGET_NAME} glad::gl_core)
        elseif(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND NOT TARGET glad::gl_core)
            message(STATUS "ImGui OpenGL backend skipped (no glad::gl_core); only core ImGui will be built.")
        endif()
    endif()
endif()
# If GLFW backend is enabled, make sure ImGui uses GLAD2 loader and link glad
if(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND TARGET imgui AND TARGET glad::gl_core)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    target_link_libraries(imgui PUBLIC glad::gl_core)
endif()

# Ensure an imgui target/alias always exist for consumers
if(NOT TARGET imgui)
    add_library(imgui INTERFACE)
    if(EXISTS "${_LOCAL_IMGUI_DIR}")
        target_include_directories(imgui INTERFACE "${_LOCAL_IMGUI_DIR}")
    elseif(DEFINED imgui_external_SOURCE_DIR AND EXISTS "${imgui_external_SOURCE_DIR}")
        target_include_directories(imgui INTERFACE "${imgui_external_SOURCE_DIR}")
    endif()
endif()
if(TARGET imgui AND NOT TARGET imgui::imgui)
    add_library(imgui::imgui ALIAS imgui)
endif()

# GLAD (OpenGL loader) — always via glad2 cmake helper; create a core 4.6 target
# Prefer local glad subdir if present; otherwise fetch helper and generate
set(_LOCAL_GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glad")
set(GLAD_GIT_TAG v2.0.8)

if(EXISTS "${_LOCAL_GLAD_DIR}/CMakeLists.txt" AND GLAD_CAN_GENERATE)
    add_subdirectory("${_LOCAL_GLAD_DIR}" "${CMAKE_BINARY_DIR}/third_party/glad")
elseif(EXISTS "${_LOCAL_GLAD_DIR}/CMakeLists.txt" AND NOT GLAD_CAN_GENERATE)
    message(WARNING "Python3/Jinja2 not available; skipping GLAD generation from local subdir. Install 'jinja2' for Python to enable glad.")
elseif(GLAD_CAN_GENERATE)
    FetchContent_Declare(glad_external
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG ${GLAD_GIT_TAG}
        SOURCE_SUBDIR cmake
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glad_external)
    if(NOT TARGET glad_gl_core)
        glad_add_library(glad_gl_core STATIC REPRODUCIBLE API gl:core=4.6)
    endif()
    if(NOT TARGET glad::gl_core)
        add_library(glad::gl_core ALIAS glad_gl_core)
    endif()
    if(UNIX AND NOT APPLE)
        target_link_libraries(glad_gl_core PUBLIC dl)
    endif()
else()
    message(STATUS "GLAD generation disabled due to missing Python3/Jinja2; glad::gl_core will not be available.")
endif()

# Googletest (tests only)
if(BUILD_TESTING)
    set(_LOCAL_GTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
    set(GOOGLETEST_GIT_TAG "v1.15.2")
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "Match engine runtime CRT linkage" FORCE)

    if(EXISTS "${_LOCAL_GTEST_DIR}/CMakeLists.txt")
        add_subdirectory("${_LOCAL_GTEST_DIR}" "${CMAKE_BINARY_DIR}/third_party/googletest")
    else()
        FetchContent_Declare(googletest_external
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG ${GOOGLETEST_GIT_TAG}
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(googletest_external)
    endif()

    if(TARGET gtest)
        target_compile_features(gtest PUBLIC cxx_std_20)
        if(TARGET engine::project_options)
            target_link_libraries(gtest PUBLIC engine::project_options)
        endif()
    endif()
    if(TARGET gtest_main)
        target_compile_features(gtest_main PUBLIC cxx_std_20)
        if(TARGET engine::project_options)
            target_link_libraries(gtest_main PUBLIC engine::project_options)
        endif()
    endif()

    if(NOT TARGET GTest::gtest AND TARGET gtest)
        add_library(GTest::gtest ALIAS gtest)
    endif()
    if(NOT TARGET GTest::gtest_main AND TARGET gtest_main)
        add_library(GTest::gtest_main ALIAS gtest_main)
    endif()
endif()

# Vulkan Memory Allocator (header-only interface)
# Prefer local copy under third_party/vma/include/vk_mem_alloc.h; fallback to fetching repo
set(_LOCAL_VMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vma")
if(EXISTS "${_LOCAL_VMA_DIR}/include/vk_mem_alloc.h")
    add_library(vma INTERFACE)
    target_include_directories(vma INTERFACE "${_LOCAL_VMA_DIR}/include")
else()
    FetchContent_Declare(vma_external
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG v3.1.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(vma_external)
    add_library(vma INTERFACE)
    target_include_directories(vma INTERFACE "${vma_external_SOURCE_DIR}")
endif()
if(NOT TARGET VMA::VMA)
    add_library(VMA::VMA ALIAS vma)
endif()
