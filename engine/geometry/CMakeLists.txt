set(target_name engine_geometry)

add_library(${target_name}
    src/api.cpp
    src/shapes/aabb.cpp
    src/shapes/cylinder.cpp
    src/shapes/ellipsoid.cpp
    src/shapes/line.cpp
    src/shapes/obb.cpp
    src/shapes/plane.cpp
    src/shapes/ray.cpp
    src/shapes/segment.cpp
    src/shapes/sphere.cpp
    src/shapes/triangle.cpp
)

target_include_directories(${target_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${target_name}
    PUBLIC
        engine_math
)

target_compile_definitions(${target_name}
    PRIVATE
        ENGINE_GEOMETRY_EXPORTS
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
