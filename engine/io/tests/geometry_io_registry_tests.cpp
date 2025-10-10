#include <gtest/gtest.h>

#include "engine/io/geometry_io_registry.hpp"

#include <memory>
#include <stdexcept>

namespace
{
    class DummyMeshImporter final : public engine::io::MeshImporter
    {
    public:
        explicit DummyMeshImporter(engine::io::MeshFileFormat format) noexcept
            : format_{format}
        {
        }

        [[nodiscard]] engine::io::MeshFileFormat format() const noexcept override
        {
            return format_;
        }

        void import(const std::filesystem::path& path, engine::geometry::MeshInterface& mesh) const override
        {
            static_cast<void>(path);
            static_cast<void>(mesh);
        }

    private:
        engine::io::MeshFileFormat format_;
    };

    class DummyMeshExporter final : public engine::io::MeshExporter
    {
    public:
        explicit DummyMeshExporter(engine::io::MeshFileFormat format) noexcept
            : format_{format}
        {
        }

        [[nodiscard]] engine::io::MeshFileFormat format() const noexcept override
        {
            return format_;
        }

        void export_mesh(const std::filesystem::path& path, const engine::geometry::MeshInterface& mesh) const override
        {
            static_cast<void>(path);
            static_cast<void>(mesh);
        }

    private:
        engine::io::MeshFileFormat format_;
    };

    class DummyPointCloudImporter final : public engine::io::PointCloudImporter
    {
    public:
        explicit DummyPointCloudImporter(engine::io::PointCloudFileFormat format) noexcept
            : format_{format}
        {
        }

        [[nodiscard]] engine::io::PointCloudFileFormat format() const noexcept override
        {
            return format_;
        }

        void import(const std::filesystem::path& path, engine::geometry::PointCloudInterface& point_cloud) const override
        {
            static_cast<void>(path);
            static_cast<void>(point_cloud);
        }

    private:
        engine::io::PointCloudFileFormat format_;
    };

    class DummyPointCloudExporter final : public engine::io::PointCloudExporter
    {
    public:
        explicit DummyPointCloudExporter(engine::io::PointCloudFileFormat format) noexcept
            : format_{format}
        {
        }

        [[nodiscard]] engine::io::PointCloudFileFormat format() const noexcept override
        {
            return format_;
        }

        void export_point_cloud(const std::filesystem::path& path,
                                 const engine::geometry::PointCloudInterface& point_cloud) const override
        {
            static_cast<void>(path);
            static_cast<void>(point_cloud);
        }

    private:
        engine::io::PointCloudFileFormat format_;
    };

    class DummyGraphImporter final : public engine::io::GraphImporter
    {
    public:
        explicit DummyGraphImporter(engine::io::GraphFileFormat format) noexcept
            : format_{format}
        {
        }

        [[nodiscard]] engine::io::GraphFileFormat format() const noexcept override
        {
            return format_;
        }

        void import(const std::filesystem::path& path, engine::geometry::GraphInterface& graph) const override
        {
            static_cast<void>(path);
            static_cast<void>(graph);
        }

    private:
        engine::io::GraphFileFormat format_;
    };

    class DummyGraphExporter final : public engine::io::GraphExporter
    {
    public:
        explicit DummyGraphExporter(engine::io::GraphFileFormat format) noexcept
            : format_{format}
        {
        }

        [[nodiscard]] engine::io::GraphFileFormat format() const noexcept override
        {
            return format_;
        }

        void export_graph(const std::filesystem::path& path, const engine::geometry::GraphInterface& graph) const override
        {
            static_cast<void>(path);
            static_cast<void>(graph);
        }

    private:
        engine::io::GraphFileFormat format_;
    };
} // namespace

TEST(GeometryIORegistry, RegistersPluginsForAllGeometryKinds)
{
    engine::io::GeometryIORegistry registry;

    registry.register_mesh_importer(std::make_unique<DummyMeshImporter>(engine::io::MeshFileFormat::obj));
    registry.register_mesh_exporter(std::make_unique<DummyMeshExporter>(engine::io::MeshFileFormat::obj));
    EXPECT_TRUE(registry.mesh_importer(engine::io::MeshFileFormat::obj) != nullptr);
    EXPECT_TRUE(registry.mesh_exporter(engine::io::MeshFileFormat::obj) != nullptr);

    registry.register_point_cloud_importer(
        std::make_unique<DummyPointCloudImporter>(engine::io::PointCloudFileFormat::xyz));
    registry.register_point_cloud_exporter(
        std::make_unique<DummyPointCloudExporter>(engine::io::PointCloudFileFormat::xyz));
    EXPECT_TRUE(registry.point_cloud_importer(engine::io::PointCloudFileFormat::xyz) != nullptr);
    EXPECT_TRUE(registry.point_cloud_exporter(engine::io::PointCloudFileFormat::xyz) != nullptr);

    registry.register_graph_importer(std::make_unique<DummyGraphImporter>(engine::io::GraphFileFormat::edgelist));
    registry.register_graph_exporter(std::make_unique<DummyGraphExporter>(engine::io::GraphFileFormat::edgelist));
    EXPECT_TRUE(registry.graph_importer(engine::io::GraphFileFormat::edgelist) != nullptr);
    EXPECT_TRUE(registry.graph_exporter(engine::io::GraphFileFormat::edgelist) != nullptr);
}

TEST(GeometryIORegistry, RejectsNullPlugins)
{
    engine::io::GeometryIORegistry registry;

    EXPECT_THROW(registry.register_mesh_importer(nullptr), std::invalid_argument);
    EXPECT_THROW(registry.register_mesh_exporter(nullptr), std::invalid_argument);

    EXPECT_THROW(registry.register_point_cloud_importer(nullptr), std::invalid_argument);
    EXPECT_THROW(registry.register_point_cloud_exporter(nullptr), std::invalid_argument);

    EXPECT_THROW(registry.register_graph_importer(nullptr), std::invalid_argument);
    EXPECT_THROW(registry.register_graph_exporter(nullptr), std::invalid_argument);
}

TEST(GeometryIORegistry, RejectsUnknownFormats)
{
    engine::io::GeometryIORegistry registry;

    EXPECT_THROW(registry.register_mesh_importer(std::make_unique<DummyMeshImporter>(engine::io::MeshFileFormat::unknown)),
                 std::invalid_argument);
    EXPECT_THROW(registry.register_mesh_exporter(std::make_unique<DummyMeshExporter>(engine::io::MeshFileFormat::unknown)),
                 std::invalid_argument);

    EXPECT_THROW(registry.register_point_cloud_importer(
                     std::make_unique<DummyPointCloudImporter>(engine::io::PointCloudFileFormat::unknown)),
                 std::invalid_argument);
    EXPECT_THROW(registry.register_point_cloud_exporter(
                     std::make_unique<DummyPointCloudExporter>(engine::io::PointCloudFileFormat::unknown)),
                 std::invalid_argument);

    EXPECT_THROW(registry.register_graph_importer(std::make_unique<DummyGraphImporter>(engine::io::GraphFileFormat::unknown)),
                 std::invalid_argument);
    EXPECT_THROW(registry.register_graph_exporter(std::make_unique<DummyGraphExporter>(engine::io::GraphFileFormat::unknown)),
                 std::invalid_argument);
}

TEST(GeometryIORegistry, GlobalRegistryProvidesDefaultPlugins)
{
    engine::io::GeometryIORegistry* first = nullptr;
    bool first_call_succeeded = true;
    try
    {
        first = std::addressof(engine::io::global_geometry_io_registry());
    }
    catch (...)
    {
        first_call_succeeded = false;
    }
    EXPECT_TRUE(first_call_succeeded);

    engine::io::GeometryIORegistry* second = nullptr;
    bool second_call_succeeded = true;
    try
    {
        second = std::addressof(engine::io::global_geometry_io_registry());
    }
    catch (...)
    {
        second_call_succeeded = false;
    }
    EXPECT_TRUE(second_call_succeeded);

    ASSERT_TRUE(first != nullptr);
    ASSERT_TRUE(second != nullptr);
    EXPECT_EQ(first, second);

    const auto& registry = *first;
    EXPECT_TRUE(registry.mesh_importer(engine::io::MeshFileFormat::obj) != nullptr);
    EXPECT_TRUE(registry.mesh_exporter(engine::io::MeshFileFormat::obj) != nullptr);
    EXPECT_TRUE(registry.point_cloud_importer(engine::io::PointCloudFileFormat::ply) != nullptr);
    EXPECT_TRUE(registry.point_cloud_exporter(engine::io::PointCloudFileFormat::ply) != nullptr);
    EXPECT_TRUE(registry.graph_importer(engine::io::GraphFileFormat::edgelist) != nullptr);
    EXPECT_TRUE(registry.graph_exporter(engine::io::GraphFileFormat::edgelist) != nullptr);
}
