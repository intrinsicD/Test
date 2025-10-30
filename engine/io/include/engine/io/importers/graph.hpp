#pragma once

#include "engine/io/geometry_io_registry.hpp"

namespace engine::io
{
    class EdgeListGraphImporter final : public GraphImporter
    {
    public:
        [[nodiscard]] GraphFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        import(const std::filesystem::path& path, geometry::GraphInterface& graph) const override;
    };

}
