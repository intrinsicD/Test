#include <gtest/gtest.h>

#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    [[nodiscard]] std::filesystem::path make_temporary_obj_path(const std::string& stem)
    {
        const auto pattern = stem + "-%%%%-%%%%-%%%%-%%%%.obj";
        return std::filesystem::temp_directory_path() / std::filesystem::unique_path(pattern);
    }

    void remove_if_exists(const std::filesystem::path& path)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
} // namespace

TEST(HalfedgeMeshIO, ReadsTriangleObj)
{
    const auto path = make_temporary_obj_path("triangle");
    {
        std::ofstream stream(path);
        ASSERT_TRUE(stream.is_open());
        stream << "v 0 0 0\n";
        stream << "v 1 0 0\n";
        stream << "v 0 1 0\n";
        stream << "f 1 2 3\n";
    }

    engine::geometry::Mesh mesh;
    ASSERT_NO_THROW(engine::geometry::mesh::read(mesh.interface, path));

    EXPECT_EQ(mesh.interface.vertex_count(), 3U);
    EXPECT_EQ(mesh.interface.face_count(), 1U);

    std::vector<engine::math::vec3> positions;
    for (auto vertex : mesh.interface.vertices())
    {
        if (mesh.interface.is_deleted(vertex))
        {
            continue;
        }
        positions.emplace_back(mesh.interface.position(vertex));
    }

    ASSERT_EQ(positions.size(), 3U);
    EXPECT_FLOAT_EQ(positions[0][0], 0.0F);
    EXPECT_FLOAT_EQ(positions[0][1], 0.0F);
    EXPECT_FLOAT_EQ(positions[0][2], 0.0F);

    EXPECT_FLOAT_EQ(positions[1][0], 1.0F);
    EXPECT_FLOAT_EQ(positions[1][1], 0.0F);
    EXPECT_FLOAT_EQ(positions[1][2], 0.0F);

    EXPECT_FLOAT_EQ(positions[2][0], 0.0F);
    EXPECT_FLOAT_EQ(positions[2][1], 1.0F);
    EXPECT_FLOAT_EQ(positions[2][2], 0.0F);

    std::size_t face_vertex_count = 0;
    for (auto face : mesh.interface.faces())
    {
        if (mesh.interface.is_deleted(face))
        {
            continue;
        }

        auto circulator = mesh.interface.vertices(face);
        const auto end = circulator;
        if (circulator)
        {
            do
            {
                ++face_vertex_count;
            }
            while (++circulator != end);
        }
    }

    EXPECT_EQ(face_vertex_count, 3U);

    remove_if_exists(path);
}

TEST(HalfedgeMeshIO, WritesAndReadsQuadObj)
{
    engine::geometry::Mesh mesh;

    const auto v0 = mesh.interface.add_vertex(engine::math::vec3{0.0F, 0.0F, 0.0F});
    const auto v1 = mesh.interface.add_vertex(engine::math::vec3{1.0F, 0.0F, 0.0F});
    const auto v2 = mesh.interface.add_vertex(engine::math::vec3{1.0F, 1.0F, 0.0F});
    const auto v3 = mesh.interface.add_vertex(engine::math::vec3{0.0F, 1.0F, 0.0F});

    ASSERT_TRUE(mesh.interface.add_quad(v0, v1, v2, v3).has_value());

    const auto path = make_temporary_obj_path("quad");

    engine::geometry::mesh::IOFlags flags;
    flags.format = engine::geometry::mesh::IOFlags::Format::kOBJ;
    flags.precision = 6;
    flags.include_header_comment = false;

    ASSERT_NO_THROW(engine::geometry::mesh::write(mesh.interface, path, flags));

    engine::geometry::Mesh round_trip;
    ASSERT_NO_THROW(engine::geometry::mesh::read(round_trip.interface, path));

    EXPECT_EQ(round_trip.interface.vertex_count(), 4U);
    EXPECT_EQ(round_trip.interface.face_count(), 1U);

    std::vector<engine::math::vec3> observed_positions;
    for (auto vertex : round_trip.interface.vertices())
    {
        if (round_trip.interface.is_deleted(vertex))
        {
            continue;
        }
        observed_positions.emplace_back(round_trip.interface.position(vertex));
    }

    ASSERT_EQ(observed_positions.size(), 4U);
    EXPECT_FLOAT_EQ(observed_positions[0][0], 0.0F);
    EXPECT_FLOAT_EQ(observed_positions[0][1], 0.0F);
    EXPECT_FLOAT_EQ(observed_positions[0][2], 0.0F);

    EXPECT_FLOAT_EQ(observed_positions[1][0], 1.0F);
    EXPECT_FLOAT_EQ(observed_positions[1][1], 0.0F);
    EXPECT_FLOAT_EQ(observed_positions[1][2], 0.0F);

    EXPECT_FLOAT_EQ(observed_positions[2][0], 1.0F);
    EXPECT_FLOAT_EQ(observed_positions[2][1], 1.0F);
    EXPECT_FLOAT_EQ(observed_positions[2][2], 0.0F);

    EXPECT_FLOAT_EQ(observed_positions[3][0], 0.0F);
    EXPECT_FLOAT_EQ(observed_positions[3][1], 1.0F);
    EXPECT_FLOAT_EQ(observed_positions[3][2], 0.0F);

    std::size_t quad_vertex_count = 0;
    for (auto face : round_trip.interface.faces())
    {
        if (round_trip.interface.is_deleted(face))
        {
            continue;
        }

        auto circulator = round_trip.interface.vertices(face);
        const auto end = circulator;
        if (circulator)
        {
            do
            {
                ++quad_vertex_count;
            }
            while (++circulator != end);
        }
    }

    EXPECT_EQ(quad_vertex_count, 4U);

    remove_if_exists(path);
}
