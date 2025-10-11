#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <system_error>

#include "engine/geometry/api.hpp"

namespace geo = engine::geometry;

namespace
{
    void cleanup_directory(const std::filesystem::path& directory)
    {
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
        (void)ec;
    }
} // namespace

TEST(SurfaceMeshIO, SaveAndLoadRoundTrip)
{
    const geo::SurfaceMesh original = geo::make_unit_quad();

    const auto directory = std::filesystem::temp_directory_path() / std::filesystem::unique_path("geo-surface-io-%%%%");
    const auto path = directory / "surface.obj";

    std::filesystem::create_directories(directory);

    ASSERT_NO_THROW(geo::save_surface_mesh(original, path));

    const auto round_trip = geo::load_surface_mesh(path);

    EXPECT_EQ(round_trip.positions.size(), original.positions.size());
    EXPECT_EQ(round_trip.indices.size(), original.indices.size());

    for (std::size_t i = 0; i < original.positions.size(); ++i)
    {
        EXPECT_FLOAT_EQ(round_trip.positions[i][0], original.positions[i][0]);
        EXPECT_FLOAT_EQ(round_trip.positions[i][1], original.positions[i][1]);
        EXPECT_FLOAT_EQ(round_trip.positions[i][2], original.positions[i][2]);
        EXPECT_FLOAT_EQ(round_trip.rest_positions[i][0], original.rest_positions[i][0]);
        EXPECT_FLOAT_EQ(round_trip.rest_positions[i][1], original.rest_positions[i][1]);
        EXPECT_FLOAT_EQ(round_trip.rest_positions[i][2], original.rest_positions[i][2]);
    }

    for (std::size_t i = 0; i < original.indices.size(); ++i)
    {
        EXPECT_EQ(round_trip.indices[i], original.indices[i]);
    }

    EXPECT_FALSE(round_trip.normals.empty());

    cleanup_directory(directory);
}

TEST(SurfaceMeshIO, RejectsDegenerateSurfaceOnSave)
{
    geo::SurfaceMesh surface;
    surface.positions = {engine::math::vec3{0.0F, 0.0F, 0.0F},
                         engine::math::vec3{0.0F, 0.0F, 0.0F},
                         engine::math::vec3{1.0F, 0.0F, 0.0F}};
    surface.indices = {0U, 1U, 2U};

    const auto directory = std::filesystem::temp_directory_path() / std::filesystem::unique_path("geo-surface-io-invalid-%%%%");
    const auto path = directory / "surface.obj";

    std::filesystem::create_directories(directory);

    EXPECT_THROW(geo::save_surface_mesh(surface, path), std::runtime_error);

    cleanup_directory(directory);
}

