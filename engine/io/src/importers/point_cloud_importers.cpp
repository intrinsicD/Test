#include "engine/io/importers/point_cloud.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/math/vector.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::map_open_error;
        using engine::io::detail::tokenize;
        using engine::math::vec3;

        [[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix)
        {
            return value.substr(0, prefix.size()) == prefix;
        }

        void read_point_cloud_ply(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            const auto header_result = detail::inspect_ply_header(path);
            if (!header_result)
            {
                throw GeometryIoException(header_result.error().code(), header_result.error().message());
            }
            const auto header = header_result.value();
            if (!header.ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PLY point clouds are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PLY file: " + path.string());
            }

            std::string line;
            std::getline(stream, line); // ply
            while (std::getline(stream, line) && line != "end_header")
            {
            }

            point_cloud.clear();
            point_cloud.reserve(header.vertex_count);

            for (std::size_t i = 0; i < header.vertex_count; ++i)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(
                        GeometryIoError::invalid_argument,
                        "Unexpected end of file while reading PLY point cloud vertices: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY point cloud vertex without 3 coordinates in file: " + path.string());
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                const auto vh = point_cloud.add_vertex(vec3{x, y, z});
                (void)vh;
            }
        }

        void read_point_cloud_xyz(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open XYZ file: " + path.string());
            }

            point_cloud.clear();

            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    continue;
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                const auto vh = point_cloud.add_vertex(vec3{x, y, z});
                (void)vh;
            }
        }

        void read_point_cloud_pcd(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PCD file: " + path.string());
            }

            std::string line;
            std::size_t point_count{0};
            bool ascii{false};
            while (std::getline(stream, line))
            {
                if (line.empty())
                {
                    continue;
                }
                auto lower = detail::to_lower(line);
                if (starts_with(lower, "#"))
                {
                    continue;
                }
                if (starts_with(lower, "fields"))
                {
                    if (lower.find("x") == std::string::npos || lower.find("y") == std::string::npos
                        || lower.find("z") == std::string::npos)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "PCD file missing XYZ fields: " + path.string());
                    }
                }
                else if (starts_with(lower, "points"))
                {
                    point_count = static_cast<std::size_t>(
                        std::stoull(lower.substr(lower.find_first_of("0123456789"))));
                }
                else if (starts_with(lower, "data"))
                {
                    ascii = lower.find("ascii") != std::string::npos;
                    break;
                }
            }

            if (!ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PCD files are not supported: " + path.string());
            }

            point_cloud.clear();
            point_cloud.reserve(point_count);

            while (std::getline(stream, line))
            {
                if (line.empty())
                {
                    continue;
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    continue;
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                const auto vh = point_cloud.add_vertex(vec3{x, y, z});
                (void)vh;
            }
        }
    } // namespace

    PointCloudFileFormat PlyPointCloudImporter::format() const noexcept
    {
        return PointCloudFileFormat::ply;
    }

    GeometryIoResult<void> PlyPointCloudImporter::import(const std::filesystem::path& path,
                                                         geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_point_cloud_ply(path, point_cloud); });
    }

    PointCloudFileFormat XyzPointCloudImporter::format() const noexcept
    {
        return PointCloudFileFormat::xyz;
    }

    GeometryIoResult<void> XyzPointCloudImporter::import(const std::filesystem::path& path,
                                                         geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_point_cloud_xyz(path, point_cloud); });
    }

    PointCloudFileFormat PcdPointCloudImporter::format() const noexcept
    {
        return PointCloudFileFormat::pcd;
    }

    GeometryIoResult<void> PcdPointCloudImporter::import(const std::filesystem::path& path,
                                                         geometry::PointCloudInterface& point_cloud) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_point_cloud_pcd(path, point_cloud); });
    }
} // namespace engine::io
