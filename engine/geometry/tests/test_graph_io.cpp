#include <filesystem>
#include <system_error>

#include <gtest/gtest.h>

#include "engine/geometry/graph/graph.hpp"

namespace geo = engine::geometry;
namespace graph_ns = engine::geometry::graph;

namespace
{
    struct TemporaryPath
    {
        TemporaryPath()
        {
            const auto temp_directory = std::filesystem::temp_directory_path();
            path = temp_directory / std::filesystem::unique_path("engine-graph-%%%%-%%%%.graph");
        }

        TemporaryPath(const TemporaryPath&) = delete;
        TemporaryPath& operator=(const TemporaryPath&) = delete;

        TemporaryPath(TemporaryPath&&) = delete;
        TemporaryPath& operator=(TemporaryPath&&) = delete;

        ~TemporaryPath()
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }

        std::filesystem::path path;
    };
}

TEST(GraphIO, RoundTripEdgeList)
{
    geo::Graph source;

    const auto v0 = source.interface.add_vertex({0.0F, 0.0F, 0.0F});
    const auto v1 = source.interface.add_vertex({1.0F, 0.0F, 0.0F});
    const auto v2 = source.interface.add_vertex({0.0F, 1.0F, 0.0F});

    ASSERT_TRUE(source.interface.add_edge(v0, v1).is_valid());
    ASSERT_TRUE(source.interface.add_edge(v1, v2).is_valid());
    ASSERT_TRUE(source.interface.add_edge(v2, v0).is_valid());

    TemporaryPath temporary_path;

    ASSERT_NO_THROW(graph_ns::write(source.interface, temporary_path.path));

    geo::Graph loaded;
    ASSERT_NO_THROW(graph_ns::read(loaded.interface, temporary_path.path));

    EXPECT_EQ(loaded.interface.vertex_count(), 3U);
    EXPECT_EQ(loaded.interface.edge_count(), 3U);

    EXPECT_FLOAT_EQ(loaded.interface.position(geo::VertexHandle(0))[0], 0.0F);
    EXPECT_FLOAT_EQ(loaded.interface.position(geo::VertexHandle(1))[0], 1.0F);
    EXPECT_FLOAT_EQ(loaded.interface.position(geo::VertexHandle(2))[1], 1.0F);

    const auto he01 = loaded.interface.find_halfedge(geo::VertexHandle(0), geo::VertexHandle(1));
    EXPECT_TRUE(he01.has_value());

    const auto he12 = loaded.interface.find_halfedge(geo::VertexHandle(1), geo::VertexHandle(2));
    EXPECT_TRUE(he12.has_value());

    const auto he20 = loaded.interface.find_halfedge(geo::VertexHandle(2), geo::VertexHandle(0));
    EXPECT_TRUE(he20.has_value());
}
