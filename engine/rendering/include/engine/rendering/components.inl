#pragma once

#include <utility>

namespace engine::rendering::components
{
    inline RenderGeometry::RenderGeometry(Geometry geometry, assets::MaterialHandle material) noexcept
        : material(std::move(material)), geometry_(std::move(geometry))
    {
    }

    inline RenderGeometry RenderGeometry::from_mesh(assets::MeshHandle mesh,
                                                    assets::MaterialHandle material)
    {
        return RenderGeometry{std::move(mesh), std::move(material)};
    }

    inline RenderGeometry RenderGeometry::from_graph(assets::GraphHandle graph,
                                                     assets::MaterialHandle material)
    {
        return RenderGeometry{std::move(graph), std::move(material)};
    }

    inline RenderGeometry RenderGeometry::from_point_cloud(assets::PointCloudHandle point_cloud,
                                                           assets::MaterialHandle material)
    {
        return RenderGeometry{std::move(point_cloud), std::move(material)};
    }

    inline bool RenderGeometry::empty() const noexcept
    {
        return std::holds_alternative<std::monostate>(geometry_);
    }

    inline bool RenderGeometry::has_mesh() const noexcept
    {
        return std::holds_alternative<assets::MeshHandle>(geometry_);
    }

    inline bool RenderGeometry::has_graph() const noexcept
    {
        return std::holds_alternative<assets::GraphHandle>(geometry_);
    }

    inline bool RenderGeometry::has_point_cloud() const noexcept
    {
        return std::holds_alternative<assets::PointCloudHandle>(geometry_);
    }

    inline const assets::MeshHandle* RenderGeometry::mesh() const noexcept
    {
        return std::get_if<assets::MeshHandle>(&geometry_);
    }

    inline const assets::GraphHandle* RenderGeometry::graph() const noexcept
    {
        return std::get_if<assets::GraphHandle>(&geometry_);
    }

    inline const assets::PointCloudHandle* RenderGeometry::point_cloud() const noexcept
    {
        return std::get_if<assets::PointCloudHandle>(&geometry_);
    }
}
