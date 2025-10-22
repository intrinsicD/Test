add_library(imgui SHARED
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(imgui PUBLIC cxx_std_20)
target_compile_options(imgui PRIVATE -fPIC)
set_target_properties(imgui PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(imgui PUBLIC glfw)

add_library(imgui::imgui ALIAS imgui)
