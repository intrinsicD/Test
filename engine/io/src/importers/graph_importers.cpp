#include "engine/io/importers/graph.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/geometry/graph/graph.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::map_open_error;
        using engine::io::detail::tokenize;
        using engine::math::vec3;

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
} // namespace engine::io
