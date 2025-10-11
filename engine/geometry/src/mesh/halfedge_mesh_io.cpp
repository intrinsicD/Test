#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace engine::geometry::mesh
{
    namespace
    {
        [[nodiscard]] IOFlags::Format resolve_format(IOFlags::Format requested,
                                                     const std::filesystem::path& path)
        {
            if (requested != IOFlags::Format::kAuto)
            {
                return requested;
            }

            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (extension == ".obj")
            {
                return IOFlags::Format::kOBJ;
            }

            throw std::runtime_error("Unsupported mesh format for file \"" + path.string() + "\"");
        }

        [[nodiscard]] std::size_t parse_obj_vertex_index(std::string_view token,
                                                         std::size_t vertex_count,
                                                         std::size_t line_number,
                                                         const std::filesystem::path& path)
        {
            const auto slash = token.find('/');
            const std::string_view index_token = token.substr(0, slash);
            if (index_token.empty())
            {
                throw std::runtime_error("Missing vertex index in OBJ face on line " + std::to_string(line_number) +
                                         " of \"" + path.string() + "\"");
            }

            int value = 0;
            const auto parsed = std::from_chars(index_token.data(), index_token.data() + index_token.size(), value);
            if (parsed.ec != std::errc{})
            {
                throw std::runtime_error("Invalid vertex index \"" + std::string(index_token) + "\" in OBJ face on line " +
                                         std::to_string(line_number) + " of \"" + path.string() + "\"");
            }

            std::ptrdiff_t resolved = 0;
            if (value > 0)
            {
                resolved = static_cast<std::ptrdiff_t>(value) - 1;
            }
            else if (value < 0)
            {
                resolved = static_cast<std::ptrdiff_t>(vertex_count) + value;
            }
            else
            {
                throw std::runtime_error("OBJ indices are 1-based; encountered zero on line " + std::to_string(line_number) +
                                         " of \"" + path.string() + "\"");
            }

            if (resolved < 0 || resolved >= static_cast<std::ptrdiff_t>(vertex_count))
            {
                throw std::runtime_error("OBJ face on line " + std::to_string(line_number) + " of \"" + path.string() +
                                         "\" references vertex " + std::to_string(value) +
                                         " outside the available range");
            }

            return static_cast<std::size_t>(resolved);
        }

        void read_obj(HalfedgeMeshInterface& mesh, const std::filesystem::path& path)
        {
            std::ifstream stream(path);
            if (!stream.is_open())
            {
                throw std::system_error(std::error_code(errno, std::generic_category()),
                                        "Failed to open OBJ file \"" + path.string() + "\" for reading");
            }

            std::vector<math::vec3> positions;
            std::vector<std::vector<std::size_t>> faces;

            std::string line;
            std::size_t line_number = 0;
            while (std::getline(stream, line))
            {
                ++line_number;
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                std::istringstream line_stream(line);
                std::string keyword;
                line_stream >> keyword;
                if (keyword.empty() || keyword[0] == '#')
                {
                    continue;
                }

                if (keyword == "v")
                {
                    float x = 0.0F;
                    float y = 0.0F;
                    float z = 0.0F;
                    if (!(line_stream >> x >> y >> z))
                    {
                        throw std::runtime_error("Invalid vertex specification on line " + std::to_string(line_number) +
                                                 " of \"" + path.string() + "\"");
                    }

                    // Consume optional homogeneous coordinate if present.
                    float w = 1.0F;
                    line_stream >> w;

                    positions.emplace_back(x, y, z);
                    continue;
                }

                if (keyword == "f")
                {
                    if (positions.empty())
                    {
                        throw std::runtime_error("Encountered face before any vertices in OBJ file \"" + path.string() +
                                                 "\"");
                    }

                    std::vector<std::size_t> polygon;
                    polygon.reserve(4);

                    std::string vertex_token;
                    while (line_stream >> vertex_token)
                    {
                        const std::size_t index = parse_obj_vertex_index(vertex_token,
                                                                          positions.size(),
                                                                          line_number,
                                                                          path);
                        polygon.push_back(index);
                    }

                    if (polygon.size() < 3)
                    {
                        throw std::runtime_error("OBJ face on line " + std::to_string(line_number) + " of \"" + path.string() +
                                                 "\" contains fewer than three vertices");
                    }

                    faces.emplace_back(std::move(polygon));
                    continue;
                }
            }

            std::size_t edge_budget = 0;
            for (const auto& polygon : faces)
            {
                edge_budget += polygon.size();
            }

            mesh.clear();
            mesh.reserve(positions.size(), edge_budget, faces.size());

            std::vector<VertexHandle> vertex_handles;
            vertex_handles.reserve(positions.size());
            for (const auto& position : positions)
            {
                vertex_handles.push_back(mesh.add_vertex(position));
            }

            std::vector<VertexHandle> face_vertices;
            for (const auto& polygon : faces)
            {
                face_vertices.clear();
                face_vertices.reserve(polygon.size());
                for (const auto index : polygon)
                {
                    face_vertices.push_back(vertex_handles[index]);
                }

                if (!mesh.add_face(face_vertices))
                {
                    throw std::runtime_error("Failed to construct face while importing OBJ file \"" + path.string() +
                                             "\"; the polygon may be non-manifold");
                }
            }
        }

        void write_obj(const HalfedgeMeshInterface& mesh,
                       const std::filesystem::path& path,
                       const IOFlags& flags)
        {
            std::ofstream stream(path);
            if (!stream.is_open())
            {
                throw std::system_error(std::error_code(errno, std::generic_category()),
                                        "Failed to open OBJ file \"" + path.string() + "\" for writing");
            }

            const int precision = std::max(1, flags.precision);
            stream.setf(std::ios::fmtflags(0), std::ios::floatfield);
            stream << std::setprecision(precision);

            if (flags.include_header_comment)
            {
                stream << "# Generated by engine::geometry halfedge mesh exporter\n";
            }

            const auto vertex_capacity = mesh.vertices_size();
            std::vector<std::size_t> index_map(vertex_capacity, std::numeric_limits<std::size_t>::max());
            std::size_t next_index = 1;

            for (auto vertex : mesh.vertices())
            {
                if (mesh.is_deleted(vertex))
                {
                    continue;
                }

                index_map[vertex.index()] = next_index++;
                const auto& position = mesh.position(vertex);
                stream << "v " << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }

            std::vector<std::size_t> polygon_indices;
            for (auto face : mesh.faces())
            {
                if (mesh.is_deleted(face))
                {
                    continue;
                }

                polygon_indices.clear();
                auto circulator = mesh.vertices(face);
                const auto end = circulator;
                if (circulator)
                {
                    do
                    {
                        const auto vertex = *circulator;
                        const auto mapped = index_map[vertex.index()];
                        if (mapped == std::numeric_limits<std::size_t>::max())
                        {
                            throw std::runtime_error("Cannot export OBJ because a face references a deleted vertex in \"" +
                                                     path.string() + "\"");
                        }
                        polygon_indices.push_back(mapped);
                    }
                    while (++circulator != end);
                }

                if (polygon_indices.size() < 3)
                {
                    continue;
                }

                stream << 'f';
                for (const auto index : polygon_indices)
                {
                    stream << ' ' << index;
                }
                stream << '\n';
            }

            stream.flush();
            if (!stream)
            {
                throw std::runtime_error("Failed while writing OBJ file \"" + path.string() + "\"");
            }
        }
    } // namespace

    void read(HalfedgeMeshInterface& mesh, const std::filesystem::path& path)
    {
        const IOFlags::Format format = resolve_format(IOFlags::Format::kAuto, path);
        switch (format)
        {
            case IOFlags::Format::kOBJ:
                read_obj(mesh, path);
                break;
            default:
                throw std::runtime_error("Unsupported mesh format for file \"" + path.string() + "\"");
        }
    }

    void write(const HalfedgeMeshInterface& mesh,
               const std::filesystem::path& path,
               const IOFlags& flags)
    {
        const IOFlags::Format format = resolve_format(flags.format, path);
        switch (format)
        {
            case IOFlags::Format::kOBJ:
                write_obj(mesh, path, flags);
                break;
            default:
                throw std::runtime_error("Unsupported mesh format for file \"" + path.string() + "\"");
        }
    }
} // namespace engine::geometry::mesh
