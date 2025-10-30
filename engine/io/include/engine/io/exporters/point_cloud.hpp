#pragma once

#include "engine/io/geometry_io_registry.hpp"

namespace engine::io
{
    class PlyPointCloudExporter final : public PointCloudExporter
    {
    public:
        [[nodiscard]] PointCloudFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_point_cloud(const std::filesystem::path& path,
                           const geometry::PointCloudInterface& point_cloud) const override;
    };

    class XyzPointCloudExporter final : public PointCloudExporter
    {
    public:
        [[nodiscard]] PointCloudFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_point_cloud(const std::filesystem::path& path,
                           const geometry::PointCloudInterface& point_cloud) const override;
    };

    class PcdPointCloudExporter final : public PointCloudExporter
    {
    public:
        [[nodiscard]] PointCloudFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_point_cloud(const std::filesystem::path& path,
                           const geometry::PointCloudInterface& point_cloud) const override;
    };
}
