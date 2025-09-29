#include "../../include/engine/geometry/point_cloud/point_cloud.hpp"

#include <string>
#include <string_view>

namespace engine::geometry::point_cloud {
    PointCloudInterface::PointCloudInterface(Vertices &vertex_props) : vertex_props_(vertex_props){
        ensure_properties();
    }

    void PointCloudInterface::ensure_properties() {
        vertex_points_ = vertex_property<math::vec3>("v:point");

        vertex_deleted_ = vertex_property<bool>("v:deleted", false);
    }

    PointCloudInterface::PointCloudInterface(const PointCloudInterface &rhs) : vertex_props_(rhs.vertex_props_) {
        operator=(rhs);
    }

    PointCloudInterface::~PointCloudInterface() = default;

    PointCloudInterface &PointCloudInterface::operator=(const PointCloudInterface &rhs) {
        if (this == &rhs) {
            return *this;
        }

        vertex_props_ = rhs.vertex_props_;

        ensure_properties();

        deleted_vertices_ = rhs.deleted_vertices_;
        has_garbage_ = rhs.has_garbage_;

        return *this;
    }

    PointCloudInterface &PointCloudInterface::assign(const PointCloudInterface &rhs) {
        if (this == &rhs) {
            return *this;
        }

        vertex_props_.clear();

        ensure_properties();

        vertex_points_.array() = rhs.vertex_points_.array();

        vertex_deleted_.array() = rhs.vertex_deleted_.array();

        vertex_props_.resize(rhs.vertices_size());

        deleted_vertices_ = rhs.deleted_vertices_;
        has_garbage_ = rhs.has_garbage_;

        return *this;
    }

    void PointCloudInterface::clear() {
        vertex_props_.clear();

        free_memory();
        ensure_properties();

        deleted_vertices_ = 0;
        has_garbage_ = false;
    }

    void PointCloudInterface::free_memory() {
        vertex_props_.shrink_to_fit();
    }

    void PointCloudInterface::reserve(std::size_t nvertices) {
        vertex_props_.reserve(nvertices);
    }

    PointCloudInterface::VertexIterator PointCloudInterface::vertices_begin() const {
        return VertexIterator(VertexHandle(0), this);
    }

    PointCloudInterface::VertexIterator PointCloudInterface::vertices_end() const {
        return VertexIterator(VertexHandle(static_cast<PropertyIndex>(vertices_size())), this);
    }

    VertexHandle PointCloudInterface::add_vertex(const math::vec3 &p) {
        VertexHandle v = new_vertex();
        if (v.is_valid()) {
            vertex_points_[v] = p;
        }
        return v;
    }

    void PointCloudInterface::delete_vertex(VertexHandle v) {
        if (is_deleted(v)) {
            return;
        }
        
        if (!vertex_deleted_[v]) {
            vertex_deleted_[v] = true;
            ++deleted_vertices_;
            has_garbage_ = true;
        }
    }

    VertexHandle PointCloudInterface::new_vertex() {
        if (vertices_size() >= kInvalidPropertyIndex) {
            return VertexHandle();
        }
        vertex_props_.push_back();
        return VertexHandle(static_cast<PropertyIndex>(vertices_size() - 1));
    }

    
    void PointCloudInterface::garbage_collection() {
        if (!has_garbage_) {
            return;
        }

        auto nv = vertices_size();

        VertexProperty<VertexHandle> vmap = add_vertex_property<VertexHandle>("v:garbage-collection");
        for (std::size_t i = 0; i < nv; ++i) {
            vmap[VertexHandle(static_cast<PropertyIndex>(i))] = VertexHandle(static_cast<PropertyIndex>(i));
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

        remove_vertex_property(vmap);

        vertex_props_.resize(nv);
        vertex_props_.shrink_to_fit();

        deleted_vertices_ = 0;
        has_garbage_ = false;
    }
} // namespace engine::geometry
