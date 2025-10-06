add_library(imgui_header_only INTERFACE)

target_include_directories(imgui_header_only
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(imgui_header_only INTERFACE cxx_std_20)

add_library(imgui::imgui ALIAS imgui_header_only)
