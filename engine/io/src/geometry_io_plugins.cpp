#include "engine/io/geometry_io_registry.hpp"

#include "engine/io/importers/mesh.hpp"
#include "engine/io/importers/point_cloud.hpp"
#include "engine/io/importers/graph.hpp"
#include "engine/io/exporters/mesh.hpp"
#include "engine/io/exporters/point_cloud.hpp"
#include "engine/io/exporters/graph.hpp"

#include <memory>

namespace engine::io
{
    void register_default_geometry_io_plugins(GeometryIORegistry& registry)
    {
        registry.register_mesh_importer(std::make_unique<ObjMeshImporter>());
        registry.register_mesh_importer(std::make_unique<OffMeshImporter>());
        registry.register_mesh_importer(std::make_unique<PlyMeshImporter>());

        registry.register_mesh_exporter(std::make_unique<ObjMeshExporter>());
        registry.register_mesh_exporter(std::make_unique<OffMeshExporter>());
        registry.register_mesh_exporter(std::make_unique<PlyMeshExporter>());

        registry.register_point_cloud_importer(std::make_unique<PlyPointCloudImporter>());
        registry.register_point_cloud_importer(std::make_unique<XyzPointCloudImporter>());
        registry.register_point_cloud_importer(std::make_unique<PcdPointCloudImporter>());

        registry.register_point_cloud_exporter(std::make_unique<PlyPointCloudExporter>());
        registry.register_point_cloud_exporter(std::make_unique<XyzPointCloudExporter>());
        registry.register_point_cloud_exporter(std::make_unique<PcdPointCloudExporter>());

        registry.register_graph_importer(std::make_unique<EdgeListGraphImporter>());
        registry.register_graph_importer(std::make_unique<PlyGraphImporter>());
        registry.register_graph_exporter(std::make_unique<EdgeListGraphExporter>());
        registry.register_graph_exporter(std::make_unique<PlyGraphExporter>());
    }
} // namespace engine::io
