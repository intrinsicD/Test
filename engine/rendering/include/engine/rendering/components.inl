#pragma once

#include <utility>

namespace engine::rendering::components
{
    inline RenderGeometry::RenderGeometry(Geometry geometry, engine::assets::MaterialHandle material) noexcept
        : material(std::move(material)), geometry_(std::move(geometry))
    {
    }

    inline RenderGeometry RenderGeometry::from_mesh(engine::assets::MeshHandle mesh,
                                                    engine::assets::MaterialHandle material)
    {
        return RenderGeometry{std::move(mesh), std::move(material)};
    }

    inline RenderGeometry RenderGeometry::from_graph(engine::assets::GraphHandle graph,
                                                     engine::assets::MaterialHandle material)
    {
        return RenderGeometry{std::move(graph), std::move(material)};
    }

    inline RenderGeometry RenderGeometry::from_point_cloud(engine::assets::PointCloudHandle point_cloud,
                                                           engine::assets::MaterialHandle material)
    {
        return RenderGeometry{std::move(point_cloud), std::move(material)};
    }

    inline bool RenderGeometry::empty() const noexcept
    {
        return std::holds_alternative<std::monostate>(geometry_);
    }

    inline bool RenderGeometry::has_mesh() const noexcept
    {
        return std::holds_alternative<engine::assets::MeshHandle>(geometry_);
    }

    inline bool RenderGeometry::has_graph() const noexcept
    {
        return std::holds_alternative<engine::assets::GraphHandle>(geometry_);
    }

    inline bool RenderGeometry::has_point_cloud() const noexcept
    {
        return std::holds_alternative<engine::assets::PointCloudHandle>(geometry_);
    }

    inline const engine::assets::MeshHandle* RenderGeometry::mesh() const noexcept
    {
        return std::get_if<engine::assets::MeshHandle>(&geometry_);
    }

    inline const engine::assets::GraphHandle* RenderGeometry::graph() const noexcept
    {
        return std::get_if<engine::assets::GraphHandle>(&geometry_);
    }

    inline const engine::assets::PointCloudHandle* RenderGeometry::point_cloud() const noexcept
    {
        return std::get_if<engine::assets::PointCloudHandle>(&geometry_);
    }
}
