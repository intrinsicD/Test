#include "engine/geometry/graph/graph.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace engine::geometry::graph
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        [[nodiscard]] IOFlags::Format resolve_format(IOFlags::Format requested,
                                                     const std::filesystem::path& path)
        {
            if (requested != IOFlags::Format::kAuto)
            {
                return requested;
            }

            std::string extension = path.extension().string();
            extension = to_lower(std::move(extension));

            if (extension == ".graph" || extension == ".edge")
            {
                return IOFlags::Format::kEdgeList;
            }

            throw std::runtime_error("Unsupported graph format for file \"" + path.string() + "\"");
        }

        struct ParsedEdge
        {
            std::size_t start{std::numeric_limits<std::size_t>::max()};
            std::size_t end{std::numeric_limits<std::size_t>::max()};
        };
    } // namespace

    void read(GraphInterface& graph, const std::filesystem::path& path)
    {
        const IOFlags::Format format = resolve_format(IOFlags::Format::kAuto, path);
        switch (format)
        {
            case IOFlags::Format::kEdgeList:
                break;
            default:
                throw std::runtime_error("Unsupported graph format for file \"" + path.string() + "\"");
        }

        std::ifstream stream(path);
        if (!stream.is_open())
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Failed to open graph file \"" + path.string() + "\" for reading");
        }

        std::vector<math::vec3> positions;
        std::vector<ParsedEdge> edges;

        std::size_t declared_vertices = 0;
        std::size_t declared_edges = 0;

        std::string line;
        std::size_t line_number = 0;
        while (std::getline(stream, line))
        {
            ++line_number;
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

            if (token.empty())
            {
                continue;
            }

            if (token[0] == '#')
            {
                continue;
            }

            if (token == "graph")
            {
                if (!(line_stream >> declared_vertices >> declared_edges))
                {
                    throw std::runtime_error("Malformed graph header on line " + std::to_string(line_number) +
                                             " of \"" + path.string() + "\"");
                }
                continue;
            }

            if (token == "v")
            {
                float x = 0.0F;
                float y = 0.0F;
                float z = 0.0F;
                if (!(line_stream >> x >> y >> z))
                {
                    throw std::runtime_error("Invalid vertex specification on line " + std::to_string(line_number) +
                                             " of \"" + path.string() + "\"");
                }

                positions.emplace_back(x, y, z);
                continue;
            }

            if (token == "e")
            {
                ParsedEdge edge{};
                if (!(line_stream >> edge.start >> edge.end))
                {
                    throw std::runtime_error("Invalid edge specification on line " + std::to_string(line_number) +
                                             " of \"" + path.string() + "\"");
                }

                edges.emplace_back(edge);
                continue;
            }

            throw std::runtime_error("Unknown directive \"" + token + "\" on line " + std::to_string(line_number) +
                                     " of \"" + path.string() + "\"");
        }

        if (declared_vertices != 0 && declared_vertices != positions.size())
        {
            throw std::runtime_error("Graph file \"" + path.string() + "\" declares " +
                                     std::to_string(declared_vertices) + " vertices but provides " +
                                     std::to_string(positions.size()));
        }

        if (declared_edges != 0 && declared_edges != edges.size())
        {
            throw std::runtime_error("Graph file \"" + path.string() + "\" declares " +
                                     std::to_string(declared_edges) + " edges but provides " +
                                     std::to_string(edges.size()));
        }

        graph.clear();
        graph.reserve(positions.size(), edges.size());

        std::vector<VertexHandle> vertex_handles;
        vertex_handles.reserve(positions.size());
        for (const auto& position : positions)
        {
            vertex_handles.push_back(graph.add_vertex(position));
        }

        for (const auto& edge : edges)
        {
            if (edge.start == edge.end)
            {
                throw std::runtime_error("Self-edge encountered while importing graph file \"" + path.string() + "\"");
            }

            if (edge.start >= vertex_handles.size() || edge.end >= vertex_handles.size())
            {
                throw std::runtime_error("Edge in graph file \"" + path.string() + "\" references vertex outside range");
            }

            const auto halfedge = graph.add_edge(vertex_handles[edge.start], vertex_handles[edge.end]);
            if (!graph.is_valid(halfedge))
            {
                throw std::runtime_error("Failed to add edge while importing graph file \"" + path.string() + "\"");
            }
        }
    }

    void write(const GraphInterface& graph, const std::filesystem::path& path, const IOFlags& flags)
    {
        const IOFlags::Format format = resolve_format(flags.format, path);
        if (format != IOFlags::Format::kEdgeList)
        {
            throw std::runtime_error("Unsupported graph format for file \"" + path.string() + "\"");
        }

        std::ofstream stream(path);
        if (!stream.is_open())
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Failed to open graph file \"" + path.string() + "\" for writing");
        }

        const int precision = std::max(1, flags.precision);
        stream.setf(std::ios::fmtflags(0), std::ios::floatfield);
        stream << std::setprecision(precision);

        std::vector<std::size_t> index_map(graph.vertices_size(), std::numeric_limits<std::size_t>::max());
        std::vector<VertexHandle> exported_vertices;
        exported_vertices.reserve(graph.vertex_count());

        for (auto vertex : graph.vertices())
        {
            if (graph.is_deleted(vertex))
            {
                continue;
            }

            index_map[vertex.index()] = exported_vertices.size();
            exported_vertices.push_back(vertex);
        }

        std::vector<std::pair<std::size_t, std::size_t>> exported_edges;
        exported_edges.reserve(graph.edge_count());
        for (auto edge : graph.edges())
        {
            if (graph.is_deleted(edge))
            {
                continue;
            }

            const auto halfedge = graph.halfedge(edge, 0);
            const auto start = graph.from_vertex(halfedge);
            const auto end = graph.to_vertex(halfedge);

            if (!start.is_valid() || !end.is_valid())
            {
                continue;
            }

            const auto start_index = index_map[start.index()];
            const auto end_index = index_map[end.index()];

            if (start_index == std::numeric_limits<std::size_t>::max() ||
                end_index == std::numeric_limits<std::size_t>::max())
            {
                continue;
            }

            if (start_index == end_index)
            {
                continue;
            }

            exported_edges.emplace_back(start_index, end_index);
        }

        if (flags.include_header_comment)
        {
            stream << "# Engine geometry graph edge list (0-based indices)\n";
        }

        if (flags.include_counts)
        {
            stream << "graph " << exported_vertices.size() << ' ' << exported_edges.size() << '\n';
        }

        for (auto vertex : exported_vertices)
        {
            const auto& position = graph.position(vertex);
            stream << "v " << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
        }

        for (const auto& [start_index, end_index] : exported_edges)
        {
            stream << "e " << start_index << ' ' << end_index << '\n';
        }

        stream.flush();
        if (!stream)
        {
            throw std::runtime_error("Failed while writing graph file \"" + path.string() + "\"");
        }
    }
} // namespace engine::geometry::graph
