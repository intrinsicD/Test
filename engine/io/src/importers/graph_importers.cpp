#include "engine/io/importers/graph.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/geometry/graph/graph.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::map_open_error;
        using engine::io::detail::tokenize;
        using engine::math::vec3;

        [[nodiscard]] detail::PlyHeaderInfo read_ply_header_or_throw(const std::filesystem::path& path)
        {
            auto header_result = detail::inspect_ply_header(path);
            if (!header_result)
            {
                const auto code = header_result.error();
                throw GeometryIoException(code.code(), std::string{code.message()});
            }

            return header_result.value();
        }

        float parse_float(std::string_view value, const std::filesystem::path& path, std::string_view context)
        {
            try
            {
                return std::stof(std::string{value});
            }
            catch (const std::exception&)
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          std::string{context} + " while reading " + path.string());
            }
        }

        std::size_t parse_index(std::string_view value, const std::filesystem::path& path, std::string_view context)
        {
            try
            {
                return static_cast<std::size_t>(std::stoull(std::string{value}));
            }
            catch (const std::exception&)
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          std::string{context} + " while reading " + path.string());
            }
        }

        enum class PlySection
        {
            none,
            vertex,
            face,
            edge
        };

        enum class EdgePropertySemantic
        {
            vertex_list,
            vertex_index,
            skip_list,
            skip_scalar
        };

        [[nodiscard]] bool is_vertex_reference_name(std::string_view name)
        {
            const auto lower = detail::to_lower(std::string{name});
            if (lower == "vertex1" || lower == "vertex2" || lower == "vertex_index"
                || lower == "vertex_index_0" || lower == "vertex_index_1" || lower == "source" || lower == "target"
                || lower == "from" || lower == "to" || lower == "v1" || lower == "v2")
            {
                return true;
            }

            return lower.find("vertex") != std::string::npos;
        }

        [[nodiscard]] EdgePropertySemantic classify_edge_property(const std::vector<std::string>& tokens)
        {
            if (tokens.size() < 3U)
            {
                return EdgePropertySemantic::skip_scalar;
            }

            const auto type = detail::to_lower(tokens[1]);
            if (type == "list")
            {
                const auto name = tokens.back();
                if (is_vertex_reference_name(name))
                {
                    return EdgePropertySemantic::vertex_list;
                }

                return EdgePropertySemantic::skip_list;
            }

            const auto name = tokens.back();
            if (is_vertex_reference_name(name))
            {
                return EdgePropertySemantic::vertex_index;
            }

            return EdgePropertySemantic::skip_scalar;
        }

        void read_graph_edgelist(const std::filesystem::path& path, geometry::GraphInterface& graph)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open edge list file: " + path.string());
            }

            std::vector<vec3> positions;
            std::vector<std::pair<std::size_t, std::size_t>> edges;
            positions.reserve(1024);
            edges.reserve(2048);

            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }
                auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }

                const auto head = tokens[0];
                if (head == "graph")
                {
                    continue;
                }
                if (head == "v")
                {
                    if (tokens.size() < 4)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Malformed vertex line in edge list: " + path.string());
                    }
                    vec3 p{0.0F, 0.0F, 0.0F};
                    p[0] = std::stof(tokens[1]);
                    p[1] = std::stof(tokens[2]);
                    p[2] = std::stof(tokens[3]);
                    positions.emplace_back(p);
                    continue;
                }
                if (head == "e")
                {
                    if (tokens.size() < 3)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Malformed edge line in edge list: " + path.string());
                    }
                    const std::size_t a = static_cast<std::size_t>(std::stoull(tokens[1]));
                    const std::size_t b = static_cast<std::size_t>(std::stoull(tokens[2]));
                    edges.emplace_back(a, b);
                    continue;
                }

                if (tokens.size() >= 2)
                {
                    try
                    {
                        const std::size_t a = static_cast<std::size_t>(std::stoull(tokens[0]));
                        const std::size_t b = static_cast<std::size_t>(std::stoull(tokens[1]));
                        edges.emplace_back(a, b);
                    }
                    catch (...)
                    {
                        // Ignore non-numeric lines.
                    }
                }
            }

            graph.clear();

            if (!positions.empty())
            {
                graph.reserve(positions.size(), edges.size());
                std::vector<geometry::VertexHandle> verts;
                verts.reserve(positions.size());
                for (const auto& p : positions)
                {
                    verts.push_back(graph.add_vertex(p));
                }
                for (const auto& [a, b] : edges)
                {
                    if (a >= verts.size() || b >= verts.size())
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Edge references vertex outside range while reading edge list: "
                                                  + path.string());
                    }
                    (void)graph.add_edge(verts[a], verts[b]);
                }
            }
            else
            {
                std::size_t max_index = 0;
                for (const auto& [a, b] : edges)
                {
                    max_index = std::max(max_index, std::max(a, b));
                }
                graph.reserve(max_index + 1U, edges.size());
                std::vector<geometry::VertexHandle> verts(max_index + 1U);
                for (std::size_t i = 0; i <= max_index; ++i)
                {
                    verts[i] = graph.add_vertex(vec3{0.0F, 0.0F, 0.0F});
                }
                for (const auto& [a, b] : edges)
                {
                    (void)graph.add_edge(verts[a], verts[b]);
                }
            }
        }

        void read_graph_ply(const std::filesystem::path& path, geometry::GraphInterface& graph)
        {
            const auto header = read_ply_header_or_throw(path);
            if (!header.ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PLY graphs are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PLY file: " + path.string());
            }

            std::string line;
            PlySection section{PlySection::none};
            std::vector<EdgePropertySemantic> edge_properties;
            bool found_header_end = false;
            while (std::getline(stream, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }

                const auto keyword = detail::to_lower(tokens[0]);
                if (keyword == "end_header")
                {
                    found_header_end = true;
                    break;
                }

                if (keyword == "comment" || keyword == "obj_info")
                {
                    continue;
                }

                if (keyword == "element")
                {
                    if (tokens.size() >= 2U)
                    {
                        const auto name = detail::to_lower(tokens[1]);
                        if (name == "vertex")
                        {
                            section = PlySection::vertex;
                        }
                        else if (name == "edge")
                        {
                            section = PlySection::edge;
                        }
                        else
                        {
                            section = PlySection::none;
                        }
                    }
                    continue;
                }

                if (keyword == "property" && section == PlySection::edge)
                {
                    edge_properties.push_back(classify_edge_property(tokens));
                }
            }

            if (!found_header_end)
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "PLY file missing end_header marker: " + path.string());
            }

            const bool has_vertex_semantics = std::any_of(edge_properties.begin(),
                                                          edge_properties.end(),
                                                          [](EdgePropertySemantic semantic)
                                                          {
                                                              return semantic == EdgePropertySemantic::vertex_index
                                                                     || semantic == EdgePropertySemantic::vertex_list;
                                                          });
            if (!has_vertex_semantics)
            {
                edge_properties.clear();
            }

            graph.clear();

            if (header.vertex_count > 0 || header.edge_count > 0)
            {
                graph.reserve(header.vertex_count, header.edge_count);
            }

            std::vector<geometry::VertexHandle> vertices;
            vertices.reserve(header.vertex_count);

            std::size_t read_vertices = 0;
            while (read_vertices < header.vertex_count)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY file ended before reading all vertices: " + path.string());
                }

                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }

                if (tokens.size() < 3)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Vertex entry in PLY graph is incomplete: " + path.string());
                }

                vec3 position{0.0F, 0.0F, 0.0F};
                position[0] = parse_float(tokens[0], path, "Invalid vertex coordinate");
                position[1] = parse_float(tokens[1], path, "Invalid vertex coordinate");
                position[2] = parse_float(tokens[2], path, "Invalid vertex coordinate");
                vertices.push_back(graph.add_vertex(position));
                ++read_vertices;
            }

            std::vector<std::pair<std::size_t, std::size_t>> edges;
            edges.reserve(header.edge_count);

            std::size_t read_edges = 0;
            while (read_edges < header.edge_count)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY file ended before reading all edges: " + path.string());
                }

                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }

                if (detail::to_lower(tokens[0]) == "comment")
                {
                    continue;
                }

                if (!edge_properties.empty())
                {
                    std::istringstream line_stream(line);
                    std::vector<std::size_t> indices;
                    indices.reserve(4);

                    for (const auto semantic : edge_properties)
                    {
                        switch (semantic)
                        {
                        case EdgePropertySemantic::vertex_list:
                        {
                            std::size_t count = 0;
                            if (!(line_stream >> count))
                            {
                                throw GeometryIoException(GeometryIoError::invalid_argument,
                                                          "Malformed PLY edge entry in " + path.string());
                            }
                            if (count < 2)
                            {
                                throw GeometryIoException(GeometryIoError::invalid_argument,
                                                          "PLY edge entry must reference at least two vertices: "
                                                              + path.string());
                            }
                            for (std::size_t i = 0; i < count; ++i)
                            {
                                std::size_t index = 0;
                                if (!(line_stream >> index))
                                {
                                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                                              "Malformed PLY edge entry in " + path.string());
                                }
                                indices.push_back(index);
                            }
                            break;
                        }
                        case EdgePropertySemantic::vertex_index:
                        {
                            std::size_t index = 0;
                            if (!(line_stream >> index))
                            {
                                throw GeometryIoException(GeometryIoError::invalid_argument,
                                                          "Malformed PLY edge entry in " + path.string());
                            }
                            indices.push_back(index);
                            break;
                        }
                        case EdgePropertySemantic::skip_list:
                        {
                            std::size_t count = 0;
                            if (!(line_stream >> count))
                            {
                                throw GeometryIoException(GeometryIoError::invalid_argument,
                                                          "Malformed PLY edge entry in " + path.string());
                            }
                            for (std::size_t i = 0; i < count; ++i)
                            {
                                std::string value;
                                if (!(line_stream >> value))
                                {
                                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                                              "Malformed PLY edge entry in " + path.string());
                                }
                            }
                            break;
                        }
                        case EdgePropertySemantic::skip_scalar:
                        {
                            std::string value;
                            if (!(line_stream >> value))
                            {
                                throw GeometryIoException(GeometryIoError::invalid_argument,
                                                          "Malformed PLY edge entry in " + path.string());
                            }
                            break;
                        }
                        }
                    }

                    if (indices.size() != 2U)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "PLY edge entry must reference exactly two vertices: "
                                                      + path.string());
                    }

                    edges.emplace_back(indices[0], indices[1]);
                }
                else
                {
                    std::size_t offset = 0;
                    if (tokens.size() >= 3)
                    {
                        try
                        {
                            const std::size_t declared = static_cast<std::size_t>(std::stoull(tokens[0]));
                            if (declared > 0 && declared <= tokens.size() - 1)
                            {
                                if (declared < 2)
                                {
                                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                                              "PLY edge entry must reference at least two vertices: "
                                                                  + path.string());
                                }
                                offset = 1;
                            }
                        }
                        catch (const std::exception&)
                        {
                            offset = 0;
                        }
                    }

                    if (tokens.size() - offset < 2)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Edge entry in PLY graph is incomplete: " + path.string());
                    }

                    const auto a = parse_index(tokens[offset], path, "Invalid edge index");
                    const auto b = parse_index(tokens[offset + 1], path, "Invalid edge index");
                    edges.emplace_back(a, b);
                }
                ++read_edges;
            }

            for (const auto& [a, b] : edges)
            {
                if (a >= vertices.size() || b >= vertices.size())
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Edge references vertex outside range while reading PLY graph: "
                                                  + path.string());
                }

                (void)graph.add_edge(vertices[a], vertices[b]);
            }
        }
    } // namespace

    GraphFileFormat EdgeListGraphImporter::format() const noexcept
    {
        return GraphFileFormat::edgelist;
    }

    GeometryIoResult<void> EdgeListGraphImporter::import(const std::filesystem::path& path,
                                                         geometry::GraphInterface& graph) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_graph_edgelist(path, graph); });
    }

    GraphFileFormat PlyGraphImporter::format() const noexcept
    {
        return GraphFileFormat::ply;
    }

    GeometryIoResult<void> PlyGraphImporter::import(const std::filesystem::path& path,
                                                    geometry::GraphInterface& graph) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_graph_ply(path, graph); });
    }
} // namespace engine::io
