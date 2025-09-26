add_library(engine_math INTERFACE)

target_include_directories(engine_math
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enable CUDA consumption by marking host/device capable headers.
# Consumers can include this target without linking additional objects.
