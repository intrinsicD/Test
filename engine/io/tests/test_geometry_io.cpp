#include <gtest/gtest.h>

#include "engine/io/geometry_io.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace
{
    struct TempDirectory
    {
        TempDirectory()
        {
            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() /
                   ("engine-io-" + std::to_string(timestamp));
            std::filesystem::create_directories(path);
        }

        ~TempDirectory()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        std::filesystem::path path;
    };

    void write_file(const std::filesystem::path& path, std::string_view content)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream{path};
        ASSERT_TRUE(stream.good());
        stream << content;
    }

    void write_binary_stl(const std::filesystem::path& path)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream{path, std::ios::binary};
        ASSERT_TRUE(stream.good());

        std::array<char, 80U> header{};
        const std::string name = "binary stl";
        std::copy(name.begin(), name.end(), header.begin());
        stream.write(header.data(), static_cast<std::streamsize>(header.size()));

        constexpr std::uint32_t triangle_count = 1U;
        stream.write(reinterpret_cast<const char*>(&triangle_count), sizeof(triangle_count));

        const std::array<float, 12U> values{
            0.0F, 0.0F, 1.0F, // normal
            0.0F, 0.0F, 0.0F, // vertex 0
            1.0F, 0.0F, 0.0F, // vertex 1
            0.0F, 1.0F, 0.0F  // vertex 2
        };
        for (float value : values)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        constexpr std::uint16_t attribute_byte_count = 0U;
        stream.write(reinterpret_cast<const char*>(&attribute_byte_count), sizeof(attribute_byte_count));
    }
} // namespace

TEST(GeometryDetection, DetectsObjMesh)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.obj";
    write_file(path, "v 0 0 0\n"
                    "v 1 0 0\n"
                    "v 0 1 0\n"
                    "f 1 2 3\n");

    const auto detection = engine::io::detect_geometry_file(path);
    EXPECT_EQ(detection.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(detection.mesh_format, engine::io::MeshFileFormat::obj);
}

TEST(GeometryDetection, DistinguishesPlyVariants)
{
    TempDirectory temp;
    const auto mesh_path = temp.path / "mesh.ply";
    write_file(mesh_path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 3\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "element face 1\n"
               "property list uchar int vertex_indices\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "0 1 0\n"
               "3 0 1 2\n");

    const auto mesh_detection = engine::io::detect_geometry_file(mesh_path);
    EXPECT_EQ(mesh_detection.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(mesh_detection.mesh_format, engine::io::MeshFileFormat::ply);

    const auto cloud_path = temp.path / "points.ply";
    write_file(cloud_path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 2\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "end_header\n"
               "0 0 0\n"
               "1 1 1\n");

    const auto cloud_detection = engine::io::detect_geometry_file(cloud_path);
    EXPECT_EQ(cloud_detection.kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(cloud_detection.point_cloud_format, engine::io::PointCloudFileFormat::ply);
}

TEST(GeometryDetection, DetectsAsciiStlBySignature)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.stl";
    write_file(path,
               "solid ascii\n"
               "  facet normal 0 0 1\n"
               "    outer loop\n"
               "      vertex 0 0 0\n"
               "      vertex 1 0 0\n"
               "      vertex 0 1 0\n"
               "    endloop\n"
               "  endfacet\n"
               "endsolid ascii\n");

    const auto detection = engine::io::detect_geometry_file(path);
    EXPECT_EQ(detection.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(detection.mesh_format, engine::io::MeshFileFormat::stl);
}

TEST(GeometryDetection, DetectsBinaryStlByStructure)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.bin";
    write_binary_stl(path);

    const auto detection = engine::io::detect_geometry_file(path);
    EXPECT_EQ(detection.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(detection.mesh_format, engine::io::MeshFileFormat::stl);
}

TEST(GeometryIO, ReadAndWriteMesh)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.obj";
    write_file(path, "v 0 0 0\n"
                    "v 1 0 0\n"
                    "v 0 1 0\n"
                    "f 1 2 3\n");

    engine::geometry::Mesh mesh;
    engine::io::read_mesh(path, mesh.interface, engine::io::MeshFileFormat::obj);

    EXPECT_EQ(mesh.interface.vertex_count(), 3U);
    EXPECT_EQ(mesh.interface.face_count(), 1U);

    const auto out_path = temp.path / "triangle.off";
    engine::io::write_mesh(out_path, mesh.interface, engine::io::MeshFileFormat::off);

    ASSERT_TRUE(std::filesystem::exists(out_path));
    const auto detection = engine::io::detect_geometry_file(out_path);
    EXPECT_EQ(detection.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(detection.mesh_format, engine::io::MeshFileFormat::off);
}

TEST(GeometryIO, ReadAndWritePointCloud)
{
    TempDirectory temp;
    const auto path = temp.path / "points.ply";
    write_file(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 2\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "end_header\n"
               "0 0 0\n"
               "1 2 3\n");

    engine::geometry::PointCloud cloud;
    engine::io::read_point_cloud(path, cloud.interface, engine::io::PointCloudFileFormat::ply);

    EXPECT_EQ(cloud.interface.vertex_count(), 2U);

    const auto out_path = temp.path / "points.xyz";
    engine::io::write_point_cloud(out_path, cloud.interface, engine::io::PointCloudFileFormat::xyz);

    ASSERT_TRUE(std::filesystem::exists(out_path));
    const auto detection = engine::io::detect_geometry_file(out_path);
    EXPECT_EQ(detection.kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(detection.point_cloud_format, engine::io::PointCloudFileFormat::xyz);
}

TEST(GeometryIO, ReadAndWriteGraph)
{
    TempDirectory temp;
    const auto path = temp.path / "graph.edgelist";
    write_file(path,
               "0 1\n"
               "1 2\n");

    engine::geometry::Graph graph;
    engine::io::read_graph(path, graph.interface, engine::io::GraphFileFormat::edgelist);

    EXPECT_EQ(graph.interface.vertex_count(), 3U);
    EXPECT_EQ(graph.interface.edge_count(), 2U);

    const auto out_path = temp.path / "graph.ply";
    engine::io::write_graph(out_path, graph.interface, engine::io::GraphFileFormat::ply);

    ASSERT_TRUE(std::filesystem::exists(out_path));
    const auto detection = engine::io::detect_geometry_file(out_path);
    EXPECT_EQ(detection.kind, engine::io::GeometryKind::graph);
    EXPECT_EQ(detection.graph_format, engine::io::GraphFileFormat::ply);
}

TEST(GeometryIO, AutoRoutingLoadAndSave)
{
    TempDirectory temp;

    const auto mesh_path = temp.path / "mesh.obj";
    write_file(mesh_path, "v 0 0 0\n"
                             "v 1 0 0\n"
                             "v 0 1 0\n"
                             "f 1 2 3\n");

    const auto cloud_path = temp.path / "cloud.ply";
    write_file(cloud_path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 1\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "end_header\n"
               "0 0 0\n");

    const auto graph_path = temp.path / "graph.edgelist";
    write_file(graph_path, "0 1\n");

    engine::geometry::Mesh mesh;
    engine::geometry::PointCloud cloud;
    engine::geometry::Graph graph;

    const auto mesh_detection = engine::io::load_geometry(mesh_path, &mesh.interface, nullptr, nullptr);
    EXPECT_EQ(mesh_detection.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(mesh.interface.face_count(), 1U);

    const auto cloud_detection = engine::io::load_geometry(cloud_path, nullptr, &cloud.interface, nullptr);
    EXPECT_EQ(cloud_detection.kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(cloud.interface.vertex_count(), 1U);

    const auto graph_detection = engine::io::load_geometry(graph_path, nullptr, nullptr, &graph.interface);
    EXPECT_EQ(graph_detection.kind, engine::io::GeometryKind::graph);
    EXPECT_EQ(graph.interface.edge_count(), 1U);

    const auto mesh_out = temp.path / "mesh_out.ply";
    const auto mesh_save = engine::io::save_geometry(mesh_out, &mesh.interface, nullptr, nullptr);
    EXPECT_EQ(mesh_save.kind, engine::io::GeometryKind::mesh);
    ASSERT_TRUE(std::filesystem::exists(mesh_out));

    const auto cloud_out = temp.path / "cloud_out.ply";
    const auto cloud_save = engine::io::save_geometry(cloud_out, nullptr, &cloud.interface, nullptr);
    EXPECT_EQ(cloud_save.kind, engine::io::GeometryKind::point_cloud);
    ASSERT_TRUE(std::filesystem::exists(cloud_out));

    const auto graph_out = temp.path / "graph_out.edgelist";
    const auto graph_save = engine::io::save_geometry(graph_out, nullptr, nullptr, &graph.interface);
    EXPECT_EQ(graph_save.kind, engine::io::GeometryKind::graph);
    ASSERT_TRUE(std::filesystem::exists(graph_out));
}
