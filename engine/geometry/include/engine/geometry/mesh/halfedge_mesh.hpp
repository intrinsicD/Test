#pragma once

#include "engine/geometry/property_registry.hpp"
#include "engine/geometry/property_set.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <filesystem>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::geometry {

class HalfedgeMesh;

using MeshIndex = std::uint32_t;

constexpr MeshIndex kInvalidMeshIndex = std::numeric_limits<MeshIndex>::max();

class MeshHandle {
public:
    using index_type = MeshIndex;

    constexpr MeshHandle() noexcept = default;
    explicit constexpr MeshHandle(index_type idx) noexcept : index_(idx) {}

    [[nodiscard]] constexpr index_type index() const noexcept { return index_; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return index_ != kInvalidMeshIndex; }
    constexpr void reset() noexcept { index_ = kInvalidMeshIndex; }

    [[nodiscard]] auto operator<=>(const MeshHandle&) const noexcept = default;

protected:
    index_type index_{kInvalidMeshIndex};
};

class VertexHandle final : public MeshHandle {
public:
    using MeshHandle::MeshHandle;
};

class HalfedgeHandle final : public MeshHandle {
public:
    using MeshHandle::MeshHandle;
};

class EdgeHandle final : public MeshHandle {
public:
    using MeshHandle::MeshHandle;
};

class FaceHandle final : public MeshHandle {
public:
    using MeshHandle::MeshHandle;
};

std::ostream& operator<<(std::ostream& os, VertexHandle v);
std::ostream& operator<<(std::ostream& os, HalfedgeHandle h);
std::ostream& operator<<(std::ostream& os, EdgeHandle e);
std::ostream& operator<<(std::ostream& os, FaceHandle f);

template <class T>
using VertexProperty = HandleProperty<VertexHandle, T>;

template <class T>
using HalfedgeProperty = HandleProperty<HalfedgeHandle, T>;

template <class T>
using EdgeProperty = HandleProperty<EdgeHandle, T>;

template <class T>
using FaceProperty = HandleProperty<FaceHandle, T>;

// Halfedge mesh -----------------------------------------------------------------------------------------------------

using Point3 = math::vec3;

struct IOFlags;

class HalfedgeMesh {
public:
    HalfedgeMesh();
    HalfedgeMesh(const HalfedgeMesh& rhs);
    HalfedgeMesh(HalfedgeMesh&&) noexcept = default;
    ~HalfedgeMesh();

    HalfedgeMesh& operator=(const HalfedgeMesh& rhs);
    HalfedgeMesh& operator=(HalfedgeMesh&&) noexcept = default;

    HalfedgeMesh& assign(const HalfedgeMesh& rhs);

    // Handle iterators ----------------------------------------------------------------------------------------------
    class VertexIterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = VertexHandle;
        using reference = VertexHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        VertexIterator() = default;
        VertexIterator(VertexHandle v, const HalfedgeMesh* mesh);

        [[nodiscard]] VertexHandle operator*() const { return handle_; }
        [[nodiscard]] auto operator<=>(const VertexIterator&) const = default;

        VertexIterator& operator++();
        VertexIterator operator++(int);
        VertexIterator& operator--();
        VertexIterator operator--(int);

    private:
        VertexHandle handle_{};
        const HalfedgeMesh* mesh_{nullptr};
    };

    class HalfedgeIterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HalfedgeHandle;
        using reference = HalfedgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        HalfedgeIterator() = default;
        HalfedgeIterator(HalfedgeHandle h, const HalfedgeMesh* mesh);

        [[nodiscard]] HalfedgeHandle operator*() const { return handle_; }
        [[nodiscard]] auto operator<=>(const HalfedgeIterator&) const = default;

        HalfedgeIterator& operator++();
        HalfedgeIterator operator++(int);
        HalfedgeIterator& operator--();
        HalfedgeIterator operator--(int);

    private:
        HalfedgeHandle handle_{};
        const HalfedgeMesh* mesh_{nullptr};
    };

    class EdgeIterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = EdgeHandle;
        using reference = EdgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        EdgeIterator() = default;
        EdgeIterator(EdgeHandle e, const HalfedgeMesh* mesh);

        [[nodiscard]] EdgeHandle operator*() const { return handle_; }
        [[nodiscard]] auto operator<=>(const EdgeIterator&) const = default;

        EdgeIterator& operator++();
        EdgeIterator operator++(int);
        EdgeIterator& operator--();
        EdgeIterator operator--(int);

    private:
        EdgeHandle handle_{};
        const HalfedgeMesh* mesh_{nullptr};
    };

    class FaceIterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = FaceHandle;
        using reference = FaceHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        FaceIterator() = default;
        FaceIterator(FaceHandle f, const HalfedgeMesh* mesh);

        [[nodiscard]] FaceHandle operator*() const { return handle_; }
        [[nodiscard]] auto operator<=>(const FaceIterator&) const = default;

        FaceIterator& operator++();
        FaceIterator operator++(int);
        FaceIterator& operator--();
        FaceIterator operator--(int);

    private:
        FaceHandle handle_{};
        const HalfedgeMesh* mesh_{nullptr};
    };

    class VertexRange {
    public:
        VertexRange(VertexIterator begin, VertexIterator end) : begin_(begin), end_(end) {}
        [[nodiscard]] VertexIterator begin() const { return begin_; }
        [[nodiscard]] VertexIterator end() const { return end_; }

    private:
        VertexIterator begin_;
        VertexIterator end_;
    };

    class HalfedgeRange {
    public:
        HalfedgeRange(HalfedgeIterator begin, HalfedgeIterator end) : begin_(begin), end_(end) {}
        [[nodiscard]] HalfedgeIterator begin() const { return begin_; }
        [[nodiscard]] HalfedgeIterator end() const { return end_; }

    private:
        HalfedgeIterator begin_;
        HalfedgeIterator end_;
    };

    class EdgeRange {
    public:
        EdgeRange(EdgeIterator begin, EdgeIterator end) : begin_(begin), end_(end) {}
        [[nodiscard]] EdgeIterator begin() const { return begin_; }
        [[nodiscard]] EdgeIterator end() const { return end_; }

    private:
        EdgeIterator begin_;
        EdgeIterator end_;
    };

    class FaceRange {
    public:
        FaceRange(FaceIterator begin, FaceIterator end) : begin_(begin), end_(end) {}
        [[nodiscard]] FaceIterator begin() const { return begin_; }
        [[nodiscard]] FaceIterator end() const { return end_; }

    private:
        FaceIterator begin_;
        FaceIterator end_;
    };

    // Circulators ----------------------------------------------------------------------------------------------------

    class VertexAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = VertexHandle;
        using reference = VertexHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        VertexAroundVertexCirculator() = default;
        VertexAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v);

        bool operator==(const VertexAroundVertexCirculator& rhs) const;
        bool operator!=(const VertexAroundVertexCirculator& rhs) const { return !(*this == rhs); }

        VertexAroundVertexCirculator& operator++();
        VertexAroundVertexCirculator operator++(int);
        VertexAroundVertexCirculator& operator--();
        VertexAroundVertexCirculator operator--(int);

        [[nodiscard]] VertexHandle operator*() const;
        explicit operator bool() const { return halfedge_.is_valid(); }

        VertexAroundVertexCirculator& begin();
        VertexAroundVertexCirculator& end();

        [[nodiscard]] HalfedgeHandle halfedge() const { return halfedge_; }

    private:
        const HalfedgeMesh* mesh_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    class HalfedgeAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HalfedgeHandle;
        using reference = HalfedgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        HalfedgeAroundVertexCirculator() = default;
        HalfedgeAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v);

        bool operator==(const HalfedgeAroundVertexCirculator& rhs) const;
        bool operator!=(const HalfedgeAroundVertexCirculator& rhs) const { return !(*this == rhs); }

        HalfedgeAroundVertexCirculator& operator++();
        HalfedgeAroundVertexCirculator operator++(int);
        HalfedgeAroundVertexCirculator& operator--();
        HalfedgeAroundVertexCirculator operator--(int);

        [[nodiscard]] HalfedgeHandle operator*() const { return halfedge_; }
        explicit operator bool() const { return halfedge_.is_valid(); }

        HalfedgeAroundVertexCirculator& begin();
        HalfedgeAroundVertexCirculator& end();

    private:
        const HalfedgeMesh* mesh_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    class EdgeAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = EdgeHandle;
        using reference = EdgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        EdgeAroundVertexCirculator() = default;
        EdgeAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v);

        bool operator==(const EdgeAroundVertexCirculator& rhs) const;
        bool operator!=(const EdgeAroundVertexCirculator& rhs) const { return !(*this == rhs); }

        EdgeAroundVertexCirculator& operator++();
        EdgeAroundVertexCirculator operator++(int);
        EdgeAroundVertexCirculator& operator--();
        EdgeAroundVertexCirculator operator--(int);

        [[nodiscard]] EdgeHandle operator*() const;
        explicit operator bool() const { return halfedge_.is_valid(); }

        EdgeAroundVertexCirculator& begin();
        EdgeAroundVertexCirculator& end();

    private:
        const HalfedgeMesh* mesh_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    class FaceAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = FaceHandle;
        using reference = FaceHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        FaceAroundVertexCirculator() = default;
        FaceAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v);

        bool operator==(const FaceAroundVertexCirculator& rhs) const;
        bool operator!=(const FaceAroundVertexCirculator& rhs) const { return !(*this == rhs); }

        FaceAroundVertexCirculator& operator++();
        FaceAroundVertexCirculator operator++(int);
        FaceAroundVertexCirculator& operator--();
        FaceAroundVertexCirculator operator--(int);

        [[nodiscard]] FaceHandle operator*() const;
        explicit operator bool() const { return halfedge_.is_valid(); }

        FaceAroundVertexCirculator& begin();
        FaceAroundVertexCirculator& end();

    private:
        const HalfedgeMesh* mesh_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    class VertexAroundFaceCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = VertexHandle;
        using reference = VertexHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        VertexAroundFaceCirculator() = default;
        VertexAroundFaceCirculator(const HalfedgeMesh* mesh, FaceHandle f);

        bool operator==(const VertexAroundFaceCirculator& rhs) const;
        bool operator!=(const VertexAroundFaceCirculator& rhs) const { return !(*this == rhs); }

        VertexAroundFaceCirculator& operator++();
        VertexAroundFaceCirculator operator++(int);
        VertexAroundFaceCirculator& operator--();
        VertexAroundFaceCirculator operator--(int);

        [[nodiscard]] VertexHandle operator*() const;

        VertexAroundFaceCirculator& begin();
        VertexAroundFaceCirculator& end();

    private:
        const HalfedgeMesh* mesh_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    class HalfedgeAroundFaceCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HalfedgeHandle;
        using reference = HalfedgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;

        HalfedgeAroundFaceCirculator() = default;
        HalfedgeAroundFaceCirculator(const HalfedgeMesh* mesh, FaceHandle f);

        bool operator==(const HalfedgeAroundFaceCirculator& rhs) const;
        bool operator!=(const HalfedgeAroundFaceCirculator& rhs) const { return !(*this == rhs); }

        HalfedgeAroundFaceCirculator& operator++();
        HalfedgeAroundFaceCirculator operator++(int);
        HalfedgeAroundFaceCirculator& operator--();
        HalfedgeAroundFaceCirculator operator--(int);

        [[nodiscard]] HalfedgeHandle operator*() const { return halfedge_; }

        HalfedgeAroundFaceCirculator& begin();
        HalfedgeAroundFaceCirculator& end();

    private:
        const HalfedgeMesh* mesh_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    // Construction ---------------------------------------------------------------------------------------------------

    [[nodiscard]] VertexHandle add_vertex(const Point3& p);
    [[nodiscard]] std::optional<FaceHandle> add_face(std::span<const VertexHandle> vertices);
    [[nodiscard]] std::optional<FaceHandle> add_triangle(VertexHandle v0, VertexHandle v1, VertexHandle v2);
    [[nodiscard]] std::optional<FaceHandle> add_quad(VertexHandle v0, VertexHandle v1, VertexHandle v2, VertexHandle v3);

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

    [[nodiscard]] bool is_valid(VertexHandle v) const { return v.is_valid() && v.index() < vertices_size(); }
    [[nodiscard]] bool is_valid(HalfedgeHandle h) const { return h.is_valid() && h.index() < halfedges_size(); }
    [[nodiscard]] bool is_valid(EdgeHandle e) const { return e.is_valid() && e.index() < edges_size(); }
    [[nodiscard]] bool is_valid(FaceHandle f) const { return f.is_valid() && f.index() < faces_size(); }

    [[nodiscard]] HalfedgeHandle halfedge(VertexHandle v) const { return vertex_connectivity_[v].halfedge; }
    void set_halfedge(VertexHandle v, HalfedgeHandle h) { vertex_connectivity_[v].halfedge = h; }

    [[nodiscard]] bool is_boundary(VertexHandle v) const;
    [[nodiscard]] bool is_isolated(VertexHandle v) const { return !halfedge(v).is_valid(); }
    [[nodiscard]] bool is_manifold(VertexHandle v) const;

    [[nodiscard]] VertexHandle to_vertex(HalfedgeHandle h) const { return halfedge_connectivity_[h].vertex; }
    [[nodiscard]] VertexHandle from_vertex(HalfedgeHandle h) const { return to_vertex(opposite_halfedge(h)); }
    void set_vertex(HalfedgeHandle h, VertexHandle v) { halfedge_connectivity_[h].vertex = v; }

    [[nodiscard]] FaceHandle face(HalfedgeHandle h) const { return halfedge_connectivity_[h].face; }
    void set_face(HalfedgeHandle h, FaceHandle f) { halfedge_connectivity_[h].face = f; }

    [[nodiscard]] HalfedgeHandle next_halfedge(HalfedgeHandle h) const { return halfedge_connectivity_[h].next; }
    void set_next_halfedge(HalfedgeHandle h, HalfedgeHandle next);
    void set_prev_halfedge(HalfedgeHandle h, HalfedgeHandle prev);
    [[nodiscard]] HalfedgeHandle prev_halfedge(HalfedgeHandle h) const { return halfedge_connectivity_[h].prev; }

    [[nodiscard]] HalfedgeHandle opposite_halfedge(HalfedgeHandle h) const { return HalfedgeHandle((h.index() & 1U) ? h.index() - 1U : h.index() + 1U); }
    [[nodiscard]] HalfedgeHandle ccw_rotated_halfedge(HalfedgeHandle h) const { return opposite_halfedge(prev_halfedge(h)); }
    [[nodiscard]] HalfedgeHandle cw_rotated_halfedge(HalfedgeHandle h) const { return next_halfedge(opposite_halfedge(h)); }

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
    [[nodiscard]] VertexRange vertices() const { return VertexRange(vertices_begin(), vertices_end()); }

    [[nodiscard]] HalfedgeIterator halfedges_begin() const;
    [[nodiscard]] HalfedgeIterator halfedges_end() const;
    [[nodiscard]] HalfedgeRange halfedges() const { return HalfedgeRange(halfedges_begin(), halfedges_end()); }

    [[nodiscard]] EdgeIterator edges_begin() const;
    [[nodiscard]] EdgeIterator edges_end() const;
    [[nodiscard]] EdgeRange edges() const { return EdgeRange(edges_begin(), edges_end()); }

    [[nodiscard]] FaceIterator faces_begin() const;
    [[nodiscard]] FaceIterator faces_end() const;
    [[nodiscard]] FaceRange faces() const { return FaceRange(faces_begin(), faces_end()); }

    [[nodiscard]] VertexAroundVertexCirculator vertices(VertexHandle v) const { return VertexAroundVertexCirculator(this, v); }
    [[nodiscard]] EdgeAroundVertexCirculator edges(VertexHandle v) const { return EdgeAroundVertexCirculator(this, v); }
    [[nodiscard]] HalfedgeAroundVertexCirculator halfedges(VertexHandle v) const { return HalfedgeAroundVertexCirculator(this, v); }
    [[nodiscard]] FaceAroundVertexCirculator faces(VertexHandle v) const { return FaceAroundVertexCirculator(this, v); }

    [[nodiscard]] VertexAroundFaceCirculator vertices(FaceHandle f) const { return VertexAroundFaceCirculator(this, f); }
    [[nodiscard]] HalfedgeAroundFaceCirculator halfedges(FaceHandle f) const { return HalfedgeAroundFaceCirculator(this, f); }

    [[nodiscard]] HalfedgeHandle insert_vertex(EdgeHandle e, const Point3& p);
    [[nodiscard]] HalfedgeHandle insert_vertex(EdgeHandle e, VertexHandle v);
    [[nodiscard]] HalfedgeHandle insert_vertex(HalfedgeHandle h, VertexHandle v);

    [[nodiscard]] HalfedgeHandle find_halfedge(VertexHandle start, VertexHandle end) const;
    [[nodiscard]] EdgeHandle find_edge(VertexHandle a, VertexHandle b) const;

    [[nodiscard]] bool is_triangle_mesh() const;
    [[nodiscard]] bool is_quad_mesh() const;

    [[nodiscard]] bool is_collapse_ok(HalfedgeHandle h) const;
    void collapse(HalfedgeHandle h);

    [[nodiscard]] bool is_removal_ok(EdgeHandle e) const;
    bool remove_edge(EdgeHandle e);

    [[nodiscard]] VertexHandle split(FaceHandle f, const Point3& p);
    void split(FaceHandle f, VertexHandle v);

    [[nodiscard]] HalfedgeHandle split(EdgeHandle e, const Point3& p);
    [[nodiscard]] HalfedgeHandle split(EdgeHandle e, VertexHandle v);

    [[nodiscard]] HalfedgeHandle insert_edge(HalfedgeHandle h0, HalfedgeHandle h1);

    [[nodiscard]] bool is_flip_ok(EdgeHandle e) const;
    void flip(EdgeHandle e);

    [[nodiscard]] std::size_t valence(VertexHandle v) const;
    [[nodiscard]] std::size_t valence(FaceHandle f) const;

    void delete_vertex(VertexHandle v);
    void delete_edge(EdgeHandle e);
    void delete_face(FaceHandle f);

    [[nodiscard]] const Point3& position(VertexHandle v) const { return vertex_points_[v]; }
    [[nodiscard]] Point3& position(VertexHandle v) { return vertex_points_[v]; }
    [[nodiscard]] std::vector<Point3>& positions() { return vertex_points_.vector(); }

    [[nodiscard]] VertexHandle new_vertex();
    [[nodiscard]] HalfedgeHandle new_edge();
    [[nodiscard]] HalfedgeHandle new_edge(VertexHandle start, VertexHandle end);
    [[nodiscard]] FaceHandle new_face();

private:
    struct VertexConnectivity {
        HalfedgeHandle halfedge{};
    };

    struct HalfedgeConnectivity {
        FaceHandle face{};
        VertexHandle vertex{};
        HalfedgeHandle next{};
        HalfedgeHandle prev{};
    };

    struct FaceConnectivity {
        HalfedgeHandle halfedge{};
    };

    void adjust_outgoing_halfedge(VertexHandle v);
    void remove_edge_helper(HalfedgeHandle h);
    void remove_loop_helper(HalfedgeHandle h);

    [[nodiscard]] bool has_garbage() const noexcept { return has_garbage_; }

    friend void read_pmp(HalfedgeMesh&, const std::filesystem::path&);
    friend void write_pmp(const HalfedgeMesh&, const std::filesystem::path&, const IOFlags&);

    MeshPropertySet vertex_props_;
    MeshPropertySet halfedge_props_;
    MeshPropertySet edge_props_;
    MeshPropertySet face_props_;

    VertexProperty<Point3> vertex_points_;
    VertexProperty<VertexConnectivity> vertex_connectivity_;
    HalfedgeProperty<HalfedgeConnectivity> halfedge_connectivity_;
    FaceProperty<FaceConnectivity> face_connectivity_;

    VertexProperty<bool> vertex_deleted_;
    EdgeProperty<bool> edge_deleted_;
    FaceProperty<bool> face_deleted_;

    MeshIndex deleted_vertices_{0};
    MeshIndex deleted_edges_{0};
    MeshIndex deleted_faces_{0};

    bool has_garbage_{false};

    using NextCacheEntry = std::pair<HalfedgeHandle, HalfedgeHandle>;
    using NextCache = std::vector<NextCacheEntry>;
    std::vector<VertexHandle> add_face_vertices_;
    std::vector<HalfedgeHandle> add_face_halfedges_;
    std::vector<bool> add_face_is_new_;
    std::vector<bool> add_face_needs_adjust_;
    NextCache add_face_next_cache_;
};

// Inline implementations --------------------------------------------------------------------------------------------

} // namespace engine::geometry

