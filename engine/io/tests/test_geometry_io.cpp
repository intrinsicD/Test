#include <gtest/gtest.h>

#include "engine/io/errors.hpp"
#include "engine/io/geometry_io.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <system_error>
#include <utility>

namespace engine::io::detail
{
    void reset_geometry_signature_cache_for_testing();
}

namespace
{
    namespace io_detail = engine::io::detail;

#if defined(_WIN32)
    void set_env_variable(const char* name, const std::string& value)
    {
        _putenv_s(name, value.c_str());
    }

    void unset_env_variable(const char* name)
    {
        _putenv_s(name, "");
    }
#else
    void set_env_variable(const char* name, const std::string& value)
    {
        ::setenv(name, value.c_str(), 1);
    }

    void unset_env_variable(const char* name)
    {
        ::unsetenv(name);
    }
#endif

    class SignatureDatabaseOverride
    {
    public:
        explicit SignatureDatabaseOverride(const std::filesystem::path& path)
        {
            set_env_variable("ENGINE_IO_GEOMETRY_SIGNATURE_PATH", path.string());
            io_detail::reset_geometry_signature_cache_for_testing();
        }

        SignatureDatabaseOverride(const SignatureDatabaseOverride&) = delete;
        SignatureDatabaseOverride& operator=(const SignatureDatabaseOverride&) = delete;

        ~SignatureDatabaseOverride()
        {
            unset_env_variable("ENGINE_IO_GEOMETRY_SIGNATURE_PATH");
            io_detail::reset_geometry_signature_cache_for_testing();
        }
    };

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

        std::array < char, 80U > header{};
        const std::string name = "binary stl";
        std::copy(name.begin(), name.end(), header.begin());
        stream.write(header.data(), static_cast<std::streamsize>(header.size()));

        constexpr std::uint32_t triangle_count = 1U;
        stream.write(reinterpret_cast<const char*>(&triangle_count), sizeof(triangle_count));

        const std::array<float, 12U> values{
            0.0F, 0.0F, 1.0F, // normal
            0.0F, 0.0F, 0.0F, // vertex 0
            1.0F, 0.0F, 0.0F, // vertex 1
            0.0F, 1.0F, 0.0F // vertex 2
        };
        for (float value : values)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        constexpr std::uint16_t attribute_byte_count = 0U;
        stream.write(reinterpret_cast<const char*>(&attribute_byte_count), sizeof(attribute_byte_count));
    }

    void write_off_mesh(const std::filesystem::path& path)
    {
        write_file(path,
                   "OFF\n"
                   "3 1 0\n"
                   "0 0 0\n"
                   "1 0 0\n"
                   "0 1 0\n"
                   "3 0 1 2\n");
    }

    void write_pcd_point_cloud(const std::filesystem::path& path)
    {
        write_file(path,
                   "# .PCD v0.7 - Point Cloud Data file format\n"
                   "VERSION 0.7\n"
                   "FIELDS x y z\n"
                   "SIZE 4 4 4\n"
                   "TYPE F F F\n"
                   "COUNT 1 1 1\n"
                   "WIDTH 2\n"
                   "HEIGHT 1\n"
                   "POINTS 2\n"
                   "DATA ascii\n"
                   "0 0 0\n"
                   "1 1 1\n");
    }

    void write_edgelist_graph(const std::filesystem::path& path)
    {
        write_file(path,
                   "# Graph edges\n"
                   "0 1\n"
                   "1 2\n"
                   "2 3\n");
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
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::obj);
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
    ASSERT_TRUE(mesh_detection);
    const auto& mesh_info = mesh_detection.value();
    EXPECT_EQ(mesh_info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(mesh_info.mesh_format, engine::io::MeshFileFormat::ply);

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
    ASSERT_TRUE(cloud_detection);
    const auto& cloud_info = cloud_detection.value();
    EXPECT_EQ(cloud_info.kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(cloud_info.point_cloud_format, engine::io::PointCloudFileFormat::ply);
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
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::stl);
}

TEST(GeometryDetection, DetectsBinaryStlByStructure)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.bin";
    write_binary_stl(path);

    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::stl);
}

TEST(GeometryDetection, DetectsObjWithoutExtension)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle";
    write_file(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n");

    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::obj);
}

TEST(GeometryDetection, DetectsOffWithoutExtension)
{
    TempDirectory temp;
    const auto path = temp.path / "mesh";
    write_off_mesh(path);

    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::off);
}

TEST(GeometryDetection, DetectsPcdWithoutExtension)
{
    TempDirectory temp;
    const auto path = temp.path / "cloud";
    write_pcd_point_cloud(path);

    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(info.point_cloud_format, engine::io::PointCloudFileFormat::pcd);
}

TEST(GeometryDetection, DetectsEdgelistWithoutExtension)
{
    TempDirectory temp;
    const auto path = temp.path / "graph";
    write_edgelist_graph(path);

    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::graph);
    EXPECT_EQ(info.graph_format, engine::io::GraphFileFormat::edgelist);
}

TEST(GeometryDetection, UsesSignatureDatabaseOverride)
{
    TempDirectory temp;
    const auto db_path = temp.path / "custom_signatures.json";
    write_file(db_path,
               "{\n"
               "  \"rules\": [\n"
               "    {\n"
               "      \"id\": \"mesh.custom.marker\",\n"
               "      \"kind\": \"mesh\",\n"
               "      \"mesh_format\": \"obj\",\n"
               "      \"match\": {\n"
               "        \"type\": \"line_prefix\",\n"
               "        \"patterns\": [\"@meshdb\"],\n"
               "        \"case_sensitive\": false\n"
               "      },\n"
               "      \"format_hint\": \".obj\"\n"
               "    }\n"
               "  ]\n"
               "}\n");

    SignatureDatabaseOverride override{db_path};

    const auto asset_path = temp.path / "custom.asset";
    write_file(asset_path,
               "@meshdb\n"
               "points\n");

    const auto detection = engine::io::detect_geometry_file(asset_path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::obj);
    EXPECT_EQ(info.format_hint, ".obj");
}

TEST(GeometryDetection, FallsBackWhenSignatureDatabaseMissing)
{
    TempDirectory temp;
    const auto missing_db_path = temp.path / "missing.json";
    SignatureDatabaseOverride override{missing_db_path};

    const auto path = temp.path / "fallback.off";
    write_off_mesh(path);

    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::off);
}

TEST(GeometryDetection, ReportsMissingFile)
{
    const auto path = std::filesystem::path{"/nonexistent/geometry.obj"};
    const auto detection = engine::io::detect_geometry_file(path);
    ASSERT_FALSE(detection);
    const auto& error = detection.error();
    EXPECT_EQ(error.code(), engine::io::GeometryIoError::file_not_found);
    EXPECT_EQ(error.identifier(), "file_not_found");
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
    ASSERT_TRUE(engine::io::read_mesh(path, mesh.interface, engine::io::MeshFileFormat::obj));

    EXPECT_EQ(mesh.interface.vertex_count(), 3U);
    EXPECT_EQ(mesh.interface.face_count(), 1U);

    const auto out_path = temp.path / "triangle.off";
    ASSERT_TRUE(engine::io::write_mesh(out_path, mesh.interface, engine::io::MeshFileFormat::off));

    ASSERT_TRUE(std::filesystem::exists(out_path));
    const auto detection = engine::io::detect_geometry_file(out_path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(info.mesh_format, engine::io::MeshFileFormat::off);
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
    ASSERT_TRUE(engine::io::read_point_cloud(path, cloud.interface, engine::io::PointCloudFileFormat::ply));

    EXPECT_EQ(cloud.interface.vertex_count(), 2U);

    const auto out_path = temp.path / "points.xyz";
    ASSERT_TRUE(engine::io::write_point_cloud(out_path, cloud.interface, engine::io::PointCloudFileFormat::xyz));

    ASSERT_TRUE(std::filesystem::exists(out_path));
    const auto detection = engine::io::detect_geometry_file(out_path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(info.point_cloud_format, engine::io::PointCloudFileFormat::xyz);
}

TEST(GeometryIO, ReadAndWriteGraph)
{
    TempDirectory temp;
    const auto path = temp.path / "graph.edgelist";
    write_file(path,
               "0 1\n"
               "1 2\n");

    engine::geometry::Graph graph;
    ASSERT_TRUE(engine::io::read_graph(path, graph.interface, engine::io::GraphFileFormat::edgelist));

    EXPECT_EQ(graph.interface.vertex_count(), 3U);
    EXPECT_EQ(graph.interface.edge_count(), 2U);

    const auto out_path = temp.path / "graph.ply";
    ASSERT_TRUE(engine::io::write_graph(out_path, graph.interface, engine::io::GraphFileFormat::ply));

    ASSERT_TRUE(std::filesystem::exists(out_path));
    const auto detection = engine::io::detect_geometry_file(out_path);
    ASSERT_TRUE(detection);
    const auto& info = detection.value();
    EXPECT_EQ(info.kind, engine::io::GeometryKind::graph);
    EXPECT_EQ(info.graph_format, engine::io::GraphFileFormat::ply);
}

TEST(GeometryIO, ReadGraphPlyWithEdgeScalars)
{
    TempDirectory temp;
    const auto path = temp.path / "graph_weighted.ply";
    write_file(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 3\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "element edge 2\n"
               "property float weight\n"
               "property list uchar int vertex_indices\n"
               "property float reliability\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "0 1 0\n"
               "0.5 2 0 1 1.0\n"
               "0.25 2 1 2 0.5\n");

    engine::geometry::Graph graph;
    ASSERT_TRUE(engine::io::read_graph(path, graph.interface, engine::io::GraphFileFormat::ply));

    EXPECT_EQ(graph.interface.vertex_count(), 3U);
    EXPECT_EQ(graph.interface.edge_count(), 2U);

    std::set<std::pair<std::size_t, std::size_t>> edges;
    for (const auto edge : graph.interface.edges())
    {
        if (graph.interface.is_deleted(edge))
        {
            continue;
        }
        const auto a = graph.interface.vertex(edge, 0).index();
        const auto b = graph.interface.vertex(edge, 1).index();
        edges.emplace(std::min(a, b), std::max(a, b));
    }

    const std::set<std::pair<std::size_t, std::size_t>> expected{{0, 1}, {1, 2}};
    EXPECT_EQ(edges, expected);
}

TEST(GeometryIO, ReadMeshMissingFileReturnsFileNotFound)
{
    engine::geometry::Mesh mesh;
    const auto result = engine::io::read_mesh("/nonexistent/path.obj", mesh.interface);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), engine::io::GeometryIoError::file_not_found);
}

TEST(GeometryIO, ReadMeshInvalidDataReturnsInvalidArgument)
{
    TempDirectory temp;
    const auto path = temp.path / "invalid.obj";
    write_file(path, "f 1 2\n");

    engine::geometry::Mesh mesh;
    const auto result = engine::io::read_mesh(path, mesh.interface, engine::io::MeshFileFormat::obj);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), engine::io::GeometryIoError::invalid_argument);
}

TEST(GeometryIO, ReadMeshWithoutGeometryReturnsInvalidArgument)
{
    TempDirectory temp;
    const auto path = temp.path / "empty_geometry.obj";
    write_file(path, "# empty file\n");

    engine::geometry::Mesh mesh;
    const auto result = engine::io::read_mesh(path, mesh.interface, engine::io::MeshFileFormat::obj);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), engine::io::GeometryIoError::invalid_argument);
    const auto& message = result.error().message();
    EXPECT_TRUE(message.find("does not define any vertices") != std::string::npos ||
        message.find("does not define any faces") != std::string::npos);
}

TEST(GeometryIO, WriteMeshWithInvalidParentReturnsIoFailure)
{
    TempDirectory temp;
    const auto blocker = temp.path / "blocked";
    write_file(blocker, "content");

    engine::geometry::Mesh mesh;
    const auto v0 = mesh.interface.add_vertex(engine::math::vec3{0.0F, 0.0F, 0.0F});
    const auto v1 = mesh.interface.add_vertex(engine::math::vec3{1.0F, 0.0F, 0.0F});
    const auto v2 = mesh.interface.add_vertex(engine::math::vec3{0.0F, 1.0F, 0.0F});
    const std::array<engine::geometry::VertexHandle, 3> face{v0, v1, v2};
    ASSERT_TRUE(mesh.interface.add_face(face));

    const auto result = engine::io::write_mesh(blocker / "triangle.obj", mesh.interface,
                                               engine::io::MeshFileFormat::obj);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), engine::io::GeometryIoError::io_failure);
}

TEST(GeometryIO, LoadMeshRejectsMissingPointer)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.obj";
    write_file(path, "v 0 0 0\n"
               "v 1 0 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n");

    engine::geometry::Mesh mesh;
    const auto result = engine::io::load_geometry(path, nullptr, nullptr, nullptr);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), engine::io::GeometryIoError::invalid_argument);

    const auto mesh_result = engine::io::load_geometry(path, &mesh.interface, nullptr, nullptr);
    ASSERT_TRUE(mesh_result);
    EXPECT_EQ(mesh_result.value().kind, engine::io::GeometryKind::mesh);
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
    ASSERT_TRUE(mesh_detection);
    EXPECT_EQ(mesh_detection.value().kind, engine::io::GeometryKind::mesh);
    EXPECT_EQ(mesh.interface.face_count(), 1U);

    const auto cloud_detection = engine::io::load_geometry(cloud_path, nullptr, &cloud.interface, nullptr);
    ASSERT_TRUE(cloud_detection);
    EXPECT_EQ(cloud_detection.value().kind, engine::io::GeometryKind::point_cloud);
    EXPECT_EQ(cloud.interface.vertex_count(), 1U);

    const auto graph_detection = engine::io::load_geometry(graph_path, nullptr, nullptr, &graph.interface);
    ASSERT_TRUE(graph_detection);
    EXPECT_EQ(graph_detection.value().kind, engine::io::GeometryKind::graph);
    EXPECT_EQ(graph.interface.edge_count(), 1U);

    const auto mesh_out = temp.path / "mesh_out.ply";
    const auto mesh_save = engine::io::save_geometry(mesh_out, &mesh.interface, nullptr, nullptr);
    ASSERT_TRUE(mesh_save);
    EXPECT_EQ(mesh_save.value().kind, engine::io::GeometryKind::mesh);
    ASSERT_TRUE(std::filesystem::exists(mesh_out));

    const auto cloud_out = temp.path / "cloud_out.ply";
    const auto cloud_save = engine::io::save_geometry(cloud_out, nullptr, &cloud.interface, nullptr);
    ASSERT_TRUE(cloud_save);
    EXPECT_EQ(cloud_save.value().kind, engine::io::GeometryKind::point_cloud);
    ASSERT_TRUE(std::filesystem::exists(cloud_out));

    const auto graph_out = temp.path / "graph_out.edgelist";
    const auto graph_save = engine::io::save_geometry(graph_out, nullptr, nullptr, &graph.interface);
    ASSERT_TRUE(graph_save);
    EXPECT_EQ(graph_save.value().kind, engine::io::GeometryKind::graph);
    ASSERT_TRUE(std::filesystem::exists(graph_out));
}