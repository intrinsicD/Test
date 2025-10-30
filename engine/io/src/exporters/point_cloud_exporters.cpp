#include "engine/io/exporters/point_cloud.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/math/vector.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::ensure_parent_directory;
        using engine::math::vec3;

        [[nodiscard]] std::string sanitise_property_name(std::string_view name)
        {
            const auto colon = name.find(':');
            std::string cleaned = colon == std::string_view::npos
                                      ? std::string{name}
                                      : std::string{name.substr(colon + 1U)};
            std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), [](unsigned char c)
            {
                if (std::isspace(c) != 0 || c == ':' || c == '/' || c == '\\')
                {
                    return '_';
                }
                return static_cast<char>(c);
            });
            if (cleaned.empty())
            {
                cleaned = "property";
            }
            return cleaned;
        }

        void write_point_cloud_ply(const geometry::PointCloudInterface& point_cloud,
                                   const std::filesystem::path& path)
        {
            ensure_parent_directory(path);
            std::ofstream output(path, std::ios::out | std::ios::trunc);
            if (!output)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PLY file for writing: " + path.string());
            }

            const auto normals = point_cloud.has_vertex_property("p:normal")
                                     ? point_cloud.get_vertex_property<vec3>("p:normal")
                                     : geometry::VertexProperty<vec3>();
            const auto colours = point_cloud.has_vertex_property("p:color")
                                     ? point_cloud.get_vertex_property<vec3>("p:color")
                                     : geometry::VertexProperty<vec3>();
            const auto alpha = point_cloud.has_vertex_property("p:alpha")
                                   ? point_cloud.get_vertex_property<float>("p:alpha")
                                   : geometry::VertexProperty<float>();

            std::vector<std::pair<std::string, geometry::VertexProperty<float>>> scalar_properties;
            const auto property_names = point_cloud.vertex_properties();
            for (const auto& name : property_names)
            {
                if (name == "v:point" || name == "v:deleted" || name == "p:normal" || name == "p:color"
                    || name == "p:alpha")
                {
                    continue;
                }

                if (auto property = point_cloud.get_vertex_property<float>(name); property)
                {
                    scalar_properties.emplace_back(sanitise_property_name(name), property);
                }
            }

            std::vector<geometry::VertexHandle> handles;
            handles.reserve(point_cloud.vertex_count());
            for (auto it = point_cloud.vertices_begin(); it != point_cloud.vertices_end(); ++it)
            {
                if (!point_cloud.is_valid(*it) || point_cloud.is_deleted(*it))
                {
                    continue;
                }
                handles.push_back(*it);
            }

            output << "ply\n";
            output << "format ascii 1.0\n";
            output << "element vertex " << handles.size() << '\n';
            output << "property float x\n";
            output << "property float y\n";
            output << "property float z\n";
            if (normals)
            {
                output << "property float nx\n";
                output << "property float ny\n";
                output << "property float nz\n";
            }
            if (colours)
            {
                output << "property float red\n";
                output << "property float green\n";
                output << "property float blue\n";
            }
            if (alpha)
            {
                output << "property float alpha\n";
            }
            for (const auto& [name, _] : scalar_properties)
            {
                output << "property float " << name << '\n';
            }
            output << "end_header\n";

            output << std::setprecision(7);
            for (const auto handle : handles)
            {
                const auto pos = point_cloud.position(handle);
                output << pos[0] << ' ' << pos[1] << ' ' << pos[2];
                if (normals)
                {
                    const auto n = normals[handle];
                    output << ' ' << n[0] << ' ' << n[1] << ' ' << n[2];
                }
                if (colours)
                {
                    const auto c = colours[handle];
                    output << ' ' << c[0] << ' ' << c[1] << ' ' << c[2];
                }
                if (alpha)
                {
                    output << ' ' << alpha[handle];
                }
                for (const auto& property : scalar_properties)
                {
                    output << ' ' << property.second[handle];
                }
                output << '\n';
            }

            if (!output)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "I/O failure while writing PLY point cloud: " + path.string());
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
        return detail::translate_io_exceptions(path, [&]() { write_point_cloud_ply(point_cloud, path); });
    }

    PointCloudFileFormat XyzPointCloudExporter::format() const noexcept
    {
        return PointCloudFileFormat::xyz;
    }

    GeometryIoResult<void> XyzPointCloudExporter::export_point_cloud(const std::filesystem::path& path,
                                                                     const geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]()
        {
            ensure_parent_directory(path);
            std::ofstream stream(path, std::ios::out | std::ios::trunc);
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open XYZ file for writing: " + path.string());
            }

            for (auto it = point_cloud.vertices_begin(); it != point_cloud.vertices_end(); ++it)
            {
                if (!point_cloud.is_valid(*it) || point_cloud.is_deleted(*it))
                {
                    continue;
                }
                const auto pos = point_cloud.position(*it);
                stream << pos[0] << ' ' << pos[1] << ' ' << pos[2] << '\n';
            }

            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "I/O failure while writing XYZ point cloud: " + path.string());
            }
        });
    }

    PointCloudFileFormat PcdPointCloudExporter::format() const noexcept
    {
        return PointCloudFileFormat::pcd;
    }

    GeometryIoResult<void> PcdPointCloudExporter::export_point_cloud(const std::filesystem::path& path,
                                                                     const geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]()
        {
            ensure_parent_directory(path);
            std::ofstream stream(path, std::ios::out | std::ios::trunc);
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PCD file for writing: " + path.string());
            }

            const auto vertex_count = point_cloud.vertex_count();
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

            for (auto it = point_cloud.vertices_begin(); it != point_cloud.vertices_end(); ++it)
            {
                if (!point_cloud.is_valid(*it) || point_cloud.is_deleted(*it))
                {
                    continue;
                }
                const auto pos = point_cloud.position(*it);
                stream << pos[0] << ' ' << pos[1] << ' ' << pos[2] << '\n';
            }

            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "I/O failure while writing PCD point cloud: " + path.string());
            }
        });
    }
} // namespace engine::io

