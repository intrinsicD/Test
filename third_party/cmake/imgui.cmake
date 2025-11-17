include_guard(GLOBAL)

# ImGui (submodule or lightweight wrapper, fallback to FetchContent)
## The imgui sources live next to this cmake module in ../imgui (i.e. third_party/imgui).
## Use CMAKE_CURRENT_LIST_DIR which is the directory containing this script, then go up one
## level to reach the `third_party/imgui` folder. Using CMAKE_CURRENT_SOURCE_DIR here was
## incorrect because it points to the parent project scope when this module is included.
set(_LOCAL_IMGUI_DIR "${CMAKE_CURRENT_LIST_DIR}/../imgui")
set(_IMGUI_ROOT_DIR "")
set(_IMGUI_GIT_TAG v1.90.5-docking)
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
        set(_IMGUI_ROOT_DIR "${_LOCAL_IMGUI_DIR}")
    else()
        set(_IMGUI_CORE_SOURCES)
        foreach(_IMGUI_FILENAME IN LISTS _IMGUI_CORE_FILENAMES)
            list(APPEND _IMGUI_CORE_SOURCES "${_LOCAL_IMGUI_DIR}/${_IMGUI_FILENAME}")
        endforeach()
        add_library(imgui STATIC ${_IMGUI_CORE_SOURCES})
        # Ensure imgui is compiled with the same C++ standard library when building
        # as a plain STATIC target (the main project uses libc++ with Clang).
        # Use a generator expression so this only applies for Clang.
        target_compile_options(imgui PUBLIC $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>)
        set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_include_directories(imgui PUBLIC "${_LOCAL_IMGUI_DIR}")
        set(_IMGUI_ROOT_DIR "${_LOCAL_IMGUI_DIR}")
        if(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND _IMGUI_LOCAL_HAS_BACKENDS)
            set(_IMGUI_BACKEND_SOURCES)
            foreach(_IMGUI_FILENAME IN LISTS _IMGUI_BACKEND_FILENAMES)
                list(APPEND _IMGUI_BACKEND_SOURCES "${_LOCAL_IMGUI_DIR}/${_IMGUI_FILENAME}")
            endforeach()
            # Add backend source files to imgui target even if glad::gl_core is not yet available.
            target_sources(imgui PRIVATE ${_IMGUI_BACKEND_SOURCES})
            target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
            target_include_directories(imgui PUBLIC
                    "${_LOCAL_IMGUI_DIR}/backends"
                    $<TARGET_PROPERTY:${ENGINE_GLFW_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>
            )
            # Link GLFW platform library; link glad loader only if the glad target exists at this time.
            target_link_libraries(imgui PUBLIC ${ENGINE_GLFW_TARGET_NAME})
            # On X11-based platforms the imgui GLFW backend references Xlib functions; ensure X11 is linked.
            if(UNIX AND NOT APPLE)
                find_package(X11 REQUIRED)
                if(X11_LIBRARIES)
                    target_link_libraries(imgui PUBLIC ${X11_LIBRARIES})
                endif()
            endif()
            if(TARGET glad::gl_core)
                target_link_libraries(imgui PUBLIC glad::gl_core)
            endif()
        elseif(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND NOT TARGET glad::gl_core)
            message(STATUS "ImGui OpenGL backend skipped (no glad::gl_core); only core ImGui will be built.")
        endif()
    endif()
else()
    FetchContent_Declare(imgui_external
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG ${_IMGUI_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui_external)
    if(EXISTS "${imgui_external_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${imgui_external_SOURCE_DIR}" "${imgui_external_BINARY_DIR}")
        set(_IMGUI_ROOT_DIR "${imgui_external_SOURCE_DIR}")
    else()
        set(_IMGUI_CORE_SOURCES
                ${imgui_external_SOURCE_DIR}/imgui.cpp
                ${imgui_external_SOURCE_DIR}/imgui_draw.cpp
                ${imgui_external_SOURCE_DIR}/imgui_tables.cpp
                ${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
        )
        add_library(imgui STATIC ${_IMGUI_CORE_SOURCES})
        # Ensure imgui is compiled with the same C++ standard library as the rest
        # of the project (apply -stdlib=libc++ for Clang so symbols use std::__1).
        target_compile_options(imgui PUBLIC $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>)
        set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_include_directories(imgui PUBLIC "${imgui_external_SOURCE_DIR}")
        set(_IMGUI_ROOT_DIR "${imgui_external_SOURCE_DIR}")
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
            target_link_libraries(imgui PUBLIC ${ENGINE_GLFW_TARGET_NAME})
            # On X11-based platforms the imgui GLFW backend references Xlib functions; ensure X11 is linked.
            if(UNIX AND NOT APPLE)
                find_package(X11 REQUIRED)
                if(X11_LIBRARIES)
                    target_link_libraries(imgui PUBLIC ${X11_LIBRARIES})
                endif()
            endif()
            if(TARGET glad::gl_core)
                target_link_libraries(imgui PUBLIC glad::gl_core)
            endif()
        elseif(DEFINED ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_AVAILABLE AND ENGINE_GLFW_TARGET_NAME AND NOT TARGET glad::gl_core)
            message(STATUS "ImGui OpenGL backend skipped (no glad::gl_core); only core ImGui will be built.")
        endif()
    endif()
endif()
# Pull in ImGui stdlib helpers when available so std::string overloads are linked.
if(TARGET imgui AND _IMGUI_ROOT_DIR AND EXISTS "${_IMGUI_ROOT_DIR}/misc/cpp/imgui_stdlib.cpp")
    target_sources(imgui PRIVATE "${_IMGUI_ROOT_DIR}/misc/cpp/imgui_stdlib.cpp")
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

unset(_LOCAL_IMGUI_DIR)
unset(_IMGUI_GIT_TAG)
unset(_IMGUI_CORE_FILENAMES)
unset(_IMGUI_BACKEND_FILENAMES)
unset(_IMGUI_LOCAL_HAS_CORE)
unset(_IMGUI_LOCAL_HAS_BACKENDS)
unset(_IMGUI_CORE_SOURCES)
unset(_IMGUI_BACKEND_SOURCES)
unset(_IMGUI_ROOT_DIR)