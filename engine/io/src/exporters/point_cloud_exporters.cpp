#include "engine/io/exporters/point_cloud.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include <filesystem>
#include <fstream>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::ensure_parent_directory;

        void write_point_cloud_ply(const std::filesystem::path& path, const geometry::PointCloudInterface& point_cloud)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PLY file for writing: " + path.string());
            }

            const std::size_t vertex_count = point_cloud.vertex_count();

            stream << "ply\n";
            stream << "format ascii 1.0\n";
            stream << "element vertex " << vertex_count << "\n";
            stream << "property float x\n";
            stream << "property float y\n";
            stream << "property float z\n";
            stream << "end_header\n";

            for (const auto v : point_cloud.vertices())
            {
                const auto& position = point_cloud.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }
        }

        void write_point_cloud_xyz(const std::filesystem::path& path, const geometry::PointCloudInterface& point_cloud)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open XYZ file for writing: " + path.string());
            }

            for (const auto v : point_cloud.vertices())
            {
                const auto& position = point_cloud.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }
        }

        void write_point_cloud_pcd(const std::filesystem::path& path, const geometry::PointCloudInterface& point_cloud)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PCD file for writing: " + path.string());
            }

            const std::size_t vertex_count = point_cloud.vertex_count();

            stream << "# .PCD v0.7 - Point Cloud Data file format\n";
            stream << "VERSION 0.7\n";
            stream << "FIELDS x y z\n";
            stream << "SIZE 4 4 4\n";
            stream << "TYPE F F F\n";
            stream << "COUNT 1 1 1\n";
            stream << "WIDTH " << vertex_count << "\n";
            stream << "HEIGHT 1\n";
            stream << "VIEWPOINT 0 0 0 1 0 0 0\n";
            stream << "POINTS " << vertex_count << "\n";
            stream << "DATA ascii\n";

            for (const auto v : point_cloud.vertices())
            {
                const auto& position = point_cloud.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }
        }
    } // namespace

    PointCloudFileFormat PlyPointCloudExporter::format() const noexcept
    {
        return PointCloudFileFormat::ply;
    }

    GeometryIoResult<void> PlyPointCloudExporter::export_point_cloud(const std::filesystem::path& path,
                                                                     const geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_point_cloud_ply(path, point_cloud); });
    }

    PointCloudFileFormat XyzPointCloudExporter::format() const noexcept
    {
        return PointCloudFileFormat::xyz;
    }

    GeometryIoResult<void> XyzPointCloudExporter::export_point_cloud(const std::filesystem::path& path,
                                                                     const geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_point_cloud_xyz(path, point_cloud); });
    }

    PointCloudFileFormat PcdPointCloudExporter::format() const noexcept
    {
        return PointCloudFileFormat::pcd;
    }

    GeometryIoResult<void> PcdPointCloudExporter::export_point_cloud(const std::filesystem::path& path,
                                                                     const geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_point_cloud_pcd(path, point_cloud); });
    }
} // namespace engine::io
