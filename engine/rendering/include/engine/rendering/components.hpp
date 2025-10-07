#pragma once

#include <variant>

#include "engine/assets/handles.hpp"

namespace engine::rendering::components
{
    /**
     * \brief Geometry component consumed by the rendering pipeline.
     *
     * Entities that should be rendered attach this component together with the
     * transform components supplied by `engine::scene`.  The renderer will look
     * for a `WorldTransform` on the same entity in order to obtain the final
     * object-to-world matrix.
     */
    struct RenderGeometry
    {
        using Geometry = std::variant<std::monostate, assets::MeshHandle, assets::GraphHandle,
                                      assets::PointCloudHandle>;

        RenderGeometry() = default;

        static RenderGeometry from_mesh(assets::MeshHandle mesh,
                                        assets::MaterialHandle material = {});

        static RenderGeometry from_graph(assets::GraphHandle graph,
                                         assets::MaterialHandle material = {});

        static RenderGeometry from_point_cloud(assets::PointCloudHandle point_cloud,
                                               assets::MaterialHandle material = {});

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] bool has_mesh() const noexcept;
        [[nodiscard]] bool has_graph() const noexcept;
        [[nodiscard]] bool has_point_cloud() const noexcept;

        [[nodiscard]] const assets::MeshHandle* mesh() const noexcept;
        [[nodiscard]] const assets::GraphHandle* graph() const noexcept;
        [[nodiscard]] const assets::PointCloudHandle* point_cloud() const noexcept;

        [[nodiscard]] const Geometry& geometry() const noexcept { return geometry_; }

        /// Handle of the material definition to bind when the geometry is drawn.
        assets::MaterialHandle material{};

    private:
        explicit RenderGeometry(Geometry geometry, assets::MaterialHandle material) noexcept;

        Geometry geometry_{};
    };
}

#include "engine/rendering/components.inl"
