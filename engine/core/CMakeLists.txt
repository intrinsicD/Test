set(target_name engine_core)

add_library(${target_name}
    src/api.cpp
    src/ecs/registry.cpp
    src/ecs/system.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_CORE_EXPORTS
)

target_link_libraries(${target_name}
    PUBLIC
        entt::entt
        spdlog::spdlog_header_only
        imgui::imgui
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
