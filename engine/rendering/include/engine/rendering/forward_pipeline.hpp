#pragma once

#include "engine/rendering/runtime_submission.hpp"

namespace engine::rendering
{
    /**
     * \brief Minimal forward rendering pipeline that extracts draw calls from a scene.
     */
    class ForwardPipeline
    {
    public:
        void render(scene::Scene& scene, RuntimeSubmissionContext& submission);
    };
}
