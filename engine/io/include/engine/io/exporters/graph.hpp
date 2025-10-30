#pragma once

#include "engine/io/geometry_io_registry.hpp"

namespace engine::io
{
    class EdgeListGraphExporter final : public GraphExporter
    {
    public:
        [[nodiscard]] GraphFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const override;
    };

    class PlyGraphExporter final : public GraphExporter
    {
    public:
        [[nodiscard]] GraphFileFormat format() const noexcept override;
        [[nodiscard]] GeometryIoResult<void>
        export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const override;
    };

}
