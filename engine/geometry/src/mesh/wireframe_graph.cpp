#include "engine/geometry/mesh/wireframe_graph.hpp"

#include <unordered_set>

namespace engine::geometry::mesh
{
    namespace
    {
        [[nodiscard]] std::uint64_t make_edge_key(std::uint32_t a, std::uint32_t b) noexcept
        {
            if (a > b)
            {
                std::swap(a, b);
            }
            return (static_cast<std::uint64_t>(a) << 32U) | static_cast<std::uint64_t>(b);
        }
    } // namespace

    geometry::Graph build_wireframe_graph(const SurfaceMesh& surface)
    {
        geometry::Graph graph;
        auto& interface = graph.interface;

        if (surface.positions.empty())
        {
            return graph;
        }

        interface.reserve(surface.positions.size(), surface.indices.size());

        std::vector<VertexHandle> vertex_handles;
        vertex_handles.reserve(surface.positions.size());
        for (const auto& position : surface.positions)
        {
            vertex_handles.push_back(interface.add_vertex(position));
        }

        if (surface.indices.empty())
        {
            return graph;
        }

        std::unordered_set<std::uint64_t> inserted_edges;
        inserted_edges.reserve(surface.indices.size());

        const auto add_edge = [&](std::uint32_t lhs, std::uint32_t rhs)
        {
            if (lhs >= vertex_handles.size() || rhs >= vertex_handles.size() || lhs == rhs)
            {
                return;
            }

            const auto key = make_edge_key(lhs, rhs);
            if (inserted_edges.find(key) != inserted_edges.end())
            {
                return;
            }
            inserted_edges.insert(key);

            const auto existing = interface.find_edge(vertex_handles[lhs], vertex_handles[rhs]);
            if (existing.has_value())
            {
                return;
            }

            static_cast<void>(interface.add_edge(vertex_handles[lhs], vertex_handles[rhs]));
        };

        for (std::size_t index = 0; index + 2 < surface.indices.size(); index += 3)
        {
            const std::uint32_t a = surface.indices[index];
            const std::uint32_t b = surface.indices[index + 1];
            const std::uint32_t c = surface.indices[index + 2];
            add_edge(a, b);
            add_edge(b, c);
            add_edge(c, a);
        }

        return graph;
    }
}
