#pragma once

#include "../properties/property_set.hpp"

#include <cassert>
#include <cstdint>
#include <compare>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace engine::geometry::graph {

class Graph;

using GraphIndex = std::uint32_t;

constexpr GraphIndex kInvalidGraphIndex = std::numeric_limits<GraphIndex>::max();

class GraphHandle {
public:
    using index_type = GraphIndex;

    constexpr GraphHandle() noexcept = default;
    explicit constexpr GraphHandle(index_type idx) noexcept : index_(idx) {}

    [[nodiscard]] constexpr index_type index() const noexcept { return index_; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return index_ != kInvalidGraphIndex; }
    constexpr void reset() noexcept { index_ = kInvalidGraphIndex; }

    [[nodiscard]] auto operator<=>(const GraphHandle&) const noexcept = default;

protected:
    index_type index_{kInvalidGraphIndex};
};

class VertexHandle final : public GraphHandle {
public:
    using GraphHandle::GraphHandle;
};

class EdgeHandle final : public GraphHandle {
public:
    using GraphHandle::GraphHandle;
};

std::ostream& operator<<(std::ostream& os, VertexHandle v);
std::ostream& operator<<(std::ostream& os, EdgeHandle e);

class Graph {
public:
    Graph();
    Graph(const Graph& rhs);
    Graph(Graph&&) noexcept = default;
    Graph& operator=(const Graph& rhs);
    Graph& operator=(Graph&&) noexcept = default;
    ~Graph();

    void clear();

    void reserve_vertices(std::size_t count);
    void reserve_edges(std::size_t count);

    [[nodiscard]] std::size_t vertex_count() const noexcept { return adjacency_.size(); }
    [[nodiscard]] std::size_t edge_count() const noexcept { return edges_.size(); }

    [[nodiscard]] bool is_empty() const noexcept { return vertex_count() == 0; }

    [[nodiscard]] bool is_valid(VertexHandle v) const noexcept;
    [[nodiscard]] bool is_valid(EdgeHandle e) const noexcept;

    VertexHandle add_vertex();
    std::optional<EdgeHandle> add_edge(VertexHandle v0, VertexHandle v1);

    [[nodiscard]] const std::vector<EdgeHandle>& incident_edges(VertexHandle v) const;
    [[nodiscard]] std::vector<VertexHandle> neighbors(VertexHandle v) const;
    [[nodiscard]] std::size_t degree(VertexHandle v) const;

    [[nodiscard]] std::pair<VertexHandle, VertexHandle> endpoints(EdgeHandle e) const;
    [[nodiscard]] VertexHandle vertex(EdgeHandle e, unsigned int i) const;

    template <class T>
    [[nodiscard]] HandleProperty<VertexHandle, T> add_vertex_property(const std::string& name, T default_value = T())
    {
        return HandleProperty<VertexHandle, T>(vertex_props_.add<T>(name, default_value));
    }

    template <class T>
    [[nodiscard]] HandleProperty<VertexHandle, T> get_vertex_property(const std::string& name) const
    {
        return HandleProperty<VertexHandle, T>(vertex_props_.get<T>(name));
    }

    template <class T>
    [[nodiscard]] HandleProperty<VertexHandle, T> vertex_property(const std::string& name, T default_value = T())
    {
        return HandleProperty<VertexHandle, T>(vertex_props_.get_or_add<T>(name, default_value));
    }

    template <class T>
    void remove_vertex_property(HandleProperty<VertexHandle, T>& prop)
    {
        vertex_props_.remove(prop);
    }

    template <class T>
    [[nodiscard]] HandleProperty<EdgeHandle, T> add_edge_property(const std::string& name, T default_value = T())
    {
        return HandleProperty<EdgeHandle, T>(edge_props_.add<T>(name, default_value));
    }

    template <class T>
    [[nodiscard]] HandleProperty<EdgeHandle, T> get_edge_property(const std::string& name) const
    {
        return HandleProperty<EdgeHandle, T>(edge_props_.get<T>(name));
    }

    template <class T>
    [[nodiscard]] HandleProperty<EdgeHandle, T> edge_property(const std::string& name, T default_value = T())
    {
        return HandleProperty<EdgeHandle, T>(edge_props_.get_or_add<T>(name, default_value));
    }

    template <class T>
    void remove_edge_property(HandleProperty<EdgeHandle, T>& prop)
    {
        edge_props_.remove(prop);
    }

private:
    struct EdgeRecord {
        VertexHandle v0{};
        VertexHandle v1{};
    };

    PropertySet vertex_props_;
    PropertySet edge_props_;

    std::vector<EdgeRecord> edges_;
    std::vector<std::vector<EdgeHandle>> adjacency_;
};

inline bool Graph::is_valid(VertexHandle v) const noexcept
{
    return v.is_valid() && v.index() < adjacency_.size();
}

inline bool Graph::is_valid(EdgeHandle e) const noexcept
{
    return e.is_valid() && e.index() < edges_.size();
}

inline std::size_t Graph::degree(VertexHandle v) const
{
    return incident_edges(v).size();
}

inline VertexHandle Graph::vertex(EdgeHandle e, unsigned int i) const
{
    const auto [v0, v1] = endpoints(e);
    return (i == 0U) ? v0 : v1;
}

} // namespace engine::geometry::graph

namespace engine::geometry {
using Graph = graph::Graph;
}

