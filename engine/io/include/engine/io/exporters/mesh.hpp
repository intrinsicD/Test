#pragma once

#include "engine/io/geometry_io_registry.hpp"

namespace engine::io
{
    class ObjMeshExporter final : public MeshExporter
    {
    public:
        [[nodiscard]] MeshFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override;
    };

    class OffMeshExporter final : public MeshExporter
    {
    public:
        [[nodiscard]] MeshFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override;
    };

    class PlyMeshExporter final : public MeshExporter
    {
    public:
        [[nodiscard]] MeshFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override;
    };
}
