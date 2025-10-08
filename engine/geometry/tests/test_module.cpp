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
