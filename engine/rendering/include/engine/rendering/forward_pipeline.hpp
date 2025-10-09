#pragma once

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering
{
    /**
     * \brief Minimal forward rendering pipeline that extracts draw calls from a scene.
     */
    class ForwardPipeline
    {
    public:
        void render(scene::Scene& scene, RenderResourceProvider& resources, MaterialSystem& materials,
                    resources::IGpuResourceProvider& device_resources, IGpuScheduler& scheduler,
                    FrameGraph& graph);
    };
}
