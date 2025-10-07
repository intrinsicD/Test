#pragma once

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"

namespace engine::rendering
{
    /**
     * \brief Minimal forward rendering pipeline that extracts draw calls from a scene.
     */
    class ForwardPipeline
    {
    public:
        void render(scene::Scene& scene, RenderResourceProvider& resources, MaterialSystem& materials,
                    FrameGraph& graph);
    };
}
