#pragma once

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
        /// Handle of the mesh asset containing vertex and index buffers.
        engine::assets::MeshHandle mesh{};

        /// Handle of the material definition to bind when the mesh is drawn.
        engine::assets::MaterialHandle material{};
    };
} // namespace engine::rendering::components
