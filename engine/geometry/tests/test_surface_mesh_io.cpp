#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <system_error>

#include "engine/geometry/api.hpp"
#include "engine/platform/filesystem/filesystem.hpp"

namespace geo = engine::geometry;
namespace fs = engine::platform::filesystem;

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

    const auto directory = std::filesystem::temp_directory_path() / ("geo-surface-io-" + fs::generate_random_suffix());
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

TEST(SurfaceMeshIO, RoundTripFatPrismPreservesBounds)
{
    geo::SurfaceMesh prism;
    prism.rest_positions = {
        engine::math::vec3{-10.0F, -2.0F, -5.0F},
        engine::math::vec3{10.0F, -2.0F, -5.0F},
        engine::math::vec3{10.0F, 2.0F, -5.0F},
        engine::math::vec3{-10.0F, 2.0F, -5.0F},
        engine::math::vec3{-10.0F, -2.0F, 5.0F},
        engine::math::vec3{10.0F, -2.0F, 5.0F},
        engine::math::vec3{10.0F, 2.0F, 5.0F},
        engine::math::vec3{-10.0F, 2.0F, 5.0F},
    };
    prism.positions = prism.rest_positions;
    prism.indices = {
        0U, 1U, 2U, 0U, 2U, 3U, // bottom
        4U, 6U, 5U, 4U, 7U, 6U, // top
        0U, 4U, 5U, 0U, 5U, 1U, // front
        1U, 5U, 6U, 1U, 6U, 2U, // right
        2U, 6U, 7U, 2U, 7U, 3U, // back
        3U, 7U, 4U, 3U, 4U, 0U  // left
    };

    geo::update_bounds(prism);
    const auto original_bounds = prism.bounds;
    const auto original_centroid = geo::centroid(prism);

    const auto directory = std::filesystem::temp_directory_path() /
                           ("geo-fat-surface-" + fs::generate_random_suffix());
    const auto path = directory / "fat_prism.obj";

    std::filesystem::create_directories(directory);

    ASSERT_NO_THROW(geo::save_surface_mesh(prism, path));

    const auto round_trip = geo::load_surface_mesh(path);

    ASSERT_EQ(round_trip.positions.size(), prism.positions.size());
    ASSERT_EQ(round_trip.indices.size(), prism.indices.size());

    for (std::size_t i = 0; i < prism.positions.size(); ++i)
    {
        EXPECT_FLOAT_EQ(round_trip.positions[i][0], prism.positions[i][0]);
        EXPECT_FLOAT_EQ(round_trip.positions[i][1], prism.positions[i][1]);
        EXPECT_FLOAT_EQ(round_trip.positions[i][2], prism.positions[i][2]);
    }

    for (std::size_t i = 0; i < prism.indices.size(); ++i)
    {
        EXPECT_EQ(round_trip.indices[i], prism.indices[i]);
    }

    EXPECT_FLOAT_EQ(round_trip.bounds.min[0], original_bounds.min[0]);
    EXPECT_FLOAT_EQ(round_trip.bounds.min[1], original_bounds.min[1]);
    EXPECT_FLOAT_EQ(round_trip.bounds.min[2], original_bounds.min[2]);
    EXPECT_FLOAT_EQ(round_trip.bounds.max[0], original_bounds.max[0]);
    EXPECT_FLOAT_EQ(round_trip.bounds.max[1], original_bounds.max[1]);
    EXPECT_FLOAT_EQ(round_trip.bounds.max[2], original_bounds.max[2]);

    const auto round_trip_centroid = geo::centroid(round_trip);
    EXPECT_FLOAT_EQ(round_trip_centroid[0], original_centroid[0]);
    EXPECT_FLOAT_EQ(round_trip_centroid[1], original_centroid[1]);
    EXPECT_FLOAT_EQ(round_trip_centroid[2], original_centroid[2]);

    cleanup_directory(directory);
}

TEST(SurfaceMeshIO, RejectsDegenerateSurfaceOnSave)
{
    geo::SurfaceMesh surface;
    surface.positions = {engine::math::vec3{0.0F, 0.0F, 0.0F},
                         engine::math::vec3{0.0F, 0.0F, 0.0F},
                         engine::math::vec3{1.0F, 0.0F, 0.0F}};
    surface.indices = {0U, 1U, 2U};

    const auto directory = std::filesystem::temp_directory_path() / ("geo-surface-io-invalid-" + fs::generate_random_suffix());
    const auto path = directory / "surface.obj";

    std::filesystem::create_directories(directory);

    EXPECT_THROW(geo::save_surface_mesh(surface, path), std::runtime_error);

    cleanup_directory(directory);
}

