set(target_name engine_scene)

add_library(${target_name}
    src/api.cpp
    src/scene.cpp
    serialization/serializer.cpp
    systems/hierarchy_system.cpp
    systems/registry.cpp
    systems/transform_system.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/entt/include
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_SCENE_EXPORTS
)

# Scene graph relies on core systems.
target_link_libraries(${target_name}
    PUBLIC
        engine_core
        engine_math
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
