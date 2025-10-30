add_executable(engine_geometry_tests
        test_module.cpp
        test_property_registry.cpp
        test_halfedge_mesh.cpp
        test_halfedge_io.cpp
        test_graph.cpp
        test_graph_io.cpp
        test_surface_mesh_conversion.cpp
        test_surface_mesh_io.cpp
        test_surface_topology.cpp
        test_point_cloud.cpp
        test_shapes.cpp
        test_octree.cpp
        test_kdtree.cpp
        test_deformation.cpp
        test_frustum.cpp
        test_remesh_request.cpp
        test_remesh_feature_preserving.cpp
        test_remesh_adaptive.cpp
        test_remesh_uniform.cpp
        test_surface_deviation.cpp
        test_remesh_parameterization.cpp
        test_remesh_telemetry.cpp
        test_remesh_cli.cpp
)

add_executable(engine_geometry_shape_interactions_tests
        test_shape_interactions.cpp
)

target_link_libraries(engine_geometry_shape_interactions_tests
        PRIVATE
        engine::project_options
        engine_geometry
        GTest::gtest_main
)

add_test(NAME engine_geometry_shape_interactions_tests COMMAND engine_geometry_shape_interactions_tests)

# Link tests against the adapter library that depends on both engine_geometry and engine_io
# so read/write wrappers and io detection are available.
target_link_libraries(engine_geometry_tests
        PRIVATE
        engine::project_options
        engine_geometry
        engine_geometry_io
        engine_geometry_remesh_cli
        engine_platform
        GTest::gtest_main
)

add_test(NAME engine_geometry_tests COMMAND engine_geometry_tests)
