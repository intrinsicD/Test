set(target_name engine_scene)

add_library(${target_name}
        src/api.cpp
        src/selection/bounding_box_strategy.cpp
        src/selection/primitive_selection.cpp
        src/selection/selection_engine.cpp
        src/scene.cpp
        src/scene_graph_validator.cpp
        src/serialization/serializer.cpp
        src/systems/hierarchy_system.cpp
        src/systems/registry.cpp
        src/systems/transform_system.cpp
        src/validation.cpp
)

engine_apply_module_defaults(${target_name}
        PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${target_name}
        PRIVATE
        ENGINE_SCENE_EXPORTS
)

# Scene graph relies on core systems.
target_link_libraries(${target_name}
        PUBLIC
        EnTT::EnTT
        engine_core
        engine_geometry
        engine_math
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

add_subdirectory(samples)
