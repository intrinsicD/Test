include_guard(GLOBAL)

# ImGui (submodule or lightweight wrapper, fallback to FetchContent)
set(_LOCAL_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
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
            GIT_TAG ${_IMGUI_GIT_TAG}
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

unset(_LOCAL_IMGUI_DIR)
unset(_IMGUI_GIT_TAG)
unset(_IMGUI_CORE_FILENAMES)
unset(_IMGUI_BACKEND_FILENAMES)
unset(_IMGUI_LOCAL_HAS_CORE)
unset(_IMGUI_LOCAL_HAS_BACKENDS)
unset(_IMGUI_CORE_SOURCES)
unset(_IMGUI_BACKEND_SOURCES)