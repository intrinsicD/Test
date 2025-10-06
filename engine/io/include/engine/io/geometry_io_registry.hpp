#pragma once

#include "engine/io/api.hpp"
#include "engine/io/geometry_io.hpp"

#include <filesystem>
#include <memory>
#include <unordered_map>

namespace engine::io
{
    class MeshImporter
    {
    public:
        virtual ~MeshImporter() = default;
        [[nodiscard]] virtual MeshFileFormat format() const noexcept = 0;
        virtual void import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const = 0;
    };

    class MeshExporter
    {
    public:
        virtual ~MeshExporter() = default;
        [[nodiscard]] virtual MeshFileFormat format() const noexcept = 0;
        virtual void export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const = 0;
    };

    class PointCloudImporter
    {
    public:
        virtual ~PointCloudImporter() = default;
        [[nodiscard]] virtual PointCloudFileFormat format() const noexcept = 0;
        virtual void import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const = 0;
    };

    class PointCloudExporter
    {
    public:
        virtual ~PointCloudExporter() = default;
        [[nodiscard]] virtual PointCloudFileFormat format() const noexcept = 0;
        virtual void export_point_cloud(const std::filesystem::path& path,
                                        const geometry::PointCloudInterface& point_cloud) const = 0;
    };

    class GraphImporter
    {
    public:
        virtual ~GraphImporter() = default;
        [[nodiscard]] virtual GraphFileFormat format() const noexcept = 0;
        virtual void import(const std::filesystem::path& path, geometry::GraphInterface& graph) const = 0;
    };

    class GraphExporter
    {
    public:
        virtual ~GraphExporter() = default;
        [[nodiscard]] virtual GraphFileFormat format() const noexcept = 0;
        virtual void export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const = 0;
    };

    class GeometryIORegistry
    {
    public:
        GeometryIORegistry();
        GeometryIORegistry(const GeometryIORegistry&) = delete;
        GeometryIORegistry& operator=(const GeometryIORegistry&) = delete;
        GeometryIORegistry(GeometryIORegistry&&) noexcept = delete;
        GeometryIORegistry& operator=(GeometryIORegistry&&) noexcept = delete;
        ~GeometryIORegistry();

        void register_mesh_importer(std::unique_ptr<MeshImporter> importer);
        void register_mesh_exporter(std::unique_ptr<MeshExporter> exporter);

        void register_point_cloud_importer(std::unique_ptr<PointCloudImporter> importer);
        void register_point_cloud_exporter(std::unique_ptr<PointCloudExporter> exporter);

        void register_graph_importer(std::unique_ptr<GraphImporter> importer);
        void register_graph_exporter(std::unique_ptr<GraphExporter> exporter);

        [[nodiscard]] const MeshImporter* mesh_importer(MeshFileFormat format) const noexcept;
        [[nodiscard]] const MeshExporter* mesh_exporter(MeshFileFormat format) const noexcept;

        [[nodiscard]] const PointCloudImporter* point_cloud_importer(PointCloudFileFormat format) const noexcept;
        [[nodiscard]] const PointCloudExporter* point_cloud_exporter(PointCloudFileFormat format) const noexcept;

        [[nodiscard]] const GraphImporter* graph_importer(GraphFileFormat format) const noexcept;
        [[nodiscard]] const GraphExporter* graph_exporter(GraphFileFormat format) const noexcept;

    private:
        struct EnumClassHash
        {
            template <typename Enum>
            [[nodiscard]] std::size_t operator()(Enum value) const noexcept
            {
                return static_cast<std::size_t>(value);
            }
        };

        std::unordered_map<MeshFileFormat, std::unique_ptr<MeshImporter>, EnumClassHash> mesh_importers_;
        std::unordered_map<MeshFileFormat, std::unique_ptr<MeshExporter>, EnumClassHash> mesh_exporters_;

        std::unordered_map<PointCloudFileFormat, std::unique_ptr<PointCloudImporter>, EnumClassHash> point_cloud_importers_;
        std::unordered_map<PointCloudFileFormat, std::unique_ptr<PointCloudExporter>, EnumClassHash> point_cloud_exporters_;

        std::unordered_map<GraphFileFormat, std::unique_ptr<GraphImporter>, EnumClassHash> graph_importers_;
        std::unordered_map<GraphFileFormat, std::unique_ptr<GraphExporter>, EnumClassHash> graph_exporters_;
    };

    ENGINE_IO_API GeometryIORegistry& global_geometry_io_registry();

    ENGINE_IO_API void register_default_geometry_io_plugins(GeometryIORegistry& registry);
} // namespace engine::io

