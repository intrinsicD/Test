#include "engine/io/geometry_io_registry.hpp"

#include <mutex>
#include <stdexcept>
#include <utility>

namespace engine::io
{
    namespace
    {
        template <typename Map, typename Key, typename Value>
        void insert_or_assign(Map& map, Key key, Value value)
        {
            map[std::move(key)] = std::move(value);
        }
    } // namespace

    GeometryIORegistry::GeometryIORegistry() = default;
    GeometryIORegistry::~GeometryIORegistry() = default;

    void GeometryIORegistry::register_mesh_importer(std::unique_ptr<MeshImporter> importer)
    {
        if (!importer)
        {
            throw std::invalid_argument("Mesh importer must not be null");
        }
        const auto format = importer->format();
        if (format == MeshFileFormat::unknown)
        {
            throw std::invalid_argument("Mesh importer cannot target unknown format");
        }
        insert_or_assign(mesh_importers_, format, std::move(importer));
    }

    void GeometryIORegistry::register_mesh_exporter(std::unique_ptr<MeshExporter> exporter)
    {
        if (!exporter)
        {
            throw std::invalid_argument("Mesh exporter must not be null");
        }
        const auto format = exporter->format();
        if (format == MeshFileFormat::unknown)
        {
            throw std::invalid_argument("Mesh exporter cannot target unknown format");
        }
        insert_or_assign(mesh_exporters_, format, std::move(exporter));
    }

    void GeometryIORegistry::register_point_cloud_importer(std::unique_ptr<PointCloudImporter> importer)
    {
        if (!importer)
        {
            throw std::invalid_argument("Point cloud importer must not be null");
        }
        const auto format = importer->format();
        if (format == PointCloudFileFormat::unknown)
        {
            throw std::invalid_argument("Point cloud importer cannot target unknown format");
        }
        insert_or_assign(point_cloud_importers_, format, std::move(importer));
    }

    void GeometryIORegistry::register_point_cloud_exporter(std::unique_ptr<PointCloudExporter> exporter)
    {
        if (!exporter)
        {
            throw std::invalid_argument("Point cloud exporter must not be null");
        }
        const auto format = exporter->format();
        if (format == PointCloudFileFormat::unknown)
        {
            throw std::invalid_argument("Point cloud exporter cannot target unknown format");
        }
        insert_or_assign(point_cloud_exporters_, format, std::move(exporter));
    }

    void GeometryIORegistry::register_graph_importer(std::unique_ptr<GraphImporter> importer)
    {
        if (!importer)
        {
            throw std::invalid_argument("Graph importer must not be null");
        }
        const auto format = importer->format();
        if (format == GraphFileFormat::unknown)
        {
            throw std::invalid_argument("Graph importer cannot target unknown format");
        }
        insert_or_assign(graph_importers_, format, std::move(importer));
    }

    void GeometryIORegistry::register_graph_exporter(std::unique_ptr<GraphExporter> exporter)
    {
        if (!exporter)
        {
            throw std::invalid_argument("Graph exporter must not be null");
        }
        const auto format = exporter->format();
        if (format == GraphFileFormat::unknown)
        {
            throw std::invalid_argument("Graph exporter cannot target unknown format");
        }
        insert_or_assign(graph_exporters_, format, std::move(exporter));
    }

    const MeshImporter* GeometryIORegistry::mesh_importer(MeshFileFormat format) const noexcept
    {
        const auto it = mesh_importers_.find(format);
        return it != mesh_importers_.end() ? it->second.get() : nullptr;
    }

    const MeshExporter* GeometryIORegistry::mesh_exporter(MeshFileFormat format) const noexcept
    {
        const auto it = mesh_exporters_.find(format);
        return it != mesh_exporters_.end() ? it->second.get() : nullptr;
    }

    const PointCloudImporter* GeometryIORegistry::point_cloud_importer(PointCloudFileFormat format) const noexcept
    {
        const auto it = point_cloud_importers_.find(format);
        return it != point_cloud_importers_.end() ? it->second.get() : nullptr;
    }

    const PointCloudExporter* GeometryIORegistry::point_cloud_exporter(PointCloudFileFormat format) const noexcept
    {
        const auto it = point_cloud_exporters_.find(format);
        return it != point_cloud_exporters_.end() ? it->second.get() : nullptr;
    }

    const GraphImporter* GeometryIORegistry::graph_importer(GraphFileFormat format) const noexcept
    {
        const auto it = graph_importers_.find(format);
        return it != graph_importers_.end() ? it->second.get() : nullptr;
    }

    const GraphExporter* GeometryIORegistry::graph_exporter(GraphFileFormat format) const noexcept
    {
        const auto it = graph_exporters_.find(format);
        return it != graph_exporters_.end() ? it->second.get() : nullptr;
    }

    GeometryIORegistry& global_geometry_io_registry()
    {
        static GeometryIORegistry registry;
        static std::once_flag init_flag;
        std::call_once(init_flag,
                       [](GeometryIORegistry* registry_ptr) { register_default_geometry_io_plugins(*registry_ptr); },
                       &registry);
        return registry;
    }
} // namespace engine::io

