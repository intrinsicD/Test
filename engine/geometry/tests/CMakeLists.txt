add_executable(engine_geometry_tests
        test_module.cpp
        test_property_registry.cpp
        test_halfedge_mesh.cpp
        test_graph.cpp
        test_point_cloud.cpp
        test_shapes.cpp
        test_octree.cpp
)

add_executable(engine_geometry_shape_interactions_tests
        test_shape_interactions.cpp
)

set_target_properties(engine_geometry_shape_interactions_tests PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_geometry_shape_interactions_tests
        PRIVATE
        engine_geometry
        GTest::gtest_main
)

add_test(NAME engine_geometry_shape_interactions_tests COMMAND engine_geometry_shape_interactions_tests)

set_target_properties(engine_geometry_tests PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
)

target_link_libraries(engine_geometry_tests
        PRIVATE
        engine_geometry
        GTest::gtest_main
)

add_test(NAME engine_geometry_tests COMMAND engine_geometry_tests)
