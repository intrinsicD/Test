set(target_name engine_rendering)

add_library(${target_name}
        src/api.cpp
        src/backend/validation.cpp
        src/backend/opengl/command_encoder.cpp
        src/backend/opengl/command_stream.cpp
        src/backend/opengl/immediate_command_stream.cpp
        src/backend/mock/presentation_backend.cpp
        src/backend/opengl/presentation_backend.cpp
        src/backend/opengl/resource_provider.cpp
        src/backend/opengl/render_resource_provider.cpp
        src/backend/opengl/runtime_adapter.cpp
        src/backend/vulkan/command_encoder.cpp
        src/backend/vulkan/resource_provider.cpp
        src/command_encoder.cpp
        src/command_encoder_tracing.cpp
        src/frame_graph.cpp
        src/frame_graph_execution.cpp
        src/frame_graph_planner.cpp
        src/frame_graph_registry.cpp
        src/forward_pipeline.cpp
        src/pipeline/research_baseline_telemetry.cpp
        src/pipeline/research_baseline.cpp
        src/material_system.cpp
        src/resources/recording_gpu_resource_provider.cpp
)

engine_apply_module_defaults(${target_name}
        PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(${target_name}
        PRIVATE
        ENGINE_RENDERING_EXPORTS
)

find_package(Vulkan QUIET)

if (Vulkan_FOUND)
    target_sources(${target_name}
            PRIVATE
            src/backend/vulkan/resource_translation.cpp
    )

    if (TARGET Vulkan::Vulkan)
        target_link_libraries(${target_name}
                PUBLIC
                Vulkan::Vulkan
        )
    elseif (Vulkan_LIBRARIES)
        target_link_libraries(${target_name}
                PUBLIC
                ${Vulkan_LIBRARIES}
        )
    endif ()

    if (Vulkan_INCLUDE_DIRS)
        target_include_directories(${target_name}
                PRIVATE
                ${Vulkan_INCLUDE_DIRS}
        )
    endif ()

    target_compile_definitions(${target_name}
            PUBLIC
            ENGINE_RENDERING_HAS_VULKAN=1
    )
else ()
    message(STATUS "Vulkan SDK not found; Vulkan backend translation helpers will be disabled.")
    target_compile_definitions(${target_name}
            PUBLIC
            ENGINE_RENDERING_HAS_VULKAN=0
    )
endif ()

set(ENGINE_RENDERING_VULKAN_AVAILABLE ${Vulkan_FOUND})

# Rendering depends on core platform layers.
target_link_libraries(${target_name}
        PUBLIC
        engine_core
        engine_geometry
        engine_assets
        engine_platform
        engine_scene
)

# Link GLAD library directly by path
set(GLAD_LIBRARY "${CMAKE_BINARY_DIR}/third_party/libglad_gl_core.so")
set(GLAD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/third_party/gladsources/glad_gl_core/include")

if(EXISTS "${GLAD_LIBRARY}" AND EXISTS "${GLAD_INCLUDE_DIR}")
    target_link_libraries(${target_name} PUBLIC ${GLAD_LIBRARY})
    target_include_directories(${target_name} PUBLIC ${GLAD_INCLUDE_DIR})
    target_compile_definitions(${target_name} PUBLIC ENGINE_RENDERING_HAS_GLAD=1)
    message(STATUS "OpenGL GLAD: ${GLAD_LIBRARY}")

    if(UNIX AND NOT APPLE)
        target_link_libraries(${target_name} PUBLIC dl)
    endif()
else()
    message(WARNING "GLAD not found: ${GLAD_LIBRARY}")
    target_compile_definitions(${target_name} PUBLIC ENGINE_RENDERING_HAS_GLAD=0)
endif ()

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
