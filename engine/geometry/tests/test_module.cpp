#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"

TEST(GeometryModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::geometry::module_name(), "geometry");
    EXPECT_STREQ(engine_geometry_module_name(), "geometry");
}

TEST(GeometryModule, MeshTranslationUpdatesBounds) {
    auto mesh = engine::geometry::make_unit_quad();
    engine::geometry::apply_uniform_translation(mesh, engine::math::vec3{0.0F, 1.0F, 0.0F});
    engine::geometry::recompute_vertex_normals(mesh);

    EXPECT_NEAR(mesh.bounds.min[1], 1.0F, 1e-4F);
    EXPECT_NEAR(mesh.bounds.max[1], 1.0F, 1e-4F);

    const auto center = engine::geometry::centroid(mesh);
    EXPECT_NEAR(center[1], 1.0F, 1e-4F);
}

TEST(GeometryModule, UpdateBoundsZeroesEmptyMeshes) {
    engine::geometry::SurfaceMesh mesh;
    mesh.bounds = engine::geometry::Aabb{
        engine::math::vec3{1.0F, 2.0F, 3.0F},
        engine::math::vec3{-1.0F, -2.0F, -3.0F},
    };

    engine::geometry::update_bounds(mesh);

    for (std::size_t axis = 0; axis < 3; ++axis) {
        EXPECT_FLOAT_EQ(mesh.bounds.min[axis], 0.0F);
        EXPECT_FLOAT_EQ(mesh.bounds.max[axis], 0.0F);
    }
}
