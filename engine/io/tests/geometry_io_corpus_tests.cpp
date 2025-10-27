#include <gtest/gtest.h>

#include "engine/io/errors.hpp"
#include "engine/io/geometry_io.hpp"

#include <filesystem>
#include <string_view>

namespace
{
    [[nodiscard]] const std::filesystem::path& corpus_root()
    {
        static const std::filesystem::path root =
            std::filesystem::path{__FILE__}.parent_path() / "corpus" / "geometry_detection";
        return root;
    }

    struct CorpusSample
    {
        const char* file_name;
        engine::io::GeometryKind expected_kind;
        engine::io::MeshFileFormat expected_mesh_format;
        engine::io::PointCloudFileFormat expected_point_cloud_format;
        engine::io::GraphFileFormat expected_graph_format;
        const char* expected_hint;
        const char* expected_error_identifier;
    };

    class GeometryCorpusDetectionTest : public ::testing::TestWithParam<CorpusSample>
    {
    };

    TEST_P(GeometryCorpusDetectionTest, ClassifiesCorpusEntries)
    {
        const auto& sample = GetParam();
        const auto path = corpus_root() / sample.file_name;
        const auto detection = engine::io::detect_geometry_file(path);

        if (sample.expected_kind == engine::io::GeometryKind::unknown)
        {
            ASSERT_FALSE(detection);
            if (sample.expected_error_identifier != nullptr)
            {
                EXPECT_EQ(detection.error().identifier(), sample.expected_error_identifier);
            }
            return;
        }

        ASSERT_TRUE(detection) << detection.error().message();
        const auto& info = detection.value();
        EXPECT_EQ(info.kind, sample.expected_kind);
        EXPECT_EQ(info.mesh_format, sample.expected_mesh_format);
        EXPECT_EQ(info.point_cloud_format, sample.expected_point_cloud_format);
        EXPECT_EQ(info.graph_format, sample.expected_graph_format);

        if (sample.expected_hint != nullptr)
        {
            EXPECT_EQ(info.format_hint, sample.expected_hint);
        }
    }

    constexpr CorpusSample kSamples[] = {
        {
            "mesh_triangle.obj", engine::io::GeometryKind::mesh, engine::io::MeshFileFormat::obj,
            engine::io::PointCloudFileFormat::unknown, engine::io::GraphFileFormat::unknown, ".obj", nullptr
        },
        {
            "mesh_ascii.ply", engine::io::GeometryKind::mesh, engine::io::MeshFileFormat::ply,
            engine::io::PointCloudFileFormat::ply, engine::io::GraphFileFormat::ply, ".ply", nullptr
        },
        {
            "mesh_ascii.stl", engine::io::GeometryKind::mesh, engine::io::MeshFileFormat::stl,
            engine::io::PointCloudFileFormat::unknown, engine::io::GraphFileFormat::unknown, ".stl", nullptr
        },
        {
            "mesh_simple.off", engine::io::GeometryKind::mesh, engine::io::MeshFileFormat::off,
            engine::io::PointCloudFileFormat::unknown, engine::io::GraphFileFormat::unknown, ".off", nullptr
        },
        {
            "point_cloud_ascii.ply", engine::io::GeometryKind::point_cloud, engine::io::MeshFileFormat::ply,
            engine::io::PointCloudFileFormat::ply, engine::io::GraphFileFormat::ply, ".ply", nullptr
        },
        {
            "point_cloud_ascii.pcd", engine::io::GeometryKind::point_cloud, engine::io::MeshFileFormat::unknown,
            engine::io::PointCloudFileFormat::pcd, engine::io::GraphFileFormat::unknown, ".pcd", nullptr
        },
        {
            "point_cloud_basic.xyz", engine::io::GeometryKind::point_cloud, engine::io::MeshFileFormat::unknown,
            engine::io::PointCloudFileFormat::xyz, engine::io::GraphFileFormat::unknown, ".xyz", nullptr
        },
        {
            "graph_ascii.ply", engine::io::GeometryKind::graph, engine::io::MeshFileFormat::ply,
            engine::io::PointCloudFileFormat::ply, engine::io::GraphFileFormat::ply, ".ply", nullptr
        },
        {
            "graph_edgelist.txt", engine::io::GeometryKind::graph, engine::io::MeshFileFormat::unknown,
            engine::io::PointCloudFileFormat::unknown, engine::io::GraphFileFormat::edgelist, ".txt", nullptr
        },
        {
            "invalid_truncated_header.ply", engine::io::GeometryKind::point_cloud, engine::io::MeshFileFormat::ply,
            engine::io::PointCloudFileFormat::ply, engine::io::GraphFileFormat::ply, ".ply", nullptr
        },
        {
            "invalid_notply_header.ply", engine::io::GeometryKind::unknown, engine::io::MeshFileFormat::unknown,
            engine::io::PointCloudFileFormat::unknown, engine::io::GraphFileFormat::unknown, nullptr, "invalid_argument"
        },
    };

    INSTANTIATE_TEST_SUITE_P(GeometryDetectionCorpus, GeometryCorpusDetectionTest,
                             ::testing::ValuesIn(kSamples));
} // namespace