#pragma once

#include "engine/io/geometry_io_registry.hpp"

namespace engine::io
{
    class PlyPointCloudImporter final : public PointCloudImporter
    {
    public:
        [[nodiscard]] PointCloudFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override;
    };

    class XyzPointCloudImporter final : public PointCloudImporter
    {
    public:
        [[nodiscard]] PointCloudFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override;
    };

    class PcdPointCloudImporter final : public PointCloudImporter
    {
    public:
        [[nodiscard]] PointCloudFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override;
    };
}
