#pragma once

#include "engine/rendering/command_encoder.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::scene
{
    class Scene;
} // namespace engine::scene

namespace engine::rendering
{
    class ForwardPipeline;

    /**
     * \brief Submission context shared between the runtime and rendering backends.
     *
     * Runtime code populates this structure with the render dependencies that the
     * frame-graph requires in order to execute. Rendering pipelines can transform
     * it into a RenderExecutionContext via `make_execution_context` and reuse the
     * compiled frame-graph stored in `frame_graph`.
     */
    struct RuntimeSubmissionContext
    {
        RenderResourceProvider& resources;
        MaterialSystem& materials;
        resources::IGpuResourceProvider& device_resources;
        IGpuScheduler& scheduler;
        CommandEncoderProvider& encoders;
        FrameGraph& frame_graph;
        ForwardPipeline* pipeline{nullptr};

        [[nodiscard]] RenderExecutionContext make_execution_context(scene::Scene& scene) const noexcept
        {
            return RenderExecutionContext{
                resources,
                materials,
                RenderView{scene},
                scheduler,
                device_resources,
                encoders,
            };
        }
    };
} // namespace engine::rendering

