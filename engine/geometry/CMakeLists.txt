set(target_name engine_geometry)

add_library(${target_name}
    src/api.cpp
    src/graph/graph.cpp
    src/properties/property_registry.cpp
    src/properties/property_handle.cpp
    src/mesh/halfedge_mesh.cpp
    src/point_cloud/point_cloud.cpp
    src/shapes/aabb.cpp
    src/shapes/cylinder.cpp
    src/shapes/ellipsoid.cpp
    src/shapes/line.cpp
    src/shapes/obb.cpp
    src/shapes/plane.cpp
    src/shapes/ray.cpp
    src/shapes/random.cpp
    src/shapes/segment.cpp
    src/shapes/sphere.cpp
    src/shapes/triangle.cpp
    src/utils/shape_interactions.cpp
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
