#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include <algorithm>

namespace engine::geometry {

std::ostream& operator<<(std::ostream& os, VertexHandle v)
{
    return os << 'v' << v.index();
}

std::ostream& operator<<(std::ostream& os, HalfedgeHandle h)
{
    return os << 'h' << h.index();
}

std::ostream& operator<<(std::ostream& os, EdgeHandle e)
{
    return os << 'e' << e.index();
}

std::ostream& operator<<(std::ostream& os, FaceHandle f)
{
    return os << 'f' << f.index();
}

HalfedgeMesh::VertexIterator::VertexIterator(VertexHandle v, const HalfedgeMesh* mesh)
    : handle_(v), mesh_(mesh)
{
    if (mesh_ && mesh_->has_garbage())
    {
        while (mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = VertexHandle(handle_.index() + 1);
        }
    }
}

HalfedgeMesh::VertexIterator& HalfedgeMesh::VertexIterator::operator++()
{
    handle_ = VertexHandle(handle_.index() + 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = VertexHandle(handle_.index() + 1);
        }
    }
    return *this;
}

HalfedgeMesh::VertexIterator HalfedgeMesh::VertexIterator::operator++(int)
{
    VertexIterator tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::VertexIterator& HalfedgeMesh::VertexIterator::operator--()
{
    handle_ = VertexHandle(handle_.index() - 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = VertexHandle(handle_.index() - 1);
        }
    }
    return *this;
}

HalfedgeMesh::VertexIterator HalfedgeMesh::VertexIterator::operator--(int)
{
    VertexIterator tmp = *this;
    --(*this);
    return tmp;
}

HalfedgeMesh::HalfedgeIterator::HalfedgeIterator(HalfedgeHandle h, const HalfedgeMesh* mesh)
    : handle_(h), mesh_(mesh)
{
    if (mesh_ && mesh_->has_garbage())
    {
        while (mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = HalfedgeHandle(handle_.index() + 1);
        }
    }
}

HalfedgeMesh::HalfedgeIterator& HalfedgeMesh::HalfedgeIterator::operator++()
{
    handle_ = HalfedgeHandle(handle_.index() + 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = HalfedgeHandle(handle_.index() + 1);
        }
    }
    return *this;
}

HalfedgeMesh::HalfedgeIterator HalfedgeMesh::HalfedgeIterator::operator++(int)
{
    HalfedgeIterator tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::HalfedgeIterator& HalfedgeMesh::HalfedgeIterator::operator--()
{
    handle_ = HalfedgeHandle(handle_.index() - 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = HalfedgeHandle(handle_.index() - 1);
        }
    }
    return *this;
}

HalfedgeMesh::HalfedgeIterator HalfedgeMesh::HalfedgeIterator::operator--(int)
{
    HalfedgeIterator tmp = *this;
    --(*this);
    return tmp;
}

HalfedgeMesh::EdgeIterator::EdgeIterator(EdgeHandle e, const HalfedgeMesh* mesh)
    : handle_(e), mesh_(mesh)
{
    if (mesh_ && mesh_->has_garbage())
    {
        while (mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = EdgeHandle(handle_.index() + 1);
        }
    }
}

HalfedgeMesh::EdgeIterator& HalfedgeMesh::EdgeIterator::operator++()
{
    handle_ = EdgeHandle(handle_.index() + 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = EdgeHandle(handle_.index() + 1);
        }
    }
    return *this;
}

HalfedgeMesh::EdgeIterator HalfedgeMesh::EdgeIterator::operator++(int)
{
    EdgeIterator tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::EdgeIterator& HalfedgeMesh::EdgeIterator::operator--()
{
    handle_ = EdgeHandle(handle_.index() - 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = EdgeHandle(handle_.index() - 1);
        }
    }
    return *this;
}

HalfedgeMesh::EdgeIterator HalfedgeMesh::EdgeIterator::operator--(int)
{
    EdgeIterator tmp = *this;
    --(*this);
    return tmp;
}

HalfedgeMesh::FaceIterator::FaceIterator(FaceHandle f, const HalfedgeMesh* mesh)
    : handle_(f), mesh_(mesh)
{
    if (mesh_ && mesh_->has_garbage())
    {
        while (mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = FaceHandle(handle_.index() + 1);
        }
    }
}

HalfedgeMesh::FaceIterator& HalfedgeMesh::FaceIterator::operator++()
{
    handle_ = FaceHandle(handle_.index() + 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = FaceHandle(handle_.index() + 1);
        }
    }
    return *this;
}

HalfedgeMesh::FaceIterator HalfedgeMesh::FaceIterator::operator++(int)
{
    FaceIterator tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::FaceIterator& HalfedgeMesh::FaceIterator::operator--()
{
    handle_ = FaceHandle(handle_.index() - 1);
    if (mesh_)
    {
        while (mesh_->has_garbage() && mesh_->is_valid(handle_) && mesh_->is_deleted(handle_))
        {
            handle_ = FaceHandle(handle_.index() - 1);
        }
    }
    return *this;
}

HalfedgeMesh::FaceIterator HalfedgeMesh::FaceIterator::operator--(int)
{
    FaceIterator tmp = *this;
    --(*this);
    return tmp;
}

HalfedgeMesh::VertexAroundVertexCirculator::VertexAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v)
    : mesh_(mesh)
{
    if (mesh_)
    {
        halfedge_ = mesh_->halfedge(v);
    }
}

bool HalfedgeMesh::VertexAroundVertexCirculator::operator==(const VertexAroundVertexCirculator& rhs) const
{
    assert(mesh_ != nullptr);
    assert(mesh_ == rhs.mesh_);
    return is_active_ && (halfedge_ == rhs.halfedge_);
}

HalfedgeMesh::VertexAroundVertexCirculator& HalfedgeMesh::VertexAroundVertexCirculator::operator++()
{
    assert(mesh_ != nullptr);
    halfedge_ = mesh_->ccw_rotated_halfedge(halfedge_);
    is_active_ = true;
    return *this;
}

HalfedgeMesh::VertexAroundVertexCirculator HalfedgeMesh::VertexAroundVertexCirculator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::VertexAroundVertexCirculator& HalfedgeMesh::VertexAroundVertexCirculator::operator--()
{
    assert(mesh_ != nullptr);
    halfedge_ = mesh_->cw_rotated_halfedge(halfedge_);
    return *this;
}

HalfedgeMesh::VertexAroundVertexCirculator HalfedgeMesh::VertexAroundVertexCirculator::operator--(int)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

VertexHandle HalfedgeMesh::VertexAroundVertexCirculator::operator*() const
{
    assert(mesh_ != nullptr);
    return mesh_->to_vertex(halfedge_);
}

HalfedgeMesh::VertexAroundVertexCirculator& HalfedgeMesh::VertexAroundVertexCirculator::begin()
{
    is_active_ = !halfedge_.is_valid();
    return *this;
}

HalfedgeMesh::VertexAroundVertexCirculator& HalfedgeMesh::VertexAroundVertexCirculator::end()
{
    is_active_ = true;
    return *this;
}

HalfedgeMesh::HalfedgeAroundVertexCirculator::HalfedgeAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v)
    : mesh_(mesh)
{
    if (mesh_)
    {
        halfedge_ = mesh_->halfedge(v);
    }
}

bool HalfedgeMesh::HalfedgeAroundVertexCirculator::operator==(const HalfedgeAroundVertexCirculator& rhs) const
{
    assert(mesh_ != nullptr);
    assert(mesh_ == rhs.mesh_);
    return is_active_ && (halfedge_ == rhs.halfedge_);
}

HalfedgeMesh::HalfedgeAroundVertexCirculator& HalfedgeMesh::HalfedgeAroundVertexCirculator::operator++()
{
    assert(mesh_ != nullptr);
    halfedge_ = mesh_->ccw_rotated_halfedge(halfedge_);
    is_active_ = true;
    return *this;
}

HalfedgeMesh::HalfedgeAroundVertexCirculator HalfedgeMesh::HalfedgeAroundVertexCirculator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::HalfedgeAroundVertexCirculator& HalfedgeMesh::HalfedgeAroundVertexCirculator::operator--()
{
    assert(mesh_ != nullptr);
    halfedge_ = mesh_->cw_rotated_halfedge(halfedge_);
    return *this;
}

HalfedgeMesh::HalfedgeAroundVertexCirculator HalfedgeMesh::HalfedgeAroundVertexCirculator::operator--(int)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

HalfedgeMesh::HalfedgeAroundVertexCirculator& HalfedgeMesh::HalfedgeAroundVertexCirculator::begin()
{
    is_active_ = !halfedge_.is_valid();
    return *this;
}

HalfedgeMesh::HalfedgeAroundVertexCirculator& HalfedgeMesh::HalfedgeAroundVertexCirculator::end()
{
    is_active_ = true;
    return *this;
}

HalfedgeMesh::EdgeAroundVertexCirculator::EdgeAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v)
    : mesh_(mesh)
{
    if (mesh_)
    {
        halfedge_ = mesh_->halfedge(v);
    }
}

bool HalfedgeMesh::EdgeAroundVertexCirculator::operator==(const EdgeAroundVertexCirculator& rhs) const
{
    assert(mesh_ != nullptr);
    assert(mesh_ == rhs.mesh_);
    return is_active_ && (halfedge_ == rhs.halfedge_);
}

HalfedgeMesh::EdgeAroundVertexCirculator& HalfedgeMesh::EdgeAroundVertexCirculator::operator++()
{
    assert(mesh_ != nullptr);
    halfedge_ = mesh_->ccw_rotated_halfedge(halfedge_);
    is_active_ = true;
    return *this;
}

HalfedgeMesh::EdgeAroundVertexCirculator HalfedgeMesh::EdgeAroundVertexCirculator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::EdgeAroundVertexCirculator& HalfedgeMesh::EdgeAroundVertexCirculator::operator--()
{
    assert(mesh_ != nullptr);
    halfedge_ = mesh_->cw_rotated_halfedge(halfedge_);
    return *this;
}

HalfedgeMesh::EdgeAroundVertexCirculator HalfedgeMesh::EdgeAroundVertexCirculator::operator--(int)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

EdgeHandle HalfedgeMesh::EdgeAroundVertexCirculator::operator*() const
{
    assert(mesh_ != nullptr);
    return mesh_->edge(halfedge_);
}

HalfedgeMesh::EdgeAroundVertexCirculator& HalfedgeMesh::EdgeAroundVertexCirculator::begin()
{
    is_active_ = !halfedge_.is_valid();
    return *this;
}

HalfedgeMesh::EdgeAroundVertexCirculator& HalfedgeMesh::EdgeAroundVertexCirculator::end()
{
    is_active_ = true;
    return *this;
}

HalfedgeMesh::FaceAroundVertexCirculator::FaceAroundVertexCirculator(const HalfedgeMesh* mesh, VertexHandle v)
    : mesh_(mesh)
{
    if (mesh_)
    {
        halfedge_ = mesh_->halfedge(v);
        if (halfedge_.is_valid() && mesh_->is_boundary(halfedge_))
        {
            operator++();
        }
    }
}

bool HalfedgeMesh::FaceAroundVertexCirculator::operator==(const FaceAroundVertexCirculator& rhs) const
{
    assert(mesh_ != nullptr);
    assert(mesh_ == rhs.mesh_);
    return is_active_ && (halfedge_ == rhs.halfedge_);
}

HalfedgeMesh::FaceAroundVertexCirculator& HalfedgeMesh::FaceAroundVertexCirculator::operator++()
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    do
    {
        halfedge_ = mesh_->ccw_rotated_halfedge(halfedge_);
    } while (mesh_->is_boundary(halfedge_));
    is_active_ = true;
    return *this;
}

HalfedgeMesh::FaceAroundVertexCirculator HalfedgeMesh::FaceAroundVertexCirculator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::FaceAroundVertexCirculator& HalfedgeMesh::FaceAroundVertexCirculator::operator--()
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    do
    {
        halfedge_ = mesh_->cw_rotated_halfedge(halfedge_);
    } while (mesh_->is_boundary(halfedge_));
    return *this;
}

HalfedgeMesh::FaceAroundVertexCirculator HalfedgeMesh::FaceAroundVertexCirculator::operator--(int)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

FaceHandle HalfedgeMesh::FaceAroundVertexCirculator::operator*() const
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    return mesh_->face(halfedge_);
}

HalfedgeMesh::FaceAroundVertexCirculator& HalfedgeMesh::FaceAroundVertexCirculator::begin()
{
    is_active_ = !halfedge_.is_valid();
    return *this;
}

HalfedgeMesh::FaceAroundVertexCirculator& HalfedgeMesh::FaceAroundVertexCirculator::end()
{
    is_active_ = true;
    return *this;
}

HalfedgeMesh::VertexAroundFaceCirculator::VertexAroundFaceCirculator(const HalfedgeMesh* mesh, FaceHandle f)
    : mesh_(mesh)
{
    if (mesh_)
    {
        halfedge_ = mesh_->halfedge(f);
    }
}

bool HalfedgeMesh::VertexAroundFaceCirculator::operator==(const VertexAroundFaceCirculator& rhs) const
{
    assert(mesh_ != nullptr);
    assert(mesh_ == rhs.mesh_);
    return is_active_ && (halfedge_ == rhs.halfedge_);
}

HalfedgeMesh::VertexAroundFaceCirculator& HalfedgeMesh::VertexAroundFaceCirculator::operator++()
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    halfedge_ = mesh_->next_halfedge(halfedge_);
    is_active_ = true;
    return *this;
}

HalfedgeMesh::VertexAroundFaceCirculator HalfedgeMesh::VertexAroundFaceCirculator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::VertexAroundFaceCirculator& HalfedgeMesh::VertexAroundFaceCirculator::operator--()
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    halfedge_ = mesh_->prev_halfedge(halfedge_);
    return *this;
}

HalfedgeMesh::VertexAroundFaceCirculator HalfedgeMesh::VertexAroundFaceCirculator::operator--(int)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

VertexHandle HalfedgeMesh::VertexAroundFaceCirculator::operator*() const
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    return mesh_->to_vertex(halfedge_);
}

HalfedgeMesh::VertexAroundFaceCirculator& HalfedgeMesh::VertexAroundFaceCirculator::begin()
{
    is_active_ = false;
    return *this;
}

HalfedgeMesh::VertexAroundFaceCirculator& HalfedgeMesh::VertexAroundFaceCirculator::end()
{
    is_active_ = true;
    return *this;
}

HalfedgeMesh::HalfedgeAroundFaceCirculator::HalfedgeAroundFaceCirculator(const HalfedgeMesh* mesh, FaceHandle f)
    : mesh_(mesh)
{
    if (mesh_)
    {
        halfedge_ = mesh_->halfedge(f);
    }
}

bool HalfedgeMesh::HalfedgeAroundFaceCirculator::operator==(const HalfedgeAroundFaceCirculator& rhs) const
{
    assert(mesh_ != nullptr);
    assert(mesh_ == rhs.mesh_);
    return is_active_ && (halfedge_ == rhs.halfedge_);
}

HalfedgeMesh::HalfedgeAroundFaceCirculator& HalfedgeMesh::HalfedgeAroundFaceCirculator::operator++()
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    halfedge_ = mesh_->next_halfedge(halfedge_);
    is_active_ = true;
    return *this;
}

HalfedgeMesh::HalfedgeAroundFaceCirculator HalfedgeMesh::HalfedgeAroundFaceCirculator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HalfedgeMesh::HalfedgeAroundFaceCirculator& HalfedgeMesh::HalfedgeAroundFaceCirculator::operator--()
{
    assert(mesh_ != nullptr && halfedge_.is_valid());
    halfedge_ = mesh_->prev_halfedge(halfedge_);
    return *this;
}

HalfedgeMesh::HalfedgeAroundFaceCirculator HalfedgeMesh::HalfedgeAroundFaceCirculator::operator--(int)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

HalfedgeMesh::HalfedgeAroundFaceCirculator& HalfedgeMesh::HalfedgeAroundFaceCirculator::begin()
{
    is_active_ = false;
    return *this;
}

HalfedgeMesh::HalfedgeAroundFaceCirculator& HalfedgeMesh::HalfedgeAroundFaceCirculator::end()
{
    is_active_ = true;
    return *this;
}

HalfedgeMesh::HalfedgeMesh()
{
    vertex_points_ = add_vertex_property<Point3>("v:point");
    vertex_connectivity_ = add_vertex_property<VertexConnectivity>("v:connectivity");
    halfedge_connectivity_ = add_halfedge_property<HalfedgeConnectivity>("h:connectivity");
    face_connectivity_ = add_face_property<FaceConnectivity>("f:connectivity");

    vertex_deleted_ = add_vertex_property<bool>("v:deleted", false);
    edge_deleted_ = add_edge_property<bool>("e:deleted", false);
    face_deleted_ = add_face_property<bool>("f:deleted", false);
}

HalfedgeMesh::HalfedgeMesh(const HalfedgeMesh& rhs)
{
    operator=(rhs);
}

HalfedgeMesh::~HalfedgeMesh() = default;

HalfedgeMesh& HalfedgeMesh::operator=(const HalfedgeMesh& rhs)
{
    if (this == &rhs)
    {
        return *this;
    }

    vertex_props_ = rhs.vertex_props_;
    halfedge_props_ = rhs.halfedge_props_;
    edge_props_ = rhs.edge_props_;
    face_props_ = rhs.face_props_;

    vertex_points_ = vertex_property<Point3>("v:point");
    vertex_connectivity_ = vertex_property<VertexConnectivity>("v:connectivity");
    halfedge_connectivity_ = halfedge_property<HalfedgeConnectivity>("h:connectivity");
    face_connectivity_ = face_property<FaceConnectivity>("f:connectivity");

    vertex_deleted_ = vertex_property<bool>("v:deleted");
    edge_deleted_ = edge_property<bool>("e:deleted");
    face_deleted_ = face_property<bool>("f:deleted");

    deleted_vertices_ = rhs.deleted_vertices_;
    deleted_edges_ = rhs.deleted_edges_;
    deleted_faces_ = rhs.deleted_faces_;
    has_garbage_ = rhs.has_garbage_;

    return *this;
}

HalfedgeMesh& HalfedgeMesh::assign(const HalfedgeMesh& rhs)
{
    if (this == &rhs)
    {
        return *this;
    }

    vertex_props_.clear();
    halfedge_props_.clear();
    edge_props_.clear();
    face_props_.clear();

    vertex_points_ = add_vertex_property<Point3>("v:point");
    vertex_connectivity_ = add_vertex_property<VertexConnectivity>("v:connectivity");
    halfedge_connectivity_ = add_halfedge_property<HalfedgeConnectivity>("h:connectivity");
    face_connectivity_ = add_face_property<FaceConnectivity>("f:connectivity");

    vertex_deleted_ = add_vertex_property<bool>("v:deleted", false);
    edge_deleted_ = add_edge_property<bool>("e:deleted", false);
    face_deleted_ = add_face_property<bool>("f:deleted", false);

    vertex_points_.array() = rhs.vertex_points_.array();
    vertex_connectivity_.array() = rhs.vertex_connectivity_.array();
    halfedge_connectivity_.array() = rhs.halfedge_connectivity_.array();
    face_connectivity_.array() = rhs.face_connectivity_.array();

    vertex_deleted_.array() = rhs.vertex_deleted_.array();
    edge_deleted_.array() = rhs.edge_deleted_.array();
    face_deleted_.array() = rhs.face_deleted_.array();

    vertex_props_.resize(rhs.vertices_size());
    halfedge_props_.resize(rhs.halfedges_size());
    edge_props_.resize(rhs.edges_size());
    face_props_.resize(rhs.faces_size());

    deleted_vertices_ = rhs.deleted_vertices_;
    deleted_edges_ = rhs.deleted_edges_;
    deleted_faces_ = rhs.deleted_faces_;
    has_garbage_ = rhs.has_garbage_;

    return *this;
}

void HalfedgeMesh::clear()
{
    vertex_props_.clear();
    halfedge_props_.clear();
    edge_props_.clear();
    face_props_.clear();

    free_memory();

    vertex_points_ = add_vertex_property<Point3>("v:point");
    vertex_connectivity_ = add_vertex_property<VertexConnectivity>("v:connectivity");
    halfedge_connectivity_ = add_halfedge_property<HalfedgeConnectivity>("h:connectivity");
    face_connectivity_ = add_face_property<FaceConnectivity>("f:connectivity");

    vertex_deleted_ = add_vertex_property<bool>("v:deleted", false);
    edge_deleted_ = add_edge_property<bool>("e:deleted", false);
    face_deleted_ = add_face_property<bool>("f:deleted", false);

    deleted_vertices_ = 0;
    deleted_edges_ = 0;
    deleted_faces_ = 0;
    has_garbage_ = false;
}

void HalfedgeMesh::free_memory()
{
    vertex_props_.shrink_to_fit();
    halfedge_props_.shrink_to_fit();
    edge_props_.shrink_to_fit();
    face_props_.shrink_to_fit();
}

void HalfedgeMesh::reserve(std::size_t nvertices, std::size_t nedges, std::size_t nfaces)
{
    vertex_props_.reserve(nvertices);
    halfedge_props_.reserve(2 * nedges);
    edge_props_.reserve(nedges);
    face_props_.reserve(nfaces);
}

HalfedgeMesh::VertexIterator HalfedgeMesh::vertices_begin() const
{
    return VertexIterator(VertexHandle(0), this);
}

HalfedgeMesh::VertexIterator HalfedgeMesh::vertices_end() const
{
    return VertexIterator(VertexHandle(static_cast<MeshIndex>(vertices_size())), this);
}

HalfedgeMesh::HalfedgeIterator HalfedgeMesh::halfedges_begin() const
{
    return HalfedgeIterator(HalfedgeHandle(0), this);
}

HalfedgeMesh::HalfedgeIterator HalfedgeMesh::halfedges_end() const
{
    return HalfedgeIterator(HalfedgeHandle(static_cast<MeshIndex>(halfedges_size())), this);
}

HalfedgeMesh::EdgeIterator HalfedgeMesh::edges_begin() const
{
    return EdgeIterator(EdgeHandle(0), this);
}

HalfedgeMesh::EdgeIterator HalfedgeMesh::edges_end() const
{
    return EdgeIterator(EdgeHandle(static_cast<MeshIndex>(edges_size())), this);
}

HalfedgeMesh::FaceIterator HalfedgeMesh::faces_begin() const
{
    return FaceIterator(FaceHandle(0), this);
}

HalfedgeMesh::FaceIterator HalfedgeMesh::faces_end() const
{
    return FaceIterator(FaceHandle(static_cast<MeshIndex>(faces_size())), this);
}

bool HalfedgeMesh::is_boundary(VertexHandle v) const
{
    const HalfedgeHandle h = halfedge(v);
    return !(h.is_valid() && face(h).is_valid());
}

bool HalfedgeMesh::is_manifold(VertexHandle v) const
{
    int gaps = 0;
    auto hit = halfedges(v);
    auto hend = hit;
    if (hit)
    {
        do
        {
            if (is_boundary(*hit))
            {
                ++gaps;
            }
        } while (++hit != hend);
    }
    return gaps < 2;
}

void HalfedgeMesh::set_next_halfedge(HalfedgeHandle h, HalfedgeHandle next)
{
    halfedge_connectivity_[h].next = next;
    halfedge_connectivity_[next].prev = h;
}

void HalfedgeMesh::set_prev_halfedge(HalfedgeHandle h, HalfedgeHandle prev)
{
    halfedge_connectivity_[h].prev = prev;
    halfedge_connectivity_[prev].next = h;
}

HalfedgeHandle HalfedgeMesh::halfedge(EdgeHandle e, unsigned int i) const
{
    assert(i <= 1);
    return HalfedgeHandle((e.index() << 1U) + i);
}

bool HalfedgeMesh::is_boundary(EdgeHandle e) const
{
    return is_boundary(halfedge(e, 0)) || is_boundary(halfedge(e, 1));
}

bool HalfedgeMesh::is_boundary(FaceHandle f) const
{
    HalfedgeHandle h = halfedge(f);
    const HalfedgeHandle start = h;
    do
    {
        if (is_boundary(opposite_halfedge(h)))
        {
            return true;
        }
        h = next_halfedge(h);
    } while (h != start);
    return false;
}

HalfedgeHandle HalfedgeMesh::insert_vertex(EdgeHandle e, const Point3& p)
{
    return insert_vertex(halfedge(e, 0), add_vertex(p));
}

HalfedgeHandle HalfedgeMesh::insert_vertex(EdgeHandle e, VertexHandle v)
{
    return insert_vertex(halfedge(e, 0), v);
}

HalfedgeHandle HalfedgeMesh::find_halfedge(VertexHandle start, VertexHandle end) const
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
        } while (h != hh);
    }

    return HalfedgeHandle();
}

EdgeHandle HalfedgeMesh::find_edge(VertexHandle a, VertexHandle b) const
{
    const HalfedgeHandle h = find_halfedge(a, b);
    return h.is_valid() ? edge(h) : EdgeHandle();
}

bool HalfedgeMesh::is_triangle_mesh() const
{
    for (auto f : faces())
    {
        if (valence(f) != 3)
        {
            return false;
        }
    }
    return true;
}

bool HalfedgeMesh::is_quad_mesh() const
{
    for (auto f : faces())
    {
        if (valence(f) != 4)
        {
            return false;
        }
    }
    return true;
}

void HalfedgeMesh::adjust_outgoing_halfedge(VertexHandle v)
{
    HalfedgeHandle h = halfedge(v);
    const HalfedgeHandle start = h;
    if (h.is_valid())
    {
        do
        {
            if (is_boundary(h))
            {
                set_halfedge(v, h);
                return;
            }
            h = cw_rotated_halfedge(h);
        } while (h != start);
    }
}

VertexHandle HalfedgeMesh::add_vertex(const Point3& p)
{
    VertexHandle v = new_vertex();
    if (v.is_valid())
    {
        vertex_points_[v] = p;
    }
    return v;
}

std::optional<FaceHandle> HalfedgeMesh::add_triangle(VertexHandle v0, VertexHandle v1, VertexHandle v2)
{
    add_face_vertices_.assign({v0, v1, v2});
    return add_face(add_face_vertices_);
}

std::optional<FaceHandle> HalfedgeMesh::add_quad(VertexHandle v0, VertexHandle v1, VertexHandle v2, VertexHandle v3)
{
    add_face_vertices_.assign({v0, v1, v2, v3});
    return add_face(add_face_vertices_);
}

std::optional<FaceHandle> HalfedgeMesh::add_face(std::span<const VertexHandle> vertices)
{
    const std::size_t n = vertices.size();
    assert(n > 2);

    VertexHandle v;
    std::size_t i, ii, id;
    HalfedgeHandle inner_next, inner_prev, outer_next, outer_prev, boundary_next, boundary_prev, patch_start, patch_end;

    std::vector<HalfedgeHandle>& halfedges = add_face_halfedges_;
    std::vector<bool>& is_new = add_face_is_new_;
    std::vector<bool>& needs_adjust = add_face_needs_adjust_;
    NextCache& next_cache = add_face_next_cache_;
    halfedges.clear();
    halfedges.resize(n);
    is_new.clear();
    is_new.resize(n);
    needs_adjust.clear();
    needs_adjust.resize(n, false);
    next_cache.clear();
    next_cache.reserve(3 * n);

    for (i = 0, ii = 1; i < n; ++i, ++ii, ii %= n)
    {
        if (!is_boundary(vertices[i]))
        {
            return std::nullopt;
        }

        halfedges[i] = find_halfedge(vertices[i], vertices[ii]);
        is_new[i] = !halfedges[i].is_valid();

        if (!is_new[i] && !is_boundary(halfedges[i]))
        {
            return std::nullopt;
        }
    }

    for (i = 0, ii = 1; i < n; ++i, ++ii, ii %= n)
    {
        if (!is_new[i] && !is_new[ii])
        {
            inner_prev = halfedges[i];
            inner_next = halfedges[ii];

            if (next_halfedge(inner_prev) != inner_next)
            {
                outer_prev = opposite_halfedge(inner_next);
                outer_next = opposite_halfedge(inner_prev);
                boundary_prev = outer_prev;
                do
                {
                    boundary_prev = opposite_halfedge(next_halfedge(boundary_prev));
                } while (!is_boundary(boundary_prev) || boundary_prev == inner_prev);
                boundary_next = next_halfedge(boundary_prev);
                assert(is_boundary(boundary_prev));
                assert(is_boundary(boundary_next));

                if (boundary_next == inner_next)
                {
                    return std::nullopt;
                }

                patch_start = next_halfedge(inner_prev);
                patch_end = prev_halfedge(inner_next);

                next_cache.emplace_back(boundary_prev, patch_start);
                next_cache.emplace_back(patch_end, boundary_next);
                next_cache.emplace_back(inner_prev, inner_next);
            }
        }
    }

    for (i = 0, ii = 1; i < n; ++i, ++ii, ii %= n)
    {
        if (is_new[i])
        {
            halfedges[i] = new_edge(vertices[i], vertices[ii]);
        }
    }

    FaceHandle f = new_face();
    set_halfedge(f, halfedges[n - 1]);

    for (i = 0, ii = 1; i < n; ++i, ++ii, ii %= n)
    {
        v = vertices[ii];
        inner_prev = halfedges[i];
        inner_next = halfedges[ii];

        id = 0;
        if (is_new[i])
        {
            id |= 1;
        }
        if (is_new[ii])
        {
            id |= 2;
        }

        if (id)
        {
            outer_prev = opposite_halfedge(inner_next);
            outer_next = opposite_halfedge(inner_prev);

            switch (id)
            {
            case 1:
                boundary_prev = prev_halfedge(inner_next);
                next_cache.emplace_back(boundary_prev, outer_next);
                set_halfedge(v, outer_next);
                break;

            case 2:
                boundary_next = next_halfedge(inner_prev);
                next_cache.emplace_back(outer_prev, boundary_next);
                set_halfedge(v, boundary_next);
                break;

            case 3:
                if (!halfedge(v).is_valid())
                {
                    set_halfedge(v, outer_next);
                    next_cache.emplace_back(outer_prev, outer_next);
                }
                else
                {
                    boundary_next = halfedge(v);
                    boundary_prev = prev_halfedge(boundary_next);
                    next_cache.emplace_back(boundary_prev, outer_next);
                    next_cache.emplace_back(outer_prev, boundary_next);
                }
                break;
            }

            next_cache.emplace_back(inner_prev, inner_next);
        }
        else
        {
            needs_adjust[ii] = (halfedge(v) == inner_next);
        }

        set_face(halfedges[i], f);
    }

    for (const auto& [first, second] : next_cache)
    {
        set_next_halfedge(first, second);
    }

    for (i = 0; i < n; ++i)
    {
        if (needs_adjust[i])
        {
            adjust_outgoing_halfedge(vertices[i]);
        }
    }

    return f;
}

std::size_t HalfedgeMesh::valence(VertexHandle v) const
{
    auto vv = vertices(v);
    return static_cast<std::size_t>(std::distance(vv.begin(), vv.end()));
}

std::size_t HalfedgeMesh::valence(FaceHandle f) const
{
    auto vv = vertices(f);
    return static_cast<std::size_t>(std::distance(vv.begin(), vv.end()));
}

HalfedgeHandle HalfedgeMesh::insert_vertex(HalfedgeHandle h0, VertexHandle v)
{
    const HalfedgeHandle h2 = next_halfedge(h0);
    const HalfedgeHandle o0 = opposite_halfedge(h0);
    const HalfedgeHandle o2 = prev_halfedge(o0);
    const VertexHandle v2 = to_vertex(h0);
    const FaceHandle fh = face(h0);
    const FaceHandle fo = face(o0);

    const HalfedgeHandle h1 = new_edge(v, v2);
    HalfedgeHandle o1 = opposite_halfedge(h1);

    set_next_halfedge(h1, h2);
    set_next_halfedge(h0, h1);
    set_vertex(h0, v);
    set_vertex(h1, v2);
    set_face(h1, fh);

    set_next_halfedge(o1, o0);
    set_next_halfedge(o2, o1);
    set_vertex(o1, v);
    set_face(o1, fo);

    set_halfedge(v2, o1);
    adjust_outgoing_halfedge(v2);
    set_halfedge(v, h1);
    adjust_outgoing_halfedge(v);

    if (fh.is_valid())
    {
        set_halfedge(fh, h0);
    }
    if (fo.is_valid())
    {
        set_halfedge(fo, o1);
    }

    return o1;
}

VertexHandle HalfedgeMesh::split(FaceHandle f, const Point3& p)
{
    VertexHandle v = add_vertex(p);
    split(f, v);
    return v;
}

void HalfedgeMesh::split(FaceHandle f, VertexHandle v)
{
    const HalfedgeHandle hend = halfedge(f);
    HalfedgeHandle h = next_halfedge(hend);

    HalfedgeHandle hold = new_edge(to_vertex(hend), v);

    set_next_halfedge(hend, hold);
    set_face(hold, f);

    hold = opposite_halfedge(hold);

    while (h != hend)
    {
        const HalfedgeHandle hnext = next_halfedge(h);

        const FaceHandle fnew = new_face();
        set_halfedge(fnew, h);

        const HalfedgeHandle hnew = new_edge(to_vertex(h), v);

        set_next_halfedge(hnew, hold);
        set_next_halfedge(hold, h);
        set_next_halfedge(h, hnew);

        set_face(hnew, fnew);
        set_face(hold, fnew);
        set_face(h, fnew);

        hold = opposite_halfedge(hnew);

        h = hnext;
    }

    set_next_halfedge(hold, hend);
    set_next_halfedge(next_halfedge(hend), hold);

    set_face(hold, f);

    set_halfedge(v, hold);
}

HalfedgeHandle HalfedgeMesh::split(EdgeHandle e, const Point3& p)
{
    return split(e, add_vertex(p));
}

HalfedgeHandle HalfedgeMesh::split(EdgeHandle e, VertexHandle v)
{
    const HalfedgeHandle h0 = halfedge(e, 0);
    const HalfedgeHandle o0 = halfedge(e, 1);

    const VertexHandle v2 = to_vertex(o0);

    const HalfedgeHandle e1 = new_edge(v, v2);
    HalfedgeHandle t1 = opposite_halfedge(e1);

    const FaceHandle f0 = face(h0);
    const FaceHandle f3 = face(o0);

    set_halfedge(v, h0);
    set_vertex(o0, v);

    if (!is_boundary(h0))
    {
        const HalfedgeHandle h1 = next_halfedge(h0);
        const HalfedgeHandle h2 = next_halfedge(h1);

        const VertexHandle v1 = to_vertex(h1);

        const HalfedgeHandle e0 = new_edge(v, v1);
        const HalfedgeHandle t0 = opposite_halfedge(e0);

        const FaceHandle f1 = new_face();
        set_halfedge(f0, h0);
        set_halfedge(f1, h2);

        set_face(h1, f0);
        set_face(t0, f0);
        set_face(h0, f0);

        set_face(h2, f1);
        set_face(t1, f1);
        set_face(e0, f1);

        set_next_halfedge(h0, h1);
        set_next_halfedge(h1, t0);
        set_next_halfedge(t0, h0);

        set_next_halfedge(e0, h2);
        set_next_halfedge(h2, t1);
        set_next_halfedge(t1, e0);
    }
    else
    {
        set_next_halfedge(prev_halfedge(h0), t1);
        set_next_halfedge(t1, h0);
    }

    if (!is_boundary(o0))
    {
        const HalfedgeHandle o1 = next_halfedge(o0);
        const HalfedgeHandle o2 = next_halfedge(o1);

        const VertexHandle v3 = to_vertex(o1);

        const HalfedgeHandle e2 = new_edge(v, v3);
        const HalfedgeHandle t2 = opposite_halfedge(e2);

        const FaceHandle f2 = new_face();
        set_halfedge(f2, o1);
        set_halfedge(f3, o0);

        set_face(o1, f2);
        set_face(t2, f2);
        set_face(e1, f2);

        set_face(o2, f3);
        set_face(o0, f3);
        set_face(e2, f3);

        set_next_halfedge(e1, o1);
        set_next_halfedge(o1, t2);
        set_next_halfedge(t2, e1);

        set_next_halfedge(o0, e2);
        set_next_halfedge(e2, o2);
        set_next_halfedge(o2, o0);
    }
    else
    {
        set_next_halfedge(e1, next_halfedge(o0));
        set_next_halfedge(o0, e1);
        set_halfedge(v, e1);
    }

    if (halfedge(v2) == h0)
    {
        set_halfedge(v2, t1);
    }

    return t1;
}

HalfedgeHandle HalfedgeMesh::insert_edge(HalfedgeHandle h0, HalfedgeHandle h1)
{
    assert(face(h0) == face(h1));
    assert(face(h0).is_valid());

    const VertexHandle v0 = to_vertex(h0);
    const VertexHandle v1 = to_vertex(h1);

    const HalfedgeHandle h2 = next_halfedge(h0);
    const HalfedgeHandle h3 = next_halfedge(h1);

    HalfedgeHandle h4 = new_edge(v0, v1);
    const HalfedgeHandle h5 = opposite_halfedge(h4);

    const FaceHandle f0 = face(h0);
    const FaceHandle f1 = new_face();

    set_halfedge(f0, h0);
    set_halfedge(f1, h1);

    set_next_halfedge(h0, h4);
    set_next_halfedge(h4, h3);
    set_face(h4, f0);

    set_next_halfedge(h1, h5);
    set_next_halfedge(h5, h2);
    HalfedgeHandle h = h2;
    do
    {
        set_face(h, f1);
        h = next_halfedge(h);
    } while (h != h2);

    return h4;
}

bool HalfedgeMesh::is_flip_ok(EdgeHandle e) const
{
    if (is_boundary(e))
    {
        return false;
    }

    const HalfedgeHandle h0 = halfedge(e, 0);
    const HalfedgeHandle h1 = halfedge(e, 1);

    const VertexHandle v0 = to_vertex(next_halfedge(h0));
    const VertexHandle v1 = to_vertex(next_halfedge(h1));

    if (v0 == v1)
    {
        return false;
    }

    if (find_halfedge(v0, v1).is_valid())
    {
        return false;
    }

    return true;
}

void HalfedgeMesh::flip(EdgeHandle e)
{
    assert(is_flip_ok(e));

    const HalfedgeHandle a0 = halfedge(e, 0);
    const HalfedgeHandle b0 = halfedge(e, 1);

    const HalfedgeHandle a1 = next_halfedge(a0);
    const HalfedgeHandle a2 = next_halfedge(a1);

    const HalfedgeHandle b1 = next_halfedge(b0);
    const HalfedgeHandle b2 = next_halfedge(b1);

    const VertexHandle va0 = to_vertex(a0);
    const VertexHandle va1 = to_vertex(a1);

    const VertexHandle vb0 = to_vertex(b0);
    const VertexHandle vb1 = to_vertex(b1);

    const FaceHandle fa = face(a0);
    const FaceHandle fb = face(b0);

    set_vertex(a0, va1);
    set_vertex(b0, vb1);

    set_next_halfedge(a0, a2);
    set_next_halfedge(a2, b1);
    set_next_halfedge(b1, a0);

    set_next_halfedge(b0, b2);
    set_next_halfedge(b2, a1);
    set_next_halfedge(a1, b0);

    set_face(a1, fb);
    set_face(b1, fa);

    set_halfedge(fa, a0);
    set_halfedge(fb, b0);

    if (halfedge(va0) == b0)
    {
        set_halfedge(va0, a1);
    }
    if (halfedge(vb0) == a0)
    {
        set_halfedge(vb0, b1);
    }
}

bool HalfedgeMesh::is_collapse_ok(HalfedgeHandle v0v1) const
{
    const HalfedgeHandle v1v0 = opposite_halfedge(v0v1);
    const VertexHandle v0 = to_vertex(v1v0);
    const VertexHandle v1 = to_vertex(v0v1);
    VertexHandle vl, vr;
    HalfedgeHandle h1, h2;

    if (!is_boundary(v0v1))
    {
        vl = to_vertex(next_halfedge(v0v1));
        h1 = next_halfedge(v0v1);
        h2 = next_halfedge(h1);
        if (is_boundary(opposite_halfedge(h1)) && is_boundary(opposite_halfedge(h2)))
        {
            return false;
        }
    }

    if (!is_boundary(v1v0))
    {
        vr = to_vertex(next_halfedge(v1v0));
        h1 = next_halfedge(v1v0);
        h2 = next_halfedge(h1);
        if (is_boundary(opposite_halfedge(h1)) && is_boundary(opposite_halfedge(h2)))
        {
            return false;
        }
    }

    if (vl == vr)
    {
        return false;
    }

    if (is_boundary(v0) && is_boundary(v1) && !is_boundary(v0v1) && !is_boundary(v1v0))
    {
        return false;
    }

    for (auto vv : vertices(v0))
    {
        if (vv != v1 && vv != vl && vv != vr)
        {
            if (find_halfedge(vv, v1).is_valid())
            {
                return false;
            }
        }
    }

    return true;
}

bool HalfedgeMesh::is_removal_ok(EdgeHandle e) const
{
    const HalfedgeHandle h0 = halfedge(e, 0);
    const HalfedgeHandle h1 = halfedge(e, 1);
    const VertexHandle v0 = to_vertex(h0);
    const VertexHandle v1 = to_vertex(h1);
    const FaceHandle f0 = face(h0);
    const FaceHandle f1 = face(h1);

    if (!f0.is_valid() || !f1.is_valid())
    {
        return false;
    }

    if (f0 == f1)
    {
        return false;
    }

    for (auto v : vertices(f0))
    {
        if (v != v0 && v != v1)
        {
            for (auto f : faces(v))
            {
                if (f == f1)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool HalfedgeMesh::remove_edge(EdgeHandle e)
{
    if (!is_removal_ok(e))
    {
        return false;
    }

    const HalfedgeHandle h0 = halfedge(e, 0);
    const HalfedgeHandle h1 = halfedge(e, 1);

    const VertexHandle v0 = to_vertex(h0);
    const VertexHandle v1 = to_vertex(h1);

    const FaceHandle f0 = face(h0);
    const FaceHandle f1 = face(h1);

    const HalfedgeHandle h0_prev = prev_halfedge(h0);
    const HalfedgeHandle h0_next = next_halfedge(h0);
    const HalfedgeHandle h1_prev = prev_halfedge(h1);
    const HalfedgeHandle h1_next = next_halfedge(h1);

    if (halfedge(v0) == h1)
    {
        set_halfedge(v0, h0_next);
    }
    if (halfedge(v1) == h0)
    {
        set_halfedge(v1, h1_next);
    }

    for (auto h : halfedges(f0))
    {
        set_face(h, f1);
    }

    set_next_halfedge(h1_prev, h0_next);
    set_next_halfedge(h0_prev, h1_next);

    if (halfedge(f1) == h1)
    {
        set_halfedge(f1, h1_next);
    }

    face_deleted_[f0] = true;
    ++deleted_faces_;
    edge_deleted_[e] = true;
    ++deleted_edges_;
    has_garbage_ = true;

    return true;
}

void HalfedgeMesh::collapse(HalfedgeHandle h)
{
    const HalfedgeHandle h0 = h;
    const HalfedgeHandle h1 = prev_halfedge(h0);
    const HalfedgeHandle o0 = opposite_halfedge(h0);
    const HalfedgeHandle o1 = next_halfedge(o0);

    remove_edge_helper(h0);

    if (next_halfedge(next_halfedge(h1)) == h1)
    {
        remove_loop_helper(h1);
    }

    if (next_halfedge(next_halfedge(o1)) == o1)
    {
        remove_loop_helper(o1);
    }
}

void HalfedgeMesh::remove_edge_helper(HalfedgeHandle h)
{
    const HalfedgeHandle hn = next_halfedge(h);
    const HalfedgeHandle hp = prev_halfedge(h);

    const HalfedgeHandle o = opposite_halfedge(h);
    const HalfedgeHandle on = next_halfedge(o);
    const HalfedgeHandle op = prev_halfedge(o);

    const FaceHandle fh = face(h);
    const FaceHandle fo = face(o);

    const VertexHandle vh = to_vertex(h);
    const VertexHandle vo = to_vertex(o);

    for (const auto hc : halfedges(vo))
    {
        set_vertex(opposite_halfedge(hc), vh);
    }

    set_next_halfedge(hp, hn);
    set_next_halfedge(op, on);

    if (fh.is_valid())
    {
        set_halfedge(fh, hn);
    }
    if (fo.is_valid())
    {
        set_halfedge(fo, on);
    }

    if (halfedge(vh) == o)
    {
        set_halfedge(vh, hn);
    }
    adjust_outgoing_halfedge(vh);
    set_halfedge(vo, HalfedgeHandle());

    vertex_deleted_[vo] = true;
    ++deleted_vertices_;
    edge_deleted_[edge(h)] = true;
    ++deleted_edges_;
    has_garbage_ = true;
}

void HalfedgeMesh::remove_loop_helper(HalfedgeHandle h)
{
    const HalfedgeHandle h0 = h;
    const HalfedgeHandle h1 = next_halfedge(h0);

    const HalfedgeHandle o0 = opposite_halfedge(h0);
    const HalfedgeHandle o1 = opposite_halfedge(h1);

    const VertexHandle v0 = to_vertex(h0);
    const VertexHandle v1 = to_vertex(h1);

    const FaceHandle fh = face(h0);
    const FaceHandle fo = face(o0);

    assert((next_halfedge(h1) == h0) && (h1 != o0));

    set_next_halfedge(h1, next_halfedge(o0));
    set_next_halfedge(prev_halfedge(o0), h1);

    set_face(h1, fo);

    set_halfedge(v0, h1);
    adjust_outgoing_halfedge(v0);
    set_halfedge(v1, o1);
    adjust_outgoing_halfedge(v1);

    if (fo.is_valid() && halfedge(fo) == o0)
    {
        set_halfedge(fo, h1);
    }

    if (fh.is_valid())
    {
        face_deleted_[fh] = true;
        ++deleted_faces_;
    }
    edge_deleted_[edge(h)] = true;
    ++deleted_edges_;
    has_garbage_ = true;
}

void HalfedgeMesh::delete_vertex(VertexHandle v)
{
    if (is_deleted(v))
    {
        return;
    }

    std::vector<FaceHandle> incident_faces;
    incident_faces.reserve(6);

    for (auto f : faces(v))
    {
        incident_faces.push_back(f);
    }

    for (auto f : incident_faces)
    {
        delete_face(f);
    }

    if (!vertex_deleted_[v])
    {
        vertex_deleted_[v] = true;
        ++deleted_vertices_;
        has_garbage_ = true;
    }
}

void HalfedgeMesh::delete_edge(EdgeHandle e)
{
    if (is_deleted(e))
    {
        return;
    }

    const FaceHandle f0 = face(halfedge(e, 0));
    const FaceHandle f1 = face(halfedge(e, 1));

    if (f0.is_valid())
    {
        delete_face(f0);
    }
    if (f1.is_valid())
    {
        delete_face(f1);
    }
}

void HalfedgeMesh::delete_face(FaceHandle f)
{
    if (face_deleted_[f])
    {
        return;
    }

    if (!face_deleted_[f])
    {
        face_deleted_[f] = true;
        ++deleted_faces_;
    }

    std::vector<EdgeHandle> deleted_edges;
    deleted_edges.reserve(3);

    std::vector<VertexHandle> vertices;
    vertices.reserve(3);

    for (auto hc : halfedges(f))
    {
        set_face(hc, FaceHandle());

        if (is_boundary(opposite_halfedge(hc)))
        {
            deleted_edges.push_back(edge(hc));
        }

        vertices.push_back(to_vertex(hc));
    }

    if (!deleted_edges.empty())
    {
        for (const auto& edge_handle : deleted_edges)
        {
            const auto h0 = halfedge(edge_handle, 0);
            const auto v0 = to_vertex(h0);
            const auto next0 = next_halfedge(h0);
            const auto prev0 = prev_halfedge(h0);

            const auto h1 = halfedge(edge_handle, 1);
            const auto v1 = to_vertex(h1);
            const auto next1 = next_halfedge(h1);
            const auto prev1 = prev_halfedge(h1);

            set_next_halfedge(prev0, next1);
            set_next_halfedge(prev1, next0);

            if (!edge_deleted_[edge_handle])
            {
                edge_deleted_[edge_handle] = true;
                ++deleted_edges_;
            }

            if (halfedge(v0) == h1)
            {
                if (next0 == h1)
                {
                    if (!vertex_deleted_[v0])
                    {
                        vertex_deleted_[v0] = true;
                        ++deleted_vertices_;
                    }
                }
                else
                {
                    set_halfedge(v0, next0);
                }
            }

            if (halfedge(v1) == h0)
            {
                if (next1 == h0)
                {
                    if (!vertex_deleted_[v1])
                    {
                        vertex_deleted_[v1] = true;
                        ++deleted_vertices_;
                    }
                }
                else
                {
                    set_halfedge(v1, next1);
                }
            }
        }
    }

    for (auto vtx : vertices)
    {
        adjust_outgoing_halfedge(vtx);
    }

    has_garbage_ = true;
}

VertexHandle HalfedgeMesh::new_vertex()
{
    if (vertices_size() >= kInvalidMeshIndex)
    {
        return VertexHandle();
    }
    vertex_props_.push_back();
    return VertexHandle(static_cast<MeshIndex>(vertices_size() - 1));
}

HalfedgeHandle HalfedgeMesh::new_edge()
{
    if (halfedges_size() >= kInvalidMeshIndex)
    {
        return HalfedgeHandle();
    }

    edge_props_.push_back();
    halfedge_props_.push_back();
    halfedge_props_.push_back();

    return HalfedgeHandle(static_cast<MeshIndex>(halfedges_size() - 2));
}

HalfedgeHandle HalfedgeMesh::new_edge(VertexHandle start, VertexHandle end)
{
    assert(start != end);

    if (halfedges_size() >= kInvalidMeshIndex)
    {
        return HalfedgeHandle();
    }

    edge_props_.push_back();
    halfedge_props_.push_back();
    halfedge_props_.push_back();

    const HalfedgeHandle h0(static_cast<MeshIndex>(halfedges_size() - 2));
    const HalfedgeHandle h1(static_cast<MeshIndex>(halfedges_size() - 1));

    set_vertex(h0, end);
    set_vertex(h1, start);

    return h0;
}

FaceHandle HalfedgeMesh::new_face()
{
    if (faces_size() >= kInvalidMeshIndex)
    {
        return FaceHandle();
    }

    face_props_.push_back();
    return FaceHandle(static_cast<MeshIndex>(faces_size() - 1));
}

void HalfedgeMesh::garbage_collection()
{
    if (!has_garbage_)
    {
        return;
    }

    auto nv = vertices_size();
    auto ne = edges_size();
    auto nh = halfedges_size();
    auto nf = faces_size();

    VertexProperty<VertexHandle> vmap = add_vertex_property<VertexHandle>("v:garbage-collection");
    HalfedgeProperty<HalfedgeHandle> hmap = add_halfedge_property<HalfedgeHandle>("h:garbage-collection");
    FaceProperty<FaceHandle> fmap = add_face_property<FaceHandle>("f:garbage-collection");
    for (std::size_t i = 0; i < nv; ++i)
    {
        vmap[VertexHandle(static_cast<MeshIndex>(i))] = VertexHandle(static_cast<MeshIndex>(i));
    }
    for (std::size_t i = 0; i < nh; ++i)
    {
        hmap[HalfedgeHandle(static_cast<MeshIndex>(i))] = HalfedgeHandle(static_cast<MeshIndex>(i));
    }
    for (std::size_t i = 0; i < nf; ++i)
    {
        fmap[FaceHandle(static_cast<MeshIndex>(i))] = FaceHandle(static_cast<MeshIndex>(i));
    }

    if (nv > 0)
    {
        std::size_t i0 = 0;
        std::size_t i1 = nv - 1;

        while (true)
        {
            while (!vertex_deleted_[VertexHandle(static_cast<MeshIndex>(i0))] && i0 < i1)
            {
                ++i0;
            }
            while (vertex_deleted_[VertexHandle(static_cast<MeshIndex>(i1))] && i0 < i1)
            {
                --i1;
            }
            if (i0 >= i1)
            {
                break;
            }

            vertex_props_.swap(i0, i1);
        }

        nv = vertex_deleted_[VertexHandle(static_cast<MeshIndex>(i0))] ? i0 : i0 + 1;
    }

    if (ne > 0)
    {
        std::size_t i0 = 0;
        std::size_t i1 = ne - 1;

        while (true)
        {
            while (!edge_deleted_[EdgeHandle(static_cast<MeshIndex>(i0))] && i0 < i1)
            {
                ++i0;
            }
            while (edge_deleted_[EdgeHandle(static_cast<MeshIndex>(i1))] && i0 < i1)
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

        ne = edge_deleted_[EdgeHandle(static_cast<MeshIndex>(i0))] ? i0 : i0 + 1;
        nh = 2 * ne;
    }

    if (nf > 0)
    {
        std::size_t i0 = 0;
        std::size_t i1 = nf - 1;

        while (true)
        {
            while (!face_deleted_[FaceHandle(static_cast<MeshIndex>(i0))] && i0 < i1)
            {
                ++i0;
            }
            while (face_deleted_[FaceHandle(static_cast<MeshIndex>(i1))] && i0 < i1)
            {
                --i1;
            }
            if (i0 >= i1)
            {
                break;
            }

            face_props_.swap(i0, i1);
        }

        nf = face_deleted_[FaceHandle(static_cast<MeshIndex>(i0))] ? i0 : i0 + 1;
    }

    for (std::size_t i = 0; i < nv; ++i)
    {
        auto v = VertexHandle(static_cast<MeshIndex>(i));
        if (!is_isolated(v))
        {
            set_halfedge(v, hmap[halfedge(v)]);
        }
    }

    for (std::size_t i = 0; i < nh; ++i)
    {
        auto h = HalfedgeHandle(static_cast<MeshIndex>(i));
        set_vertex(h, vmap[to_vertex(h)]);
        set_next_halfedge(h, hmap[next_halfedge(h)]);
        if (!is_boundary(h))
        {
            set_face(h, fmap[face(h)]);
        }
    }

    for (std::size_t i = 0; i < nf; ++i)
    {
        auto f = FaceHandle(static_cast<MeshIndex>(i));
        set_halfedge(f, hmap[halfedge(f)]);
    }

    remove_vertex_property(vmap);
    remove_halfedge_property(hmap);
    remove_face_property(fmap);

    vertex_props_.resize(nv);
    vertex_props_.shrink_to_fit();
    halfedge_props_.resize(nh);
    halfedge_props_.shrink_to_fit();
    edge_props_.resize(ne);
    edge_props_.shrink_to_fit();
    face_props_.resize(nf);
    face_props_.shrink_to_fit();

    deleted_vertices_ = deleted_edges_ = deleted_faces_ = 0;
    has_garbage_ = false;
}

} // namespace engine::geometry

