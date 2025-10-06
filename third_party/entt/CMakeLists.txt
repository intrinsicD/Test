add_library(entt INTERFACE)

set_target_properties(entt PROPERTIES EXPORT_NAME entt)

target_include_directories(entt
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(entt INTERFACE cxx_std_20)

add_library(entt::entt ALIAS entt)
