set(target_name engine_animation)

add_library(${target_name}
    src/api.cpp
    src/serialization.cpp
    src/deformation/linear_blend_skinning.cpp
)

engine_apply_module_defaults(${target_name}
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${target_name}
    PUBLIC
        engine_math
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_ANIMATION_EXPORTS
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
