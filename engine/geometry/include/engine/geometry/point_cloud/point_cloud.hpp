#pragma once

#include "engine/geometry/properties/property_handle.hpp"
#include "engine/geometry/utils/iterators.hpp"
#include "engine/geometry/utils/ranges.hpp"
#include "engine/math/vector.hpp"

#include <limits>
#include <ostream>
#include <string>
#include <vector>

namespace engine::geometry::point_cloud {
    struct IOFlags;

    class PointCloudInterface {
    public:
        explicit PointCloudInterface(Vertices &vertex_props);

        PointCloudInterface(const PointCloudInterface &rhs);

        PointCloudInterface(PointCloudInterface &&) noexcept = default;

        ~PointCloudInterface();

        PointCloudInterface &operator=(const PointCloudInterface &rhs);

        PointCloudInterface &assign(const PointCloudInterface &rhs);

        using VertexIterator = Iterator<PointCloudInterface, VertexHandle>;

        using VertexRange = Range<VertexIterator>;

        [[nodiscard]] VertexHandle add_vertex(const math::vec3 &p);

        void clear();

        void free_memory();

        void reserve(std::size_t nvertices);

        void garbage_collection();

        [[nodiscard]] std::size_t vertices_size() const noexcept { return vertex_props_.size(); }

        [[nodiscard]] std::size_t vertex_count() const noexcept { return vertices_size() - deleted_vertices_; }

        [[nodiscard]] bool is_empty() const noexcept { return vertex_count() == 0; }

        [[nodiscard]] bool is_deleted(VertexHandle v) const { return vertex_deleted_[v]; }

        [[nodiscard]] bool is_valid(VertexHandle v) const { return v.is_valid() && v.index() < vertices_size(); }

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

        [[nodiscard]] std::vector<std::string> vertex_properties() const { return vertex_props_.properties(); }

        [[nodiscard]] VertexIterator vertices_begin() const;

        [[nodiscard]] VertexIterator vertices_end() const;

        [[nodiscard]] VertexRange vertices() const { return {vertices_begin(), vertices_end()}; }

        void delete_vertex(VertexHandle v);

        [[nodiscard]] const math::vec3 &position(VertexHandle v) const { return vertex_points_[v]; }
        [[nodiscard]] math::vec3 &position(VertexHandle v) { return vertex_points_[v]; }
        [[nodiscard]] std::vector<math::vec3> &positions() { return vertex_points_.vector(); }

        [[nodiscard]] VertexHandle new_vertex();

        [[nodiscard]] bool has_garbage() const noexcept { return has_garbage_; }
    private:
        void ensure_properties();

        Vertices vertex_props_;

        VertexProperty<math::vec3> vertex_points_;

        VertexProperty<bool> vertex_deleted_;

        PropertyIndex deleted_vertices_{0};

        bool has_garbage_{false};
    };
} // namespace engine::geometry

namespace engine::geometry {
    using PointCloudInterface = point_cloud::PointCloudInterface;

    struct PointCloudData {
        Vertices vertex_props;
    };

    struct PointCloud {
        PointCloud() : data(), interface(data.vertex_props) {
        }

        PointCloudData data;
        PointCloudInterface interface;
    };
}
