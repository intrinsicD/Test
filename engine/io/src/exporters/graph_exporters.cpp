#include "engine/io/exporters/graph.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/geometry/graph/graph.hpp"

#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::ensure_parent_directory;

        void write_graph_edgelist(const std::filesystem::path& path, const geometry::GraphInterface& graph)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open edge list file for writing: " + path.string());
            }

            const std::size_t vcount = graph.vertex_count();
            const std::size_t ecount = graph.edge_count();
            stream << "# Engine IO graph edge list (0-based indices)\n";
            stream << "graph " << vcount << ' ' << ecount << "\n";

            const std::size_t invalid = std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> vertex_indices(graph.vertices_size(), invalid);
            std::vector<geometry::VertexHandle> exported_vertices;
            exported_vertices.reserve(vcount);
            for (const auto v : graph.vertices())
            {
                if (graph.is_deleted(v)) { continue; }
                vertex_indices[v.index()] = exported_vertices.size();
                exported_vertices.push_back(v);
            }

            for (const auto v : exported_vertices)
            {
                const auto& p = graph.position(v);
                stream << "v " << p[0] << ' ' << p[1] << ' ' << p[2] << "\n";
            }

            for (const auto e : graph.edges())
            {
                if (graph.is_deleted(e)) { continue; }
                const auto va = graph.vertex(e, 0);
                const auto vb = graph.vertex(e, 1);
                const auto ia = vertex_indices[va.index()];
                const auto ib = vertex_indices[vb.index()];
                if (ia == invalid || ib == invalid)
                {
                    throw GeometryIoException(
                        GeometryIoError::invalid_argument,
                        "Graph contains edge with unregistered vertex while writing edge list");
                }
                if (ia == ib)
                {
                    continue;
                }
                stream << "e " << ia << ' ' << ib << "\n";
            }

            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "I/O failure while writing edge list: " + path.string());
            }
        }
    } // namespace

    GraphFileFormat EdgeListGraphExporter::format() const noexcept
    {
        return GraphFileFormat::edgelist;
    }

    GeometryIoResult<void>
    EdgeListGraphExporter::export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_graph_edgelist(path, graph); });
    }
} // namespace engine::io
