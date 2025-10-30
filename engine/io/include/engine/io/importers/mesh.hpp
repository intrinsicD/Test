#pragma once

#include "engine/io/geometry_io_registry.hpp"

namespace engine::io
{
    class ObjMeshImporter final : public MeshImporter
    {
    public:
        [[nodiscard]] MeshFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override;
    };

    class OffMeshImporter final : public MeshImporter
    {
    public:
        [[nodiscard]] MeshFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override;
    };

    class PlyMeshImporter final : public MeshImporter
    {
    public:
        [[nodiscard]] MeshFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override;
    };
}
