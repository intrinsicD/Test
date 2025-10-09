add_library(engine_math INTERFACE)

engine_apply_module_defaults(engine_math
    PUBLIC_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enable CUDA consumption by marking host/device capable headers.
# Consumers can include this target without linking additional objects.

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
