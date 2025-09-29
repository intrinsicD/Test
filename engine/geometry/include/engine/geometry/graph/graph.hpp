#pragma once

#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/properties/property_handle.hpp"
#include "engine/geometry/utils/iterators.hpp"
#include "engine/geometry/utils/ranges.hpp"
#include "engine/geometry/utils/circulators.hpp"
#include "engine/geometry/utils/connectivity.hpp"
#include "engine/math/vector.hpp"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace engine::geometry::graph {
    struct IOFlags;

    class GraphInterface {
    public:
        GraphInterface(Vertices &vertex_props,
                       Halfedges &halfedge_props,
                       Edges &edge_props);

        GraphInterface(const GraphInterface &rhs);

        GraphInterface(GraphInterface &&) noexcept = default;

        ~GraphInterface();

        GraphInterface &operator=(const GraphInterface &rhs);

        GraphInterface &assign(const GraphInterface &rhs);

        using VertexIterator = Iterator<GraphInterface, VertexHandle>;
        using HalfedgeIterator = Iterator<GraphInterface, HalfedgeHandle>;
        using EdgeIterator = Iterator<GraphInterface, EdgeHandle>;

        using VertexRange = Range<VertexIterator>;
        using HalfedgeRange = Range<HalfedgeIterator>;
        using EdgeRange = Range<EdgeIterator>;

        [[nodiscard]] VertexHandle add_vertex(const math::vec3 &p);

        void clear();

        void free_memory();

        void reserve(std::size_t nvertices, std::size_t nedges);

        void garbage_collection();

        [[nodiscard]] std::size_t vertices_size() const noexcept { return vertex_props_.size(); }
        [[nodiscard]] std::size_t halfedges_size() const noexcept { return halfedge_props_.size(); }
        [[nodiscard]] std::size_t edges_size() const noexcept { return edge_props_.size(); }

        [[nodiscard]] std::size_t vertex_count() const noexcept { return vertices_size() - deleted_vertices_; }
        [[nodiscard]] std::size_t halfedge_count() const noexcept { return halfedges_size() - 2 * deleted_edges_; }
        [[nodiscard]] std::size_t edge_count() const noexcept { return edges_size() - deleted_edges_; }

        [[nodiscard]] bool is_empty() const noexcept { return vertex_count() == 0; }

        [[nodiscard]] bool is_deleted(VertexHandle v) const { return vertex_deleted_[v]; }
        [[nodiscard]] bool is_deleted(HalfedgeHandle h) const { return edge_deleted_[edge(h)]; }
        [[nodiscard]] bool is_deleted(EdgeHandle e) const { return edge_deleted_[e]; }

        [[nodiscard]] bool is_valid(VertexHandle v) const { return v.is_valid() && v.index() < vertices_size(); }
        [[nodiscard]] bool is_valid(HalfedgeHandle h) const { return h.is_valid() && h.index() < halfedges_size(); }
        [[nodiscard]] bool is_valid(EdgeHandle e) const { return e.is_valid() && e.index() < edges_size(); }

        [[nodiscard]] HalfedgeHandle halfedge(VertexHandle v) const { return vertex_connectivity_[v].halfedge; }
        void set_halfedge(VertexHandle v, HalfedgeHandle h) { vertex_connectivity_[v].halfedge = h; }

        [[nodiscard]] bool is_boundary(VertexHandle v) const;

        [[nodiscard]] bool is_isolated(VertexHandle v) const { return !halfedge(v).is_valid(); }

        [[nodiscard]] VertexHandle to_vertex(HalfedgeHandle h) const { return halfedge_connectivity_[h].vertex; }
        [[nodiscard]] VertexHandle from_vertex(HalfedgeHandle h) const { return to_vertex(opposite_halfedge(h)); }
        void set_vertex(HalfedgeHandle h, VertexHandle v) { halfedge_connectivity_[h].vertex = v; }

        [[nodiscard]] HalfedgeHandle next_halfedge(HalfedgeHandle h) const { return halfedge_connectivity_[h].next; }

        void set_next_halfedge(HalfedgeHandle h, HalfedgeHandle next);

        void set_prev_halfedge(HalfedgeHandle h, HalfedgeHandle prev);

        [[nodiscard]] HalfedgeHandle prev_halfedge(HalfedgeHandle h) const { return halfedge_connectivity_[h].prev; }

        [[nodiscard]] HalfedgeHandle opposite_halfedge(HalfedgeHandle h) const {
            return HalfedgeHandle((h.index() & 1U) ? h.index() - 1U : h.index() + 1U);
        }

        [[nodiscard]] HalfedgeHandle ccw_rotated_halfedge(HalfedgeHandle h) const {
            return opposite_halfedge(prev_halfedge(h));
        }

        [[nodiscard]] HalfedgeHandle cw_rotated_halfedge(HalfedgeHandle h) const {
            return next_halfedge(opposite_halfedge(h));
        }

        [[nodiscard]] EdgeHandle edge(HalfedgeHandle h) const { return EdgeHandle(h.index() >> 1U); }
        [[nodiscard]] HalfedgeHandle halfedge(EdgeHandle e, unsigned int i) const;

        [[nodiscard]] VertexHandle vertex(EdgeHandle e, unsigned int i) const { return to_vertex(halfedge(e, i)); }

        
        template<class T>
        [[nodiscard]] VertexProperty<T> add_vertex_property(const std::string &name, T default_value = T()) {
            return VertexProperty<T>(vertex_props_.add<T>(name, default_value));
        }

        template<class T>
        [[nodiscard]] VertexProperty<T> get_vertex_property(const std::string &name) const {
            return VertexProperty<T>(vertex_props_.get<T>(name));
        }

        template<class T>
        [[nodiscard]] VertexProperty<T> vertex_property(const std::string &name, T default_value = T()) {
            return VertexProperty<T>(vertex_props_.get_or_add<T>(name, default_value));
        }

        template<class T>
        void remove_vertex_property(VertexProperty<T> &prop) {
            vertex_props_.remove(prop);
        }

        [[nodiscard]] bool has_vertex_property(const std::string &name) const { return vertex_props_.exists(name); }

        template<class T>
        [[nodiscard]] HalfedgeProperty<T> add_halfedge_property(const std::string &name, T default_value = T()) {
            return HalfedgeProperty<T>(halfedge_props_.add<T>(name, default_value));
        }

        template<class T>
        [[nodiscard]] EdgeProperty<T> add_edge_property(const std::string &name, T default_value = T()) {
            return EdgeProperty<T>(edge_props_.add<T>(name, default_value));
        }

        template<class T>
        [[nodiscard]] HalfedgeProperty<T> get_halfedge_property(const std::string &name) const {
            return HalfedgeProperty<T>(halfedge_props_.get<T>(name));
        }

        template<class T>
        [[nodiscard]] EdgeProperty<T> get_edge_property(const std::string &name) const {
            return EdgeProperty<T>(edge_props_.get<T>(name));
        }

        template<class T>
        [[nodiscard]] HalfedgeProperty<T> halfedge_property(const std::string &name, T default_value = T()) {
            return HalfedgeProperty<T>(halfedge_props_.get_or_add<T>(name, default_value));
        }

        template<class T>
        [[nodiscard]] EdgeProperty<T> edge_property(const std::string &name, T default_value = T()) {
            return EdgeProperty<T>(edge_props_.get_or_add<T>(name, default_value));
        }

        template<class T>
        void remove_halfedge_property(HalfedgeProperty<T> &prop) {
            halfedge_props_.remove(prop);
        }

        [[nodiscard]] bool has_halfedge_property(const std::string &name) const { return halfedge_props_.exists(name); }

        template<class T>
        void remove_edge_property(EdgeProperty<T> &prop) {
            edge_props_.remove(prop);
        }

        [[nodiscard]] bool has_edge_property(const std::string &name) const { return edge_props_.exists(name); }

        [[nodiscard]] std::vector<std::string> vertex_properties() const { return vertex_props_.properties(); }
        [[nodiscard]] std::vector<std::string> halfedge_properties() const { return halfedge_props_.properties(); }
        [[nodiscard]] std::vector<std::string> edge_properties() const { return edge_props_.properties(); }

        
        [[nodiscard]] VertexIterator vertices_begin() const;

        [[nodiscard]] VertexIterator vertices_end() const;

        [[nodiscard]] VertexRange vertices() const { return {vertices_begin(), vertices_end()}; }

        [[nodiscard]] HalfedgeIterator halfedges_begin() const;

        [[nodiscard]] HalfedgeIterator halfedges_end() const;

        [[nodiscard]] HalfedgeRange halfedges() const { return {halfedges_begin(), halfedges_end()}; }

        [[nodiscard]] EdgeIterator edges_begin() const;

        [[nodiscard]] EdgeIterator edges_end() const;

        [[nodiscard]] EdgeRange edges() const { return {edges_begin(), edges_end()}; }
        
        [[nodiscard]] VertexAroundVertexCirculator<GraphInterface> vertices(VertexHandle v) const {
            return {this, v};
        }

        [[nodiscard]] EdgeAroundVertexCirculator<GraphInterface> edges(VertexHandle v) const {
            return {this, v};
        }

        [[nodiscard]] HalfedgeAroundVertexCirculator<GraphInterface> halfedges(VertexHandle v) const {
            return {this, v};
        }
        
        [[nodiscard]] HalfedgeHandle insert_vertex(EdgeHandle e, const math::vec3 &p);

        [[nodiscard]] HalfedgeHandle insert_vertex(EdgeHandle e, VertexHandle v);

        [[nodiscard]] HalfedgeHandle insert_vertex(HalfedgeHandle h, VertexHandle v);

        [[nodiscard]] HalfedgeHandle find_halfedge(VertexHandle start, VertexHandle end) const;

        [[nodiscard]] EdgeHandle find_edge(VertexHandle a, VertexHandle b) const;
        
        [[nodiscard]] bool is_collapse_ok(HalfedgeHandle h) const;

        void collapse(HalfedgeHandle h);

        [[nodiscard]] bool is_removal_ok(EdgeHandle e) const;

        bool remove_edge(EdgeHandle e);

        [[nodiscard]] HalfedgeHandle split(EdgeHandle e, const math::vec3 &p);

        [[nodiscard]] HalfedgeHandle split(EdgeHandle e, VertexHandle v);

        [[nodiscard]] std::size_t valence(VertexHandle v) const;

        void delete_vertex(VertexHandle v);

        void delete_edge(EdgeHandle e);

        [[nodiscard]] const math::vec3 &position(VertexHandle v) const { return vertex_points_[v]; }
        [[nodiscard]] math::vec3 &position(VertexHandle v) { return vertex_points_[v]; }
        [[nodiscard]] std::vector<math::vec3> &positions() { return vertex_points_.vector(); }

        [[nodiscard]] VertexHandle new_vertex();

        [[nodiscard]] HalfedgeHandle new_edge();

        [[nodiscard]] HalfedgeHandle new_edge(VertexHandle start, VertexHandle end);

        [[nodiscard]] HalfedgeHandle add_edge(VertexHandle start, VertexHandle end);

        [[nodiscard]] bool has_garbage() const noexcept { return has_garbage_; }
    private:
        void ensure_properties();

        friend void read(GraphInterface &, const std::filesystem::path &);

        friend void write(const GraphInterface &, const std::filesystem::path &, const IOFlags &);

        Vertices &vertex_props_;
        Halfedges &halfedge_props_;
        Edges &edge_props_;

        VertexProperty<math::vec3> vertex_points_;
        VertexProperty<VertexConnectivity> vertex_connectivity_;
        HalfedgeProperty<HalfedgeConnectivity> halfedge_connectivity_;

        VertexProperty<bool> vertex_deleted_;
        EdgeProperty<bool> edge_deleted_;

        PropertyIndex deleted_vertices_{0};
        PropertyIndex deleted_edges_{0};

        bool has_garbage_{false};
    };
} // namespace engine::geometry::graph


namespace engine::geometry {
    using GraphInterface = graph::GraphInterface;

    struct GraphData {
        Vertices vertex_props;
        Halfedges halfedge_props;
        Edges edge_props;
    };

    struct Graph {
        Graph() : data(), interface(data.vertex_props, data.halfedge_props, data.edge_props) {
        }

        GraphData data;
        GraphInterface interface;
    };
}
