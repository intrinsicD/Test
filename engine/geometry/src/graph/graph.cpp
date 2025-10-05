#include "engine/geometry/graph/graph.hpp"

namespace engine::geometry::graph
{
    GraphInterface::GraphInterface(Vertices& vertex_props,
                                   Halfedges& halfedge_props,
                                   Edges& edge_props) : vertex_props_(vertex_props),
                                                        halfedge_props_(halfedge_props),
                                                        edge_props_(edge_props)
    {
        ensure_properties();
    }

    void GraphInterface::ensure_properties()
    {
        vertex_points_ = vertex_property<math::vec3>("v:point");
        vertex_connectivity_ = vertex_property<VertexConnectivity>("v:connectivity");
        halfedge_connectivity_ = halfedge_property<HalfedgeConnectivity>("h:connectivity");

        vertex_deleted_ = vertex_property<bool>("v:deleted", false);
        edge_deleted_ = edge_property<bool>("e:deleted", false);
    }

    GraphInterface::GraphInterface(const GraphInterface& rhs) : vertex_props_(rhs.vertex_props_),
                                                                halfedge_props_(rhs.halfedge_props_),
                                                                edge_props_(rhs.edge_props_)
    {
        operator=(rhs);
    }

    GraphInterface::~GraphInterface() = default;

    GraphInterface& GraphInterface::operator=(const GraphInterface& rhs)
    {
        if (this == &rhs)
        {
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

    GraphInterface& GraphInterface::assign(const GraphInterface& rhs)
    {
        if (this == &rhs)
        {
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

    void GraphInterface::clear()
    {
        vertex_props_.clear();
        halfedge_props_.clear();
        edge_props_.clear();

        free_memory();
        ensure_properties();

        deleted_vertices_ = 0;
        deleted_edges_ = 0;
        has_garbage_ = false;
    }

    void GraphInterface::free_memory()
    {
        vertex_props_.shrink_to_fit();
        halfedge_props_.shrink_to_fit();
        edge_props_.shrink_to_fit();
    }

    void GraphInterface::reserve(std::size_t nvertices, std::size_t nedges)
    {
        vertex_props_.reserve(nvertices);
        halfedge_props_.reserve(2 * nedges);
        edge_props_.reserve(nedges);
    }

    GraphInterface::VertexIterator GraphInterface::vertices_begin() const
    {
        return VertexIterator(VertexHandle(0), this);
    }

    GraphInterface::VertexIterator GraphInterface::vertices_end() const
    {
        return VertexIterator(VertexHandle(static_cast<PropertyIndex>(vertices_size())), this);
    }

    GraphInterface::HalfedgeIterator GraphInterface::halfedges_begin() const
    {
        return HalfedgeIterator(HalfedgeHandle(0), this);
    }

    GraphInterface::HalfedgeIterator GraphInterface::halfedges_end() const
    {
        return HalfedgeIterator(HalfedgeHandle(static_cast<PropertyIndex>(halfedges_size())), this);
    }

    GraphInterface::EdgeIterator GraphInterface::edges_begin() const
    {
        return EdgeIterator(EdgeHandle(0), this);
    }

    GraphInterface::EdgeIterator GraphInterface::edges_end() const
    {
        return EdgeIterator(EdgeHandle(static_cast<PropertyIndex>(edges_size())), this);
    }


    bool GraphInterface::is_boundary(VertexHandle v) const
    {
        // A vertex is on the boundary if any of its incident halfedges next pointer points to its opposite halfedge. (this is by design so that the circulators work)
        const HalfedgeHandle h = halfedge(v);
        return (h.is_valid() && next_halfedge(h) == opposite_halfedge(h));
    }


    void GraphInterface::set_next_halfedge(HalfedgeHandle h, HalfedgeHandle next)
    {
        halfedge_connectivity_[h].next = next;
        halfedge_connectivity_[next].prev = h;
    }

    void GraphInterface::set_prev_halfedge(HalfedgeHandle h, HalfedgeHandle prev)
    {
        halfedge_connectivity_[h].prev = prev;
        halfedge_connectivity_[prev].next = h;
    }

    HalfedgeHandle GraphInterface::halfedge(EdgeHandle e, unsigned int i) const
    {
        assert(i <= 1);
        return HalfedgeHandle((e.index() << 1U) + i);
    }


    HalfedgeHandle GraphInterface::insert_vertex(EdgeHandle e, const math::vec3& p)
    {
        return insert_vertex(halfedge(e, 0), add_vertex(p));
    }

    HalfedgeHandle GraphInterface::insert_vertex(EdgeHandle e, VertexHandle v)
    {
        return insert_vertex(halfedge(e, 0), v);
    }

    std::optional<HalfedgeHandle> GraphInterface::find_halfedge(VertexHandle start, VertexHandle end) const
    {
        assert(is_valid(start) && is_valid(end));

        HalfedgeHandle h = halfedge(start);
        const HalfedgeHandle hh = h;

        if (h.is_valid())
        {
            do
            {
                if (to_vertex(h) == end)
                {
                    return h;
                }
                h = cw_rotated_halfedge(h);
            }
            while (h != hh);
        }

        return std::nullopt;
    }

    std::optional<EdgeHandle> GraphInterface::find_edge(VertexHandle a, VertexHandle b) const
    {
        if (auto h = find_halfedge(a, b)) return edge(*h);
        return std::nullopt;
    }


    VertexHandle GraphInterface::add_vertex(const math::vec3& p)
    {
        VertexHandle v = new_vertex();
        if (v.is_valid())
        {
            vertex_points_[v] = p;
        }
        return v;
    }


    std::size_t GraphInterface::valence(VertexHandle v) const
    {
        auto vv = vertices(v);
        return static_cast<std::size_t>(std::distance(vv.begin(), vv.end()));
    }


    HalfedgeHandle GraphInterface::insert_vertex(HalfedgeHandle h0, VertexHandle v)
    {
        auto h1 = opposite_halfedge(h0);
        auto end = to_vertex(h0);
        auto h0next = next_halfedge(h0);
        auto h1prev = prev_halfedge(h1);

        auto h2 = new_edge(v, end);
        if (!is_valid(h2)) return {};
        auto h3 = opposite_halfedge(h2);

        set_next_halfedge(h0, h2);
        set_next_halfedge(h2, h0next);

        set_next_halfedge(h3, h1);
        set_next_halfedge(h1prev, h3);

        set_vertex(h0, v);

        return h2;
    }


    bool GraphInterface::is_collapse_ok(HalfedgeHandle h) const
    {
        if (!is_valid(h) || is_deleted(edge(h)))
        {
            return false;
        }

        auto o = opposite_halfedge(h);
        auto v0 = to_vertex(h); // Vertex to keep
        auto v1 = to_vertex(o); // Vertex to remove

        // Policy: Do not allow collapse if it would merge two previously
        // connected neighborhoods, creating a duplicate edge.
        for (auto v1_neighbor : vertices(v1))
        {
            if (v1_neighbor == v0)
            {
                continue;
            }
            if (find_edge(v0, v1_neighbor).has_value())
            {
                return false; // Collapse would create a duplicate edge.
            }
        }

        return true; // Collapse is OK.
    }

    bool GraphInterface::is_removal_ok(EdgeHandle e) const
    {
        if (!is_valid(e) || is_deleted(e))
        {
            return false;
        }

        // Policy: Do not allow removal if it would isolate a vertex.
        auto h = halfedge(e, 0);
        if (is_boundary(from_vertex(h)) || is_boundary(to_vertex(h)))
        {
            return false; // Removal would isolate a vertex.
        }

        return true; // Removal is OK.
    }

    bool GraphInterface::remove_vertex(VertexHandle v)
    {
        if (is_deleted(v))
        {
            return false;
        }

        // 1. Collect all incident edges. We must do this first because
        //    remove_edge will modify the half-edge connectivity,
        //    breaking the circulator.
        std::vector<EdgeHandle> incident_edges;
        for (auto h : halfedges(v))
        {
            incident_edges.push_back(edge(h));
        }

        // 2. Remove each incident edge.
        for (const auto& edge_to_remove : incident_edges)
        {
            // Check if the edge wasn't already removed as part of
            // removing another incident edge (can happen in weird configs, though rare).
            if (!is_deleted(edge_to_remove))
            {
                remove_edge(edge_to_remove);
            }
        }

        // 3. Mark the now-isolated vertex as deleted.
        vertex_deleted_[v] = true;
        ++deleted_vertices_;
        has_garbage_ = true;

        return true;
    }

    bool GraphInterface::remove_edge(EdgeHandle e)
    {
        if (!is_removal_ok(e))
        {
            return false;
        }

        auto h = halfedge(e, 0);
        auto o = opposite_halfedge(h);
        auto start = to_vertex(h);
        auto end = to_vertex(o);
        //check if any vertex points to h or o
        //if so, reassign to next halfedge
        //after that remove h and o from the circular list by reassigning next and prev pointers
        auto hnext = next_halfedge(h);
        auto hprev = prev_halfedge(h);
        auto onext = next_halfedge(o);
        auto oprev = prev_halfedge(o);
        if (halfedge(start) == h)
        {
            set_halfedge(start, hnext);
        }
        if (halfedge(end) == o)
        {
            set_halfedge(end, onext);
        }
        set_next_halfedge(hprev, hnext);
        set_next_halfedge(oprev, onext);

        edge_deleted_[e] = true;
        ++deleted_edges_;
        has_garbage_ = true;

        return true;
    }

    void GraphInterface::collapse(HalfedgeHandle h)
    {
        // Let h be the half-edge v1 -> v0. We collapse v1 onto v0.
        auto o = opposite_halfedge(h);
        auto v0 = to_vertex(h); // Vertex to keep
        auto v1 = to_vertex(o); // Vertex to remove

        // 1. Re-route all edges incident to v1 to now be incident to v0.
        // The halfedge circulator is perfect for this.
        for (auto hh : halfedges(v1))
        {
            // hh is an outgoing half-edge from v1.
            // Its opposite, opposite_halfedge(hh), is an incoming half-edge to v1.
            // We need to change the destination of this incoming half-edge to v0.
            set_vertex(opposite_halfedge(hh), v0);
        }

        // 2. Unlink h and o from the half-edge rings around v0 and v1.
        auto h_prev = prev_halfedge(h);
        auto h_next = next_halfedge(h);
        set_next_halfedge(h_prev, h_next);

        auto o_prev = prev_halfedge(o);
        auto o_next = next_halfedge(o);
        set_next_halfedge(o_prev, o_next);

        // 3. Update the representative half-edge for v0 if it was pointing
        // to a half-edge that is now part of a 1-edge or 2-edge loop.
        // A safe bet is to assign it to any valid outgoing half-edge.
        // h_next is a good candidate if it's not the opposite of h_prev.
        if (halfedge(v0) == o)
        {
            set_halfedge(v0, h_next);
        }

        // After re-routing, all of v1's former neighbors are now connected to v0.
        // It's possible halfedge(v0) now points to a half-edge inside what
        // is now a 2-edge loop. We should ensure it points to a non-loop edge if possible.
        // (This is an advanced robustness check, the line above is usually sufficient).


        // 4. Mark elements for deletion
        set_halfedge(v1, HalfedgeHandle()); // Isolate v1
        vertex_deleted_[v1] = true;
        ++deleted_vertices_;

        edge_deleted_[edge(h)] = true;
        ++deleted_edges_;

        has_garbage_ = true;
    }


    void GraphInterface::delete_vertex(VertexHandle v)
    {
        if (is_deleted(v))
        {
            return;
        }

        vertex_deleted_[v] = true;
        ++deleted_vertices_;
        has_garbage_ = true;
    }

    void GraphInterface::delete_edge(EdgeHandle e)
    {
        if (is_deleted(e))
        {
            return;
        }

        edge_deleted_[e] = true;
        ++deleted_edges_;
        has_garbage_ = true;
    }


    VertexHandle GraphInterface::new_vertex()
    {
        if (vertices_size() >= kInvalidPropertyIndex)
        {
            return {};
        }
        vertex_props_.push_back();
        return VertexHandle(static_cast<PropertyIndex>(vertices_size() - 1));
    }

    HalfedgeHandle GraphInterface::new_edge()
    {
        if (halfedges_size() >= kInvalidPropertyIndex)
        {
            return {};
        }

        edge_props_.push_back();
        halfedge_props_.push_back();
        halfedge_props_.push_back();

        return HalfedgeHandle(static_cast<PropertyIndex>(halfedges_size() - 2));
    }

    HalfedgeHandle GraphInterface::new_edge(VertexHandle start, VertexHandle end)
    {
        assert(start != end);

        if (halfedges_size() >= kInvalidPropertyIndex)
        {
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

    [[nodiscard]] HalfedgeHandle GraphInterface::add_edge(VertexHandle start, VertexHandle end)
    {
        assert(start != end);


        if (auto h01 = find_halfedge(start, end))
        {
            return *h01;
        }

        auto h = new_edge(start, end);
        if (!is_valid(h)) return {};

        auto o = opposite_halfedge(h);

        auto in_0 = opposite_halfedge(halfedge(start));
        if (is_valid(in_0))
        {
            auto out_next_0 = next_halfedge(in_0);
            set_next_halfedge(in_0, h);
            set_next_halfedge(o, out_next_0);
        }

        set_halfedge(start, h);

        auto out_1 = halfedge(end);
        if (is_valid(out_1))
        {
            auto in_prev_1 = prev_halfedge(out_1);
            set_next_halfedge(h, out_1);
            set_next_halfedge(in_prev_1, o);
        }

        set_halfedge(end, o);

        return h;
    }


    void GraphInterface::garbage_collection()
    {
        if (!has_garbage_)
        {
            return;
        }

        auto nv = vertices_size();
        auto ne = edges_size();
        auto nh = halfedges_size();

        VertexProperty<VertexHandle> vmap = add_vertex_property<VertexHandle>("v:garbage-collection");
        HalfedgeProperty<HalfedgeHandle> hmap = add_halfedge_property<HalfedgeHandle>("h:garbage-collection");

        for (std::size_t i = 0; i < nv; ++i)
        {
            vmap[VertexHandle(static_cast<PropertyIndex>(i))] = VertexHandle(static_cast<PropertyIndex>(i));
        }
        for (std::size_t i = 0; i < nh; ++i)
        {
            hmap[HalfedgeHandle(static_cast<PropertyIndex>(i))] = HalfedgeHandle(static_cast<PropertyIndex>(i));
        }

        if (nv > 0)
        {
            std::size_t i0 = 0;
            std::size_t i1 = nv - 1;

            while (true)
            {
                while (!vertex_deleted_[VertexHandle(static_cast<PropertyIndex>(i0))] && i0 < i1)
                {
                    ++i0;
                }
                while (vertex_deleted_[VertexHandle(static_cast<PropertyIndex>(i1))] && i0 < i1)
                {
                    --i1;
                }
                if (i0 >= i1)
                {
                    break;
                }

                vertex_props_.swap(i0, i1);
            }

            nv = vertex_deleted_[VertexHandle(static_cast<PropertyIndex>(i0))] ? i0 : i0 + 1;
        }

        if (ne > 0)
        {
            std::size_t i0 = 0;
            std::size_t i1 = ne - 1;

            while (true)
            {
                while (!edge_deleted_[EdgeHandle(static_cast<PropertyIndex>(i0))] && i0 < i1)
                {
                    ++i0;
                }
                while (edge_deleted_[EdgeHandle(static_cast<PropertyIndex>(i1))] && i0 < i1)
                {
                    --i1;
                }
                if (i0 >= i1)
                {
                    break;
                }

                edge_props_.swap(i0, i1);
                halfedge_props_.swap(2 * i0, 2 * i1);
                halfedge_props_.swap(2 * i0 + 1, 2 * i1 + 1);
            }

            ne = edge_deleted_[EdgeHandle(static_cast<PropertyIndex>(i0))] ? i0 : i0 + 1;
            nh = 2 * ne;
        }

        for (std::size_t i = 0; i < nv; ++i)
        {
            auto v = VertexHandle(static_cast<PropertyIndex>(i));
            if (!is_isolated(v))
            {
                set_halfedge(v, hmap[halfedge(v)]);
            }
        }

        for (std::size_t i = 0; i < nh; ++i)
        {
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
