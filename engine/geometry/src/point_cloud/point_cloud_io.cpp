#include "engine/geometry/point_cloud/point_cloud.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace engine::geometry::point_cloud
{
    namespace
    {
        constexpr std::string_view kNormalProperty{"p:normal"};
        constexpr std::string_view kColorProperty{"p:color"};
        constexpr std::string_view kAlphaProperty{"p:alpha"};

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

        struct PlyProperty
        {
            PlyPropertySemantic semantic{PlyPropertySemantic::kScalar};
            std::size_t scalar_index{std::numeric_limits<std::size_t>::max()};
            std::string name;
        };

        struct PlyHeader
        {
            std::size_t vertex_count{0};
            bool binary{false};
            bool has_normals{false};
            bool has_colors{false};
            bool has_alpha{false};
            std::vector<PlyProperty> properties;
            std::vector<std::string> scalar_names;
        };

        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

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
            const auto index = header.scalar_names.size();
            header.scalar_names.emplace_back(std::string{name});
            return PlyPropertySemantic::kScalar;
        }

        PlyHeader parse_header(std::istream& stream)
        {
            PlyHeader header{};

            std::string line;
            if (!std::getline(stream, line))
            {
                throw std::runtime_error("PLY stream is empty");
            }
            if (to_lower(line) != "ply")
            {
                throw std::runtime_error("Expected PLY signature at beginning of file");
            }

            bool in_vertex_section = false;
            while (std::getline(stream, line))
            {
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
                    if (format != "ascii")
                    {
                        header.binary = true;
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

            if (header.binary)
            {
                throw std::runtime_error("Binary PLY files are not supported yet");
            }

            if (header.vertex_count == 0U)
            {
                return header;
            }

            if (header.properties.size() < 3U)
            {
                throw std::runtime_error("PLY vertex element does not describe positions");
            }

            return header;
        }

        [[nodiscard]] std::string sanitise_property_name(std::string_view name)
        {
            const auto colon = name.find(':');
            std::string cleaned = colon == std::string_view::npos ? std::string{name} : std::string{name.substr(colon + 1U)};
            std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), [](unsigned char c) {
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

        void read_ply(PointCloudInterface& cloud, std::istream& stream)
        {
            const PlyHeader header = parse_header(stream);

            cloud.clear();
            cloud.reserve(header.vertex_count);

            VertexProperty<math::vec3> normals_property;
            VertexProperty<math::vec3> colors_property;
            VertexProperty<float> alpha_property;
            if (header.has_normals)
            {
                normals_property = cloud.vertex_property<math::vec3>(std::string{kNormalProperty}, math::vec3{0.0F});
            }
            if (header.has_colors)
            {
                colors_property = cloud.vertex_property<math::vec3>(std::string{kColorProperty}, math::vec3{0.0F});
            }
            if (header.has_alpha)
            {
                alpha_property = cloud.vertex_property<float>(std::string{kAlphaProperty}, 1.0F);
            }

            std::vector<VertexProperty<float>> scalar_properties;
            scalar_properties.reserve(header.scalar_names.size());
            for (const auto& name : header.scalar_names)
            {
                scalar_properties.emplace_back(cloud.vertex_property<float>("p:" + name, 0.0F));
            }

            std::vector<float> scalar_values(header.scalar_names.size(), 0.0F);

            for (std::size_t i = 0; i < header.vertex_count; ++i)
            {
                math::vec3 position{0.0F, 0.0F, 0.0F};
                math::vec3 normal{0.0F, 0.0F, 0.0F};
                math::vec3 colour{0.0F, 0.0F, 0.0F};
                float alpha = 1.0F;

                for (float& value : scalar_values)
                {
                    value = 0.0F;
                }

                for (const PlyProperty& property : header.properties)
                {
                    double value = 0.0;
                    stream >> value;
                    if (!stream)
                    {
                        throw std::runtime_error("Unexpected end of PLY vertex data");
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

                const VertexHandle handle = cloud.add_vertex(position);
                if (header.has_normals && normals_property)
                {
                    normals_property[handle] = normal;
                }
                if (header.has_colors && colors_property)
                {
                    colors_property[handle] = colour;
                }
                if (header.has_alpha && alpha_property)
                {
                    alpha_property[handle] = alpha;
                }
                for (std::size_t scalar_index = 0; scalar_index < scalar_properties.size(); ++scalar_index)
                {
                    if (scalar_properties[scalar_index])
                    {
                        scalar_properties[scalar_index][handle] = scalar_values[scalar_index];
                    }
                }
            }
        }

        IOFlags::Format resolve_format(const IOFlags& flags, const std::filesystem::path& path)
        {
            if (flags.format != IOFlags::Format::kAuto)
            {
                return flags.format;
            }

            const auto extension = to_lower(path.extension().string());
            if (extension == ".ply")
            {
                return IOFlags::Format::kPLY;
            }
            throw std::runtime_error("Unable to infer output format from file extension");
        }

        void ensure_parent_directory(const std::filesystem::path& path)
        {
            const auto parent = path.parent_path();
            if (!parent.empty())
            {
                std::error_code ec;
                std::filesystem::create_directories(parent, ec);
            }
        }

        void write_ply(const PointCloudInterface& cloud,
                       const std::filesystem::path& path,
                       const IOFlags& flags)
        {
            if (flags.binary)
            {
                throw std::runtime_error("Binary PLY output is not implemented yet");
            }

            ensure_parent_directory(path);

            std::ofstream output(path, std::ios::out | std::ios::trunc);
            if (!output)
            {
                throw std::runtime_error("Failed to open PLY file for writing");
            }

            const auto normals = flags.export_normals && cloud.has_vertex_property(flags.normal_property)
                                     ? cloud.get_vertex_property<math::vec3>(flags.normal_property)
                                     : VertexProperty<math::vec3>();
            const auto colours = flags.export_colors && cloud.has_vertex_property(flags.color_property)
                                     ? cloud.get_vertex_property<math::vec3>(flags.color_property)
                                     : VertexProperty<math::vec3>();
            const auto alpha = flags.export_alpha && cloud.has_vertex_property(flags.alpha_property)
                                   ? cloud.get_vertex_property<float>(flags.alpha_property)
                                   : VertexProperty<float>();

            std::vector<std::pair<std::string, VertexProperty<float>>> scalar_properties;
            if (flags.export_custom_scalar_properties)
            {
                const auto names = cloud.vertex_properties();
                for (const auto& name : names)
                {
                    if (name == "v:point" || name == "v:deleted" || name == flags.normal_property ||
                        name == flags.color_property || name == flags.alpha_property)
                    {
                        continue;
                    }

                    if (auto property = cloud.get_vertex_property<float>(name); property)
                    {
                        scalar_properties.emplace_back(name, property);
                    }
                }
            }

            std::vector<VertexHandle> handles;
            handles.reserve(cloud.vertex_count());
            for (auto it = cloud.vertices_begin(); it != cloud.vertices_end(); ++it)
            {
                if (!cloud.is_valid(*it) || cloud.is_deleted(*it))
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
                output << "property float " << sanitise_property_name(name) << '\n';
            }
            output << "end_header\n";
            output << std::setprecision(7);

            for (const VertexHandle handle : handles)
            {
                const auto pos = cloud.position(handle);
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
        }
    } // namespace

    void read(PointCloudInterface& cloud, const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::in);
        if (!input)
        {
            throw std::runtime_error("Failed to open PLY file for reading");
        }

        read_ply(cloud, input);
    }

    void write(const PointCloudInterface& cloud, const std::filesystem::path& path, const IOFlags& flags)
    {
        const auto format = resolve_format(flags, path);
        switch (format)
        {
            case IOFlags::Format::kPLY:
                write_ply(cloud, path, flags);
                break;
        }
    }
} // namespace engine::geometry::point_cloud
