#include <cmath>

#include <gtest/gtest.h>

#include "engine/rendering/backend/opengl/render_resource_provider.hpp"

#include "engine/geometry/api.hpp"

namespace
{
    using Provider = engine::rendering::backend::opengl::OpenGLRenderResourceProvider;
    using engine::assets::MeshHandle;
    using engine::geometry::SurfaceMesh;
    using engine::math::vec2;
    using engine::math::vec3;
}

TEST(OpenGLRenderResourceProvider, LoadsMeshUsingResolver)
{
    bool resolver_called = false;
    auto resolver = [&](const MeshHandle& handle) -> std::optional<SurfaceMesh>
    {
        resolver_called = true;
        EXPECT_EQ(handle.id(), "triangle.mesh");

        SurfaceMesh mesh;
        mesh.positions = {vec3{0.0F, 0.0F, 0.0F}, vec3{1.0F, 0.0F, 0.0F}, vec3{0.0F, 1.0F, 0.0F}};
        mesh.texture_coordinates = {vec2{0.0F, 0.0F}, vec2{1.0F, 0.0F}, vec2{0.0F, 1.0F}};
        mesh.indices = {0U, 1U, 2U};
        return mesh;
    };

    Provider provider(resolver);
    MeshHandle handle{std::string{"triangle.mesh"}};
    provider.require_mesh(handle);

    EXPECT_TRUE(resolver_called);
    const auto* record = provider.mesh(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->positions.size(), 3U);
    EXPECT_EQ(record->indices.size(), 3U);
    EXPECT_EQ(record->texture_coordinates.size(), 3U);
    EXPECT_FALSE(record->normals.empty());

    for (const auto& normal : record->normals)
    {
        EXPECT_NEAR(normal[0], 0.0F, 1e-5F);
        EXPECT_NEAR(normal[1], 0.0F, 1e-5F);
        EXPECT_NEAR(std::fabs(normal[2]), 1.0F, 1e-5F);
    }

    EXPECT_FLOAT_EQ(record->bounds.min[0], 0.0F);
    EXPECT_FLOAT_EQ(record->bounds.min[1], 0.0F);
    EXPECT_FLOAT_EQ(record->bounds.max[0], 1.0F);
    EXPECT_FLOAT_EQ(record->bounds.max[1], 1.0F);
}

TEST(OpenGLRenderResourceProvider, CachesMeshesAfterFirstLoad)
{
    int resolver_count = 0;
    auto resolver = [&](const MeshHandle&) -> std::optional<SurfaceMesh>
    {
        ++resolver_count;
        SurfaceMesh mesh;
        mesh.positions = {vec3{0.0F, 0.0F, 0.0F}, vec3{1.0F, 0.0F, 0.0F}, vec3{0.0F, 1.0F, 0.0F}};
        mesh.indices = {0U, 1U, 2U};
        return mesh;
    };

    Provider provider(resolver);
    MeshHandle handle{std::string{"cached.mesh"}};

    provider.require_mesh(handle);
    provider.require_mesh(handle);
    provider.require_mesh(handle);

    EXPECT_EQ(resolver_count, 1);
    EXPECT_EQ(provider.loaded_mesh_count(), 1U);
}

TEST(OpenGLRenderResourceProvider, IgnoresEmptyHandles)
{
    bool resolver_called = false;
    auto resolver = [&](const MeshHandle&) -> std::optional<SurfaceMesh>
    {
        resolver_called = true;
        return std::nullopt;
    };

    Provider provider(resolver);
    MeshHandle empty_handle;

    provider.require_mesh(empty_handle);
    EXPECT_FALSE(resolver_called);
    EXPECT_EQ(provider.loaded_mesh_count(), 0U);
}

TEST(OpenGLRenderResourceProvider, ThrowsWhenResolverFails)
{
    auto resolver = [](const MeshHandle&) -> std::optional<SurfaceMesh> { return std::nullopt; };
    Provider provider(resolver);
    MeshHandle handle{std::string{"invalid.mesh"}};

    EXPECT_THROW(provider.require_mesh(handle), std::runtime_error);
    EXPECT_EQ(provider.loaded_mesh_count(), 0U);
}
