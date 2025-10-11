#pragma once

#include "engine/geometry/api.hpp"
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
#include <utility>
#include <vector>

namespace engine::geometry::mesh
{
    struct ENGINE_GEOMETRY_API IOFlags
    {
        enum class Format
        {
            kAuto,
            kOBJ,
        };

        Format format{Format::kAuto};
        int precision{9};
        bool include_header_comment{true};
    };

    class ENGINE_GEOMETRY_API HalfedgeMeshInterface
    {
    public:
        // Handle iterators ---------------------------------------------------------------------------------------------
        using VertexIterator = Iterator<HalfedgeMeshInterface, VertexHandle>;
        using HalfedgeIterator = Iterator<HalfedgeMeshInterface, HalfedgeHandle>;
        using EdgeIterator = Iterator<HalfedgeMeshInterface, EdgeHandle>;
        using FaceIterator = Iterator<HalfedgeMeshInterface, FaceHandle>;

        using VertexRange = Range<VertexIterator>;
        using HalfedgeRange = Range<HalfedgeIterator>;
        using EdgeRange = Range<EdgeIterator>;
        using FaceRange = Range<FaceIterator>;

        HalfedgeMeshInterface(Vertices& vertex_props,
                              Halfedges& halfedge_props,
                              Edges& edge_props,
                              Faces& face_props);

        HalfedgeMeshInterface(const HalfedgeMeshInterface& rhs);

        HalfedgeMeshInterface(HalfedgeMeshInterface&&) noexcept = default;

        ~HalfedgeMeshInterface();

        HalfedgeMeshInterface& operator=(const HalfedgeMeshInterface& rhs);

        HalfedgeMeshInterface& assign(const HalfedgeMeshInterface& rhs);

        // Construction ---------------------------------------------------------------------------------------------------

        [[nodiscard]] VertexHandle add_vertex(const math::vec3& p);

        [[nodiscard]] std::optional<FaceHandle> add_face(std::span<const VertexHandle> vertices);

        [[nodiscard]] std::optional<FaceHandle> add_triangle(VertexHandle v0, VertexHandle v1, VertexHandle v2);

        [[nodiscard]] std::optional<FaceHandle> add_quad(VertexHandle v0, VertexHandle v1, VertexHandle v2,
                                                         VertexHandle v3);

        void clear();

        void free_memory();

        void reserve(std::size_t nvertices, std::size_t nedges, std::size_t nfaces);

        void garbage_collection();

        [[nodiscard]] std::size_t vertices_size() const noexcept { return vertex_props_.size(); }
        [[nodiscard]] std::size_t halfedges_size() const noexcept { return halfedge_props_.size(); }
        [[nodiscard]] std::size_t edges_size() const noexcept { return edge_props_.size(); }
        [[nodiscard]] std::size_t faces_size() const noexcept { return face_props_.size(); }

        [[nodiscard]] std::size_t vertex_count() const noexcept { return vertices_size() - deleted_vertices_; }
        [[nodiscard]] std::size_t halfedge_count() const noexcept { return halfedges_size() - 2 * deleted_edges_; }
        [[nodiscard]] std::size_t edge_count() const noexcept { return edges_size() - deleted_edges_; }
        [[nodiscard]] std::size_t face_count() const noexcept { return faces_size() - deleted_faces_; }

        [[nodiscard]] bool is_empty() const noexcept { return vertex_count() == 0; }

        [[nodiscard]] bool is_deleted(VertexHandle v) const { return vertex_deleted_[v]; }
        [[nodiscard]] bool is_deleted(HalfedgeHandle h) const { return edge_deleted_[edge(h)]; }
        [[nodiscard]] bool is_deleted(EdgeHandle e) const { return edge_deleted_[e]; }
        [[nodiscard]] bool is_deleted(FaceHandle f) const { return face_deleted_[f]; }

        [[nodiscard]] bool is_valid(VertexHandle v) const { return v.index() < vertices_size(); }
        [[nodiscard]] bool is_valid(HalfedgeHandle h) const { return h.index() < halfedges_size(); }
        [[nodiscard]] bool is_valid(EdgeHandle e) const { return e.index() < edges_size(); }
        [[nodiscard]] bool is_valid(FaceHandle f) const { return f.index() < faces_size(); }

        [[nodiscard]] HalfedgeHandle halfedge(VertexHandle v) const { return vertex_connectivity_[v].halfedge; }
        void set_halfedge(VertexHandle v, HalfedgeHandle h) { vertex_connectivity_[v].halfedge = h; }

        [[nodiscard]] bool is_boundary(VertexHandle v) const
        {
            const HalfedgeHandle h = halfedge(v);
            return !(h.is_valid() && face(h).is_valid());
        }

        [[nodiscard]] bool is_isolated(VertexHandle v) const { return !halfedge(v).is_valid(); }

        [[nodiscard]] bool is_manifold(VertexHandle v) const;

        [[nodiscard]] VertexHandle to_vertex(HalfedgeHandle h) const { return halfedge_connectivity_[h].vertex; }
        [[nodiscard]] VertexHandle from_vertex(HalfedgeHandle h) const { return to_vertex(opposite_halfedge(h)); }
        void set_vertex(HalfedgeHandle h, VertexHandle v) { halfedge_connectivity_[h].vertex = v; }

        [[nodiscard]] FaceHandle face(HalfedgeHandle h) const { return halfedge_connectivity_[h].face; }
        void set_face(HalfedgeHandle h, FaceHandle f) { halfedge_connectivity_[h].face = f; }

        [[nodiscard]] HalfedgeHandle next_halfedge(HalfedgeHandle h) const { return halfedge_connectivity_[h].next; }

        void set_next_halfedge(HalfedgeHandle h, HalfedgeHandle next)
        {
            halfedge_connectivity_[h].next = next;
            halfedge_connectivity_[next].prev = h;
        }

        void set_prev_halfedge(HalfedgeHandle h, HalfedgeHandle prev)
        {
            halfedge_connectivity_[h].prev = prev;
            halfedge_connectivity_[prev].next = h;
        }

        [[nodiscard]] HalfedgeHandle prev_halfedge(HalfedgeHandle h) const { return halfedge_connectivity_[h].prev; }

        [[nodiscard]] HalfedgeHandle opposite_halfedge(HalfedgeHandle h) const
        {
            return HalfedgeHandle((h.index() & 1U) ? h.index() - 1U : h.index() + 1U);
        }

        [[nodiscard]] HalfedgeHandle ccw_rotated_halfedge(HalfedgeHandle h) const
        {
            return opposite_halfedge(prev_halfedge(h));
        }

        [[nodiscard]] HalfedgeHandle cw_rotated_halfedge(HalfedgeHandle h) const
        {
            return next_halfedge(opposite_halfedge(h));
        }

        [[nodiscard]] EdgeHandle edge(HalfedgeHandle h) const { return EdgeHandle(h.index() >> 1U); }
        [[nodiscard]] bool is_boundary(HalfedgeHandle h) const { return !face(h).is_valid(); }

        [[nodiscard]] HalfedgeHandle halfedge(EdgeHandle e, unsigned int i) const;

        [[nodiscard]] VertexHandle vertex(EdgeHandle e, unsigned int i) const { return to_vertex(halfedge(e, i)); }
        [[nodiscard]] FaceHandle face(EdgeHandle e, unsigned int i) const { return face(halfedge(e, i)); }

        [[nodiscard]] bool is_boundary(EdgeHandle e) const;

        [[nodiscard]] HalfedgeHandle halfedge(FaceHandle f) const { return face_connectivity_[f].halfedge; }
        void set_halfedge(FaceHandle f, HalfedgeHandle h) { face_connectivity_[f].halfedge = h; }

        [[nodiscard]] bool is_boundary(FaceHandle f) const;

        template <class T>
        [[nodiscard]] VertexProperty<T> add_vertex_property(const std::string& name, T default_value = T())
        {
            return VertexProperty<T>(vertex_props_.add<T>(name, default_value));
        }

        template <class T>
        [[nodiscard]] VertexProperty<T> get_vertex_property(const std::string& name) const
        {
            return VertexProperty<T>(vertex_props_.get<T>(name));
        }

        template <class T>
        [[nodiscard]] VertexProperty<T> vertex_property(const std::string& name, T default_value = T())
        {
            return VertexProperty<T>(vertex_props_.get_or_add<T>(name, default_value));
        }

        template <class T>
        void remove_vertex_property(VertexProperty<T>& prop)
        {
            vertex_props_.remove(prop);
        }

        [[nodiscard]] bool has_vertex_property(const std::string& name) const { return vertex_props_.exists(name); }

        template <class T>
        [[nodiscard]] HalfedgeProperty<T> add_halfedge_property(const std::string& name, T default_value = T())
        {
            return HalfedgeProperty<T>(halfedge_props_.add<T>(name, default_value));
        }

        template <class T>
        [[nodiscard]] EdgeProperty<T> add_edge_property(const std::string& name, T default_value = T())
        {
            return EdgeProperty<T>(edge_props_.add<T>(name, default_value));
        }

        template <class T>
        [[nodiscard]] HalfedgeProperty<T> get_halfedge_property(const std::string& name) const
        {
            return HalfedgeProperty<T>(halfedge_props_.get<T>(name));
        }

        template <class T>
        [[nodiscard]] EdgeProperty<T> get_edge_property(const std::string& name) const
        {
            return EdgeProperty<T>(edge_props_.get<T>(name));
        }

        template <class T>
        [[nodiscard]] HalfedgeProperty<T> halfedge_property(const std::string& name, T default_value = T())
        {
            return HalfedgeProperty<T>(halfedge_props_.get_or_add<T>(name, default_value));
        }

        template <class T>
        [[nodiscard]] EdgeProperty<T> edge_property(const std::string& name, T default_value = T())
        {
            return EdgeProperty<T>(edge_props_.get_or_add<T>(name, default_value));
        }

        template <class T>
        void remove_halfedge_property(HalfedgeProperty<T>& prop)
        {
            halfedge_props_.remove(prop);
        }

        [[nodiscard]] bool has_halfedge_property(const std::string& name) const { return halfedge_props_.exists(name); }

        template <class T>
        void remove_edge_property(EdgeProperty<T>& prop)
        {
            edge_props_.remove(prop);
        }

        [[nodiscard]] bool has_edge_property(const std::string& name) const { return edge_props_.exists(name); }

        template <class T>
        [[nodiscard]] FaceProperty<T> add_face_property(const std::string& name, T default_value = T())
        {
            return FaceProperty<T>(face_props_.add<T>(name, default_value));
        }

        template <class T>
        [[nodiscard]] FaceProperty<T> get_face_property(const std::string& name) const
        {
            return FaceProperty<T>(face_props_.get<T>(name));
        }

        template <class T>
        [[nodiscard]] FaceProperty<T> face_property(const std::string& name, T default_value = T())
        {
            return FaceProperty<T>(face_props_.get_or_add<T>(name, default_value));
        }

        template <class T>
        void remove_face_property(FaceProperty<T>& prop)
        {
            face_props_.remove(prop);
        }

        [[nodiscard]] bool has_face_property(const std::string& name) const { return face_props_.exists(name); }

        [[nodiscard]] std::vector<std::string> vertex_properties() const { return vertex_props_.properties(); }
        [[nodiscard]] std::vector<std::string> halfedge_properties() const { return halfedge_props_.properties(); }
        [[nodiscard]] std::vector<std::string> edge_properties() const { return edge_props_.properties(); }
        [[nodiscard]] std::vector<std::string> face_properties() const { return face_props_.properties(); }

        [[nodiscard]] VertexIterator vertices_begin() const;

        [[nodiscard]] VertexIterator vertices_end() const;

        [[nodiscard]] VertexRange vertices() const { return {vertices_begin(), vertices_end()}; }

        [[nodiscard]] HalfedgeIterator halfedges_begin() const;

        [[nodiscard]] HalfedgeIterator halfedges_end() const;

        [[nodiscard]] HalfedgeRange halfedges() const { return {halfedges_begin(), halfedges_end()}; }

        [[nodiscard]] EdgeIterator edges_begin() const;

        [[nodiscard]] EdgeIterator edges_end() const;

        [[nodiscard]] EdgeRange edges() const { return {edges_begin(), edges_end()}; }

        [[nodiscard]] FaceIterator faces_begin() const;

        [[nodiscard]] FaceIterator faces_end() const;

        [[nodiscard]] FaceRange faces() const { return {faces_begin(), faces_end()}; }

        [[nodiscard]] VertexAroundVertexCirculator<HalfedgeMeshInterface> vertices(VertexHandle v) const
        {
            return {this, v};
        }

        [[nodiscard]] EdgeAroundVertexCirculator<HalfedgeMeshInterface> edges(VertexHandle v) const
        {
            return {this, v};
        }

        [[nodiscard]] HalfedgeAroundVertexCirculator<HalfedgeMeshInterface> halfedges(VertexHandle v) const
        {
            return {this, v};
        }

        [[nodiscard]] FaceAroundVertexCirculator<HalfedgeMeshInterface> faces(VertexHandle v) const
        {
            return {this, v};
        }

        [[nodiscard]] VertexAroundFaceCirculator<HalfedgeMeshInterface> vertices(FaceHandle f) const
        {
            return {this, f};
        }

        [[nodiscard]] HalfedgeAroundFaceCirculator<HalfedgeMeshInterface> halfedges(FaceHandle f) const
        {
            return {this, f};
        }

        [[nodiscard]] HalfedgeHandle insert_vertex(EdgeHandle e, const math::vec3& p);

        [[nodiscard]] HalfedgeHandle insert_vertex(EdgeHandle e, VertexHandle v);

        [[nodiscard]] HalfedgeHandle insert_vertex(HalfedgeHandle h, VertexHandle v);

        [[nodiscard]] std::optional<HalfedgeHandle> find_halfedge(VertexHandle start, VertexHandle end) const;

        [[nodiscard]] std::optional<EdgeHandle> find_edge(VertexHandle a, VertexHandle b) const;

        [[nodiscard]] bool is_triangle_mesh() const;

        [[nodiscard]] bool is_quad_mesh() const;

        [[nodiscard]] bool is_collapse_ok(HalfedgeHandle h) const;

        void collapse(HalfedgeHandle h);

        [[nodiscard]] bool is_removal_ok(EdgeHandle e) const;

        bool remove_edge(EdgeHandle e);

        [[nodiscard]] VertexHandle split(FaceHandle f, const math::vec3& p);

        void split(FaceHandle f, VertexHandle v);

        [[nodiscard]] HalfedgeHandle split(EdgeHandle e, const math::vec3& p);

        [[nodiscard]] HalfedgeHandle split(EdgeHandle e, VertexHandle v);

        [[nodiscard]] HalfedgeHandle insert_edge(HalfedgeHandle h0, HalfedgeHandle h1);

        [[nodiscard]] bool is_flip_ok(EdgeHandle e) const;

        void flip(EdgeHandle e);

        [[nodiscard]] std::size_t valence(VertexHandle v) const;

        [[nodiscard]] std::size_t valence(FaceHandle f) const;

        void delete_vertex(VertexHandle v);

        void delete_edge(EdgeHandle e);

        void delete_face(FaceHandle f);

        [[nodiscard]] const math::vec3& position(VertexHandle v) const { return vertex_points_[v]; }
        [[nodiscard]] math::vec3& position(VertexHandle v) { return vertex_points_[v]; }
        [[nodiscard]] std::span<const math::vec3> positions() const { return vertex_points_.span(); }
        [[nodiscard]] std::span<math::vec3> positions() { return vertex_points_.span(); }

        [[nodiscard]] VertexHandle new_vertex();

        [[nodiscard]] HalfedgeHandle new_edge();

        [[nodiscard]] HalfedgeHandle new_edge(VertexHandle start, VertexHandle end);

        [[nodiscard]] FaceHandle new_face();

        [[nodiscard]] bool has_garbage() const noexcept { return has_garbage_; }

    private:
        void ensure_properties();

        void adjust_outgoing_halfedge(VertexHandle v);

        void remove_edge_helper(HalfedgeHandle h);

        void remove_loop_helper(HalfedgeHandle h);

        friend ENGINE_GEOMETRY_API void read(HalfedgeMeshInterface&, const std::filesystem::path&);

        friend ENGINE_GEOMETRY_API void write(const HalfedgeMeshInterface&, const std::filesystem::path&, const IOFlags&);

        Vertices& vertex_props_;
        Halfedges& halfedge_props_;
        Edges& edge_props_;
        Faces& face_props_;

        VertexProperty<math::vec3> vertex_points_;
        VertexProperty<VertexConnectivity> vertex_connectivity_;
        HalfedgeProperty<HalfedgeConnectivity> halfedge_connectivity_;
        FaceProperty<FaceConnectivity> face_connectivity_;

        VertexProperty<bool> vertex_deleted_;
        EdgeProperty<bool> edge_deleted_;
        FaceProperty<bool> face_deleted_;

        PropertyIndex deleted_vertices_{0};
        PropertyIndex deleted_edges_{0};
        PropertyIndex deleted_faces_{0};

        bool has_garbage_{false};

        using NextCacheEntry = std::pair<HalfedgeHandle, HalfedgeHandle>;
        using NextCache = std::vector<NextCacheEntry>;
        std::vector<VertexHandle> add_face_vertices_;
        std::vector<HalfedgeHandle> add_face_halfedges_;
        std::vector<bool> add_face_is_new_;
        std::vector<bool> add_face_needs_adjust_;
        NextCache add_face_next_cache_;
    };
} // namespace engine::geometry

namespace engine::geometry
{
    using MeshInterface = mesh::HalfedgeMeshInterface;

    struct ENGINE_GEOMETRY_API MeshData
    {
        Vertices vertex_props;
        Halfedges halfedge_props;
        Edges edge_props;
        Faces face_props;
    };

    struct ENGINE_GEOMETRY_API Mesh
    {
        Mesh() : data(), interface(data.vertex_props, data.halfedge_props, data.edge_props, data.face_props)
        {
        }

        Mesh(const Mesh& rhs) : Mesh()
        {
            interface = rhs.interface;
        }

        Mesh(Mesh&& rhs) noexcept : Mesh()
        {
            interface = rhs.interface;
            rhs.interface.clear();
        }

        Mesh& operator=(const Mesh& rhs)
        {
            if (this != &rhs)
            {
                interface = rhs.interface;
            }
            return *this;
        }

        Mesh& operator=(Mesh&& rhs) noexcept
        {
            if (this != &rhs)
            {
                interface = rhs.interface;
                rhs.interface.clear();
            }
            return *this;
        }

        MeshData data;
        MeshInterface interface;
    };
}
