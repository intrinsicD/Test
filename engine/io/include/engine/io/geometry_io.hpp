#pragma once

#include "engine/io/api.hpp"

#include "engine/geometry/graph/graph.hpp"
#include "engine/geometry/mesh/halfedge_mesh.hpp"
#include "engine/geometry/point_cloud/point_cloud.hpp"

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>

namespace engine::io
{
    enum class GeometryKind : std::uint8_t
    {
        unknown = 0U,
        mesh,
        point_cloud,
        graph
    };

    enum class MeshFileFormat : std::uint8_t
    {
        unknown = 0U,
        obj,
        ply,
        off,
        stl
    };

    enum class PointCloudFileFormat : std::uint8_t
    {
        unknown = 0U,
        ply,
        xyz,
        pcd
    };

    enum class GraphFileFormat : std::uint8_t
    {
        unknown = 0U,
        edgelist,
        ply
    };

    struct GeometryDetectionResult
    {
        GeometryKind kind{GeometryKind::unknown};
        MeshFileFormat mesh_format{MeshFileFormat::unknown};
        PointCloudFileFormat point_cloud_format{PointCloudFileFormat::unknown};
        GraphFileFormat graph_format{GraphFileFormat::unknown};
        std::string format_hint{};
    };

    [[nodiscard]] ENGINE_IO_API GeometryDetectionResult detect_geometry_file(const std::filesystem::path& path);

    ENGINE_IO_API GeometryDetectionResult load_geometry(const std::filesystem::path& path,
                                                        geometry::MeshInterface* mesh,
                                                        geometry::PointCloudInterface* point_cloud,
                                                        geometry::GraphInterface* graph);

    ENGINE_IO_API GeometryDetectionResult save_geometry(const std::filesystem::path& path,
                                                        const geometry::MeshInterface* mesh,
                                                        const geometry::PointCloudInterface* point_cloud,
                                                        const geometry::GraphInterface* graph);

    ENGINE_IO_API void read_mesh(const std::filesystem::path& path,
                                 geometry::MeshInterface& mesh,
                                 MeshFileFormat format = MeshFileFormat::unknown);

    ENGINE_IO_API void write_mesh(const std::filesystem::path& path,
                                  const geometry::MeshInterface& mesh,
                                  MeshFileFormat format = MeshFileFormat::unknown);

    ENGINE_IO_API void read_point_cloud(const std::filesystem::path& path,
                                        geometry::PointCloudInterface& point_cloud,
                                        PointCloudFileFormat format = PointCloudFileFormat::unknown);

    ENGINE_IO_API void write_point_cloud(const std::filesystem::path& path,
                                         const geometry::PointCloudInterface& point_cloud,
                                         PointCloudFileFormat format = PointCloudFileFormat::unknown);

    ENGINE_IO_API void read_graph(const std::filesystem::path& path,
                                  geometry::GraphInterface& graph,
                                  GraphFileFormat format = GraphFileFormat::unknown);

    ENGINE_IO_API void write_graph(const std::filesystem::path& path,
                                   const geometry::GraphInterface& graph,
                                   GraphFileFormat format = GraphFileFormat::unknown);

    ENGINE_IO_API std::ostream& operator<<(std::ostream& stream, GeometryKind kind);
    ENGINE_IO_API std::ostream& operator<<(std::ostream& stream, MeshFileFormat format);
    ENGINE_IO_API std::ostream& operator<<(std::ostream& stream, PointCloudFileFormat format);
    ENGINE_IO_API std::ostream& operator<<(std::ostream& stream, GraphFileFormat format);
} // namespace engine::io

