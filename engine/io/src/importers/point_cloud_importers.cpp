#include "engine/io/importers/point_cloud.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/math/vector.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::map_open_error;
        using engine::math::vec3;

        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        enum class PlyPropertySemantic
        {
            kPositionX,
            kPositionY,
            kPositionZ,
            kNormalX,
            kNormalY,
            kNormalZ,
            kColorR,
            kColorG,
            kColorB,
            kAlpha,
            kScalar,
        };

        enum class PlyFormat
        {
            kAscii,
            kBinaryLittleEndian,
            kBinaryBigEndian,
        };

        struct PlyProperty
        {
            PlyPropertySemantic semantic{PlyPropertySemantic::kScalar};
            std::size_t scalar_index{std::numeric_limits<std::size_t>::max()};
            std::string name;
        };

        struct PlyHeader
        {
            std::size_t vertex_count{0};
            PlyFormat format{PlyFormat::kAscii};
            bool has_normals{false};
            bool has_colors{false};
            bool has_alpha{false};
            std::vector<PlyProperty> properties;
            std::vector<std::string> scalar_names;
        };

        PlyPropertySemantic classify_property(std::string_view name, PlyHeader& header)
        {
            const std::string lower = to_lower(std::string{name});
            if (lower == "x" || lower == "position_x" || lower == "posx")
            {
                return PlyPropertySemantic::kPositionX;
            }
            if (lower == "y" || lower == "position_y" || lower == "posy")
            {
                return PlyPropertySemantic::kPositionY;
            }
            if (lower == "z" || lower == "position_z" || lower == "posz")
            {
                return PlyPropertySemantic::kPositionZ;
            }
            if (lower == "nx" || lower == "normal_x" || lower == "normx")
            {
                header.has_normals = true;
                return PlyPropertySemantic::kNormalX;
            }
            if (lower == "ny" || lower == "normal_y" || lower == "normy")
            {
                header.has_normals = true;
                return PlyPropertySemantic::kNormalY;
            }
            if (lower == "nz" || lower == "normal_z" || lower == "normz")
            {
                header.has_normals = true;
                return PlyPropertySemantic::kNormalZ;
            }
            if (lower == "red" || lower == "r" || lower == "diffuse_red")
            {
                header.has_colors = true;
                return PlyPropertySemantic::kColorR;
            }
            if (lower == "green" || lower == "g" || lower == "diffuse_green")
            {
                header.has_colors = true;
                return PlyPropertySemantic::kColorG;
            }
            if (lower == "blue" || lower == "b" || lower == "diffuse_blue")
            {
                header.has_colors = true;
                return PlyPropertySemantic::kColorB;
            }
            if (lower == "alpha" || lower == "a")
            {
                header.has_alpha = true;
                return PlyPropertySemantic::kAlpha;
            }

            header.scalar_names.emplace_back(std::string{name});
            return PlyPropertySemantic::kScalar;
        }

        PlyHeader parse_header(std::istream& stream, const std::filesystem::path& path)
        {
            PlyHeader header{};

            std::string line;
            if (!std::getline(stream, line))
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "PLY stream is empty: " + path.string());
            }
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (to_lower(line) != "ply")
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "Expected PLY signature at beginning of file: " + path.string());
            }

            bool in_vertex_section = false;
            while (std::getline(stream, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                if (line.empty())
                {
                    continue;
                }

                std::istringstream line_stream(line);
                std::string token;
                line_stream >> token;
                if (token == "comment")
                {
                    continue;
                }
                if (token == "format")
                {
                    std::string format;
                    std::string version;
                    line_stream >> format >> version;
                    const std::string lower = to_lower(format);
                    if (lower == "ascii")
                    {
                        header.format = PlyFormat::kAscii;
                    }
                    else if (lower == "binary_little_endian")
                    {
                        header.format = PlyFormat::kBinaryLittleEndian;
                    }
                    else if (lower == "binary_big_endian")
                    {
                        header.format = PlyFormat::kBinaryBigEndian;
                    }
                    else
                    {
                        throw GeometryIoException(GeometryIoError::unsupported_format,
                                                  "Unsupported PLY format '" + format + "' in " + path.string());
                    }
                    continue;
                }
                if (token == "element")
                {
                    std::string element_name;
                    std::size_t count = 0;
                    line_stream >> element_name >> count;
                    in_vertex_section = (element_name == "vertex");
                    if (in_vertex_section)
                    {
                        header.vertex_count = count;
                        header.properties.clear();
                        header.scalar_names.clear();
                    }
                    continue;
                }
                if (token == "property")
                {
                    std::string type;
                    line_stream >> type;
                    if (type == "list")
                    {
                        // Unsupported property type for point clouds.
                        continue;
                    }
                    std::string name_token;
                    line_stream >> name_token;
                    if (in_vertex_section)
                    {
                        PlyProperty property{};
                        property.semantic = classify_property(name_token, header);
                        if (property.semantic == PlyPropertySemantic::kScalar)
                        {
                            property.scalar_index = header.scalar_names.size() - 1U;
                        }
                        property.name = std::move(name_token);
                        header.properties.emplace_back(std::move(property));
                    }
                    continue;
                }
                if (token == "end_header")
                {
                    break;
                }
            }

            if (header.vertex_count == 0U)
            {
                return header;
            }
            if (header.properties.size() < 3U)
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "PLY vertex element does not describe positions in " + path.string());
            }

            return header;
        }

        void read_point_cloud_ply(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PLY file: " + path.string());
            }

            PlyHeader header = parse_header(stream, path);
            if (header.format != PlyFormat::kAscii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PLY point clouds are not supported: " + path.string());
            }

            point_cloud.clear();
            point_cloud.reserve(header.vertex_count);

            geometry::VertexProperty<vec3> normals_property;
            geometry::VertexProperty<vec3> colours_property;
            geometry::VertexProperty<float> alpha_property;
            if (header.has_normals)
            {
                normals_property = point_cloud.vertex_property<vec3>("p:normal", vec3{0.0F, 0.0F, 0.0F});
            }
            if (header.has_colors)
            {
                colours_property = point_cloud.vertex_property<vec3>("p:color", vec3{0.0F, 0.0F, 0.0F});
            }
            if (header.has_alpha)
            {
                alpha_property = point_cloud.vertex_property<float>("p:alpha", 1.0F);
            }

            std::vector<geometry::VertexProperty<float>> scalar_properties;
            scalar_properties.reserve(header.scalar_names.size());
            for (const auto& name : header.scalar_names)
            {
                scalar_properties.emplace_back(point_cloud.vertex_property<float>("p:" + name, 0.0F));
            }

            std::vector<float> scalar_values(header.scalar_names.size(), 0.0F);

            std::string line;
            std::size_t vertices_read = 0;
            while (vertices_read < header.vertex_count)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected end of file while reading PLY vertices: " + path.string());
                }

                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }

                std::istringstream line_stream(line);
                vec3 position{0.0F, 0.0F, 0.0F};
                vec3 normal{0.0F, 0.0F, 0.0F};
                vec3 colour{0.0F, 0.0F, 0.0F};
                float alpha = 1.0F;
                std::fill(scalar_values.begin(), scalar_values.end(), 0.0F);

                for (const PlyProperty& property : header.properties)
                {
                    double value = 0.0;
                    line_stream >> value;
                    if (!line_stream)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Malformed PLY vertex entry in " + path.string());
                    }

                    switch (property.semantic)
                    {
                    case PlyPropertySemantic::kPositionX:
                        position[0] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kPositionY:
                        position[1] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kPositionZ:
                        position[2] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kNormalX:
                        normal[0] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kNormalY:
                        normal[1] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kNormalZ:
                        normal[2] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kColorR:
                        colour[0] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kColorG:
                        colour[1] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kColorB:
                        colour[2] = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kAlpha:
                        alpha = static_cast<float>(value);
                        break;
                    case PlyPropertySemantic::kScalar:
                        if (property.scalar_index < scalar_values.size())
                        {
                            scalar_values[property.scalar_index] = static_cast<float>(value);
                        }
                        break;
                    }
                }

                const auto handle = point_cloud.add_vertex(position);
                if (header.has_normals && normals_property)
                {
                    normals_property[handle] = normal;
                }
                if (header.has_colors && colours_property)
                {
                    colours_property[handle] = colour;
                }
                if (header.has_alpha && alpha_property)
                {
                    alpha_property[handle] = alpha;
                }
                for (std::size_t index = 0; index < scalar_properties.size(); ++index)
                {
                    if (scalar_properties[index])
                    {
                        scalar_properties[index][handle] = scalar_values[index];
                    }
                }

                ++vertices_read;
            }
        }

        [[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix)
        {
            return value.substr(0, prefix.size()) == prefix;
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
                std::istringstream line_stream(line);
                float x = 0.0F;
                float y = 0.0F;
                float z = 0.0F;
                line_stream >> x >> y >> z;
                if (!line_stream)
                {
                    continue;
                }
                (void)point_cloud.add_vertex(vec3{x, y, z});
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
                std::istringstream line_stream(line);
                float x = 0.0F;
                float y = 0.0F;
                float z = 0.0F;
                line_stream >> x >> y >> z;
                if (!line_stream)
                {
                    continue;
                }
                (void)point_cloud.add_vertex(vec3{x, y, z});
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

