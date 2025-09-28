#include "engine/geometry/graph/graph.hpp"

namespace engine::geometry::graph {

std::ostream& operator<<(std::ostream& os, VertexHandle v)
{
    return os << 'v' << v.index();
}

std::ostream& operator<<(std::ostream& os, EdgeHandle e)
{
    return os << 'e' << e.index();
}

Graph::Graph() = default;

Graph::Graph(const Graph& rhs) = default;

Graph& Graph::operator=(const Graph& rhs) = default;

Graph::~Graph() = default;

void Graph::clear()
{
    edges_.clear();
    adjacency_.clear();
    vertex_props_.clear();
    edge_props_.clear();
}

void Graph::reserve_vertices(std::size_t count)
{
    adjacency_.reserve(count);
    vertex_props_.reserve(count);
}

void Graph::reserve_edges(std::size_t count)
{
    edges_.reserve(count);
    edge_props_.reserve(count);
}

VertexHandle Graph::add_vertex()
{
    const auto index = static_cast<VertexHandle::index_type>(adjacency_.size());
    adjacency_.emplace_back();
    vertex_props_.push_back();
    return VertexHandle(index);
}

std::optional<EdgeHandle> Graph::add_edge(VertexHandle v0, VertexHandle v1)
{
    if (!is_valid(v0) || !is_valid(v1))
    {
        return std::nullopt;
    }

    const auto index = static_cast<EdgeHandle::index_type>(edges_.size());
    edges_.push_back({v0, v1});
    edge_props_.push_back();

    const EdgeHandle handle{index};
    adjacency_[v0.index()].push_back(handle);
    adjacency_[v1.index()].push_back(handle);

    return handle;
}

const std::vector<EdgeHandle>& Graph::incident_edges(VertexHandle v) const
{
    assert(is_valid(v));
    return adjacency_[v.index()];
}

std::vector<VertexHandle> Graph::neighbors(VertexHandle v) const
{
    assert(is_valid(v));
    const auto& edges = adjacency_[v.index()];
    std::vector<VertexHandle> result;
    result.reserve(edges.size());
    for (auto edge : edges)
    {
        const auto [a, b] = endpoints(edge);
        result.push_back((a == v) ? b : a);
    }
    return result;
}

std::pair<VertexHandle, VertexHandle> Graph::endpoints(EdgeHandle e) const
{
    assert(is_valid(e));
    const auto& record = edges_[e.index()];
    return {record.v0, record.v1};
}

} // namespace engine::geometry::graph

