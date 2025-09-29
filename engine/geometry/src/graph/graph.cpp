#include "engine/geometry/graph/graph.hpp"

namespace engine::geometry::graph {
    GraphInterface::GraphInterface(Vertices &vertex_props,
                                   Halfedges &halfedge_props,
                                   Edges &edge_props) : vertex_props_(vertex_props),
                                                        halfedge_props_(halfedge_props),
                                                        edge_props_(edge_props) {
        ensure_properties();
    }

    void GraphInterface::ensure_properties() {
        vertex_points_ = vertex_property<math::vec3>("v:point");
        vertex_connectivity_ = vertex_property<VertexConnectivity>("v:connectivity");
        halfedge_connectivity_ = halfedge_property<HalfedgeConnectivity>("h:connectivity");

        vertex_deleted_ = vertex_property<bool>("v:deleted", false);
        edge_deleted_ = edge_property<bool>("e:deleted", false);
    }

    GraphInterface::GraphInterface(const GraphInterface &rhs) : vertex_props_(rhs.vertex_props_),
                                                                halfedge_props_(rhs.halfedge_props_),
                                                                edge_props_(rhs.edge_props_) {
        operator=(rhs);
    }

    GraphInterface::~GraphInterface() = default;

    GraphInterface &GraphInterface::operator=(const GraphInterface &rhs) {
        if (this == &rhs) {
            return *this;
        }

        vertex_props_ = rhs.vertex_props_;
        halfedge_props_ = rhs.halfedge_props_;
        edge_props_ = rhs.edge_props_;

        ensure_properties();

        deleted_vertices_ = rhs.deleted_vertices_;
        deleted_edges_ = rhs.deleted_edges_;
        has_garbage_ = rhs.has_garbage_;

        return *this;
    }

    GraphInterface &GraphInterface::assign(const GraphInterface &rhs) {
        if (this == &rhs) {
            return *this;
        }

        vertex_props_.clear();
        halfedge_props_.clear();
        edge_props_.clear();

        ensure_properties();

        vertex_points_.array() = rhs.vertex_points_.array();
        vertex_connectivity_.array() = rhs.vertex_connectivity_.array();
        halfedge_connectivity_.array() = rhs.halfedge_connectivity_.array();

        vertex_deleted_.array() = rhs.vertex_deleted_.array();
        edge_deleted_.array() = rhs.edge_deleted_.array();

        vertex_props_.resize(rhs.vertices_size());
        halfedge_props_.resize(rhs.halfedges_size());
        edge_props_.resize(rhs.edges_size());

        deleted_vertices_ = rhs.deleted_vertices_;
        deleted_edges_ = rhs.deleted_edges_;
        has_garbage_ = rhs.has_garbage_;

        return *this;
    }

    void GraphInterface::clear() {
        vertex_props_.clear();
        halfedge_props_.clear();
        edge_props_.clear();

        free_memory();
        ensure_properties();

        deleted_vertices_ = 0;
        deleted_edges_ = 0;
        has_garbage_ = false;
    }

    void GraphInterface::free_memory() {
        vertex_props_.shrink_to_fit();
        halfedge_props_.shrink_to_fit();
        edge_props_.shrink_to_fit();
    }

    void GraphInterface::reserve(std::size_t nvertices, std::size_t nedges) {
        vertex_props_.reserve(nvertices);
        halfedge_props_.reserve(2 * nedges);
        edge_props_.reserve(nedges);
    }

    GraphInterface::VertexIterator GraphInterface::vertices_begin() const {
        return VertexIterator(VertexHandle(0), this);
    }

    GraphInterface::VertexIterator GraphInterface::vertices_end() const {
        return VertexIterator(VertexHandle(static_cast<PropertyIndex>(vertices_size())), this);
    }

    GraphInterface::HalfedgeIterator GraphInterface::halfedges_begin() const {
        return HalfedgeIterator(HalfedgeHandle(0), this);
    }

    GraphInterface::HalfedgeIterator GraphInterface::halfedges_end() const {
        return HalfedgeIterator(HalfedgeHandle(static_cast<PropertyIndex>(halfedges_size())), this);
    }

    GraphInterface::EdgeIterator GraphInterface::edges_begin() const {
        return EdgeIterator(EdgeHandle(0), this);
    }

    GraphInterface::EdgeIterator GraphInterface::edges_end() const {
        return EdgeIterator(EdgeHandle(static_cast<PropertyIndex>(edges_size())), this);
    }


    bool GraphInterface::is_boundary(VertexHandle v) const {
        const HalfedgeHandle h = halfedge(v);
        return (h.is_valid() && next_halfedge(h) == opposite_halfedge(h));
    }


    void GraphInterface::set_next_halfedge(HalfedgeHandle h, HalfedgeHandle next) {
        halfedge_connectivity_[h].next = next;
        halfedge_connectivity_[next].prev = h;
    }

    void GraphInterface::set_prev_halfedge(HalfedgeHandle h, HalfedgeHandle prev) {
        halfedge_connectivity_[h].prev = prev;
        halfedge_connectivity_[prev].next = h;
    }

    HalfedgeHandle GraphInterface::halfedge(EdgeHandle e, unsigned int i) const {
        assert(i <= 1);
        return HalfedgeHandle((e.index() << 1U) + i);
    }


    HalfedgeHandle GraphInterface::insert_vertex(EdgeHandle e, const math::vec3 &p) {
        return insert_vertex(halfedge(e, 0), add_vertex(p));
    }

    HalfedgeHandle GraphInterface::insert_vertex(EdgeHandle e, VertexHandle v) {
        return insert_vertex(halfedge(e, 0), v);
    }

    HalfedgeHandle GraphInterface::find_halfedge(VertexHandle start, VertexHandle end) const {
        assert(is_valid(start) && is_valid(end));

        HalfedgeHandle h = halfedge(start);
        const HalfedgeHandle hh = h;

        if (h.is_valid()) {
            do {
                if (to_vertex(h) == end) {
                    return h;
                }
                h = cw_rotated_halfedge(h);
            } while (h != hh);
        }

        return {};
    }

    EdgeHandle GraphInterface::find_edge(VertexHandle a, VertexHandle b) const {
        const HalfedgeHandle h = find_halfedge(a, b);
        return h.is_valid() ? edge(h) : EdgeHandle();
    }


    VertexHandle GraphInterface::add_vertex(const math::vec3 &p) {
        VertexHandle v = new_vertex();
        if (v.is_valid()) {
            vertex_points_[v] = p;
        }
        return v;
    }


    std::size_t GraphInterface::valence(VertexHandle v) const {
        auto vv = vertices(v);
        return static_cast<std::size_t>(std::distance(vv.begin(), vv.end()));
    }


    HalfedgeHandle GraphInterface::insert_vertex(HalfedgeHandle h0, VertexHandle v) {
        //TODO
        auto h1 = opposite_halfedge(h0);
        auto start = to_vertex(h0);
        auto end = to_vertex(h1);
        auto h0next = next_halfedge(h0);
        auto h1prev = prev_halfedge(h1);

        auto h2 = new_edge(v, end);
        if (!is_valid(h2)) return {};
        auto h3 = opposite_halfedge(h2);

        set_next_halfedge(h0, h2);
        set_next_halfedge(h2, h0next);

        set_next_halfedge(h3, h1);
        set_next_halfedge(h1prev, h3);

        return h2;
    }


    bool GraphInterface::is_collapse_ok(HalfedgeHandle v0v1) const {
        //TODO

        return true;
    }

    bool GraphInterface::is_removal_ok(EdgeHandle e) const {
        if (is_deleted(e)) {
            return false;
        }

        auto h = halfedge(e, 0);
        auto start = from_vertex(h);
        auto end = to_vertex(h);

        return !(is_boundary(start) || is_boundary(end));
    }

    bool GraphInterface::remove_edge(EdgeHandle e) {
        if (edge_deleted_[e]) {
            return false;
        }

        //TODO...

        edge_deleted_[e] = true;
        ++deleted_edges_;
        has_garbage_ = true;

        return true;
    }

    void GraphInterface::collapse(HalfedgeHandle h) {
        //TODO
    }


    void GraphInterface::delete_vertex(VertexHandle v) {
        if (is_deleted(v)) {
            return;
        }


        vertex_deleted_[v] = true;
        ++deleted_vertices_;
        has_garbage_ = true;
    }

    void GraphInterface::delete_edge(EdgeHandle e) {
        if (is_deleted(e)) {
            return;
        }

        edge_deleted_[e] = true;
        ++deleted_edges_;
        has_garbage_ = true;
    }

    
    VertexHandle GraphInterface::new_vertex() {
        if (vertices_size() >= kInvalidPropertyIndex) {
            return {};
        }
        vertex_props_.push_back();
        return VertexHandle(static_cast<PropertyIndex>(vertices_size() - 1));
    }

    HalfedgeHandle GraphInterface::new_edge() {
        if (halfedges_size() >= kInvalidPropertyIndex) {
            return {};
        }

        edge_props_.push_back();
        halfedge_props_.push_back();
        halfedge_props_.push_back();

        return HalfedgeHandle(static_cast<PropertyIndex>(halfedges_size() - 2));
    }

    HalfedgeHandle GraphInterface::new_edge(VertexHandle start, VertexHandle end) {
        assert(start != end);

        if (halfedges_size() >= kInvalidPropertyIndex) {
            return {};
        }

        edge_props_.push_back();
        halfedge_props_.push_back();
        halfedge_props_.push_back();

        const HalfedgeHandle h0(static_cast<PropertyIndex>(halfedges_size() - 2));
        const HalfedgeHandle h1(static_cast<PropertyIndex>(halfedges_size() - 1));

        set_vertex(h0, end);
        set_vertex(h1, start);

        set_next_halfedge(h0, h1);
        set_next_halfedge(h1, h0);

        return h0;
    }

    [[nodiscard]] HalfedgeHandle GraphInterface::add_edge(VertexHandle start, VertexHandle end) {
        assert(start != end);

        auto h01 = find_halfedge(start, end);
        if (is_valid(h01)) {
            return h01;
        }

        auto h = new_edge(start, end);
        if (!is_valid(h)) return {};

        auto o = opposite_halfedge(h);

        auto in_0 = opposite_halfedge(halfedge(start));
        if (is_valid(in_0)) {
            auto out_next_0 = next_halfedge(in_0);
            set_next_halfedge(in_0, h);
            set_next_halfedge(o, out_next_0);
        }

        set_halfedge(start, h);

        auto out_1 = halfedge(end);
        if (is_valid(out_1)) {
            auto out_prev_1 = next_halfedge(out_1);
            set_next_halfedge(h, out_1);
            set_next_halfedge(out_prev_1, o);
        }

        set_halfedge(end, o);

        return h;
    }

    
    void GraphInterface::garbage_collection() {
        if (!has_garbage_) {
            return;
        }

        auto nv = vertices_size();
        auto ne = edges_size();
        auto nh = halfedges_size();

        VertexProperty<VertexHandle> vmap = add_vertex_property<VertexHandle>("v:garbage-collection");
        HalfedgeProperty<HalfedgeHandle> hmap = add_halfedge_property<HalfedgeHandle>("h:garbage-collection");

        for (std::size_t i = 0; i < nv; ++i) {
            vmap[VertexHandle(static_cast<PropertyIndex>(i))] = VertexHandle(static_cast<PropertyIndex>(i));
        }
        for (std::size_t i = 0; i < nh; ++i) {
            hmap[HalfedgeHandle(static_cast<PropertyIndex>(i))] = HalfedgeHandle(static_cast<PropertyIndex>(i));
        }

        if (nv > 0) {
            std::size_t i0 = 0;
            std::size_t i1 = nv - 1;

            while (true) {
                while (!vertex_deleted_[VertexHandle(static_cast<PropertyIndex>(i0))] && i0 < i1) {
                    ++i0;
                }
                while (vertex_deleted_[VertexHandle(static_cast<PropertyIndex>(i1))] && i0 < i1) {
                    --i1;
                }
                if (i0 >= i1) {
                    break;
                }

                vertex_props_.swap(i0, i1);
            }

            nv = vertex_deleted_[VertexHandle(static_cast<PropertyIndex>(i0))] ? i0 : i0 + 1;
        }

        if (ne > 0) {
            std::size_t i0 = 0;
            std::size_t i1 = ne - 1;

            while (true) {
                while (!edge_deleted_[EdgeHandle(static_cast<PropertyIndex>(i0))] && i0 < i1) {
                    ++i0;
                }
                while (edge_deleted_[EdgeHandle(static_cast<PropertyIndex>(i1))] && i0 < i1) {
                    --i1;
                }
                if (i0 >= i1) {
                    break;
                }

                edge_props_.swap(i0, i1);
                halfedge_props_.swap(2 * i0, 2 * i1);
                halfedge_props_.swap(2 * i0 + 1, 2 * i1 + 1);
            }

            ne = edge_deleted_[EdgeHandle(static_cast<PropertyIndex>(i0))] ? i0 : i0 + 1;
            nh = 2 * ne;
        }

        for (std::size_t i = 0; i < nv; ++i) {
            auto v = VertexHandle(static_cast<PropertyIndex>(i));
            if (!is_isolated(v)) {
                set_halfedge(v, hmap[halfedge(v)]);
            }
        }

        for (std::size_t i = 0; i < nh; ++i) {
            auto h = HalfedgeHandle(static_cast<PropertyIndex>(i));
            set_vertex(h, vmap[to_vertex(h)]);
            set_next_halfedge(h, hmap[next_halfedge(h)]);
        }

        remove_vertex_property(vmap);
        remove_halfedge_property(hmap);

        vertex_props_.resize(nv);
        vertex_props_.shrink_to_fit();
        halfedge_props_.resize(nh);
        halfedge_props_.shrink_to_fit();
        edge_props_.resize(ne);
        edge_props_.shrink_to_fit();

        deleted_vertices_ = deleted_edges_ = 0;
        has_garbage_ = false;
    }
} // namespace engine::geometry::graph
