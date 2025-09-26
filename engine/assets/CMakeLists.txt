set(target_name engine_assets)

add_library(${target_name}
    src/api.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
