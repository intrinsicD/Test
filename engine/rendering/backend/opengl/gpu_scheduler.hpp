#pragma once

#include "engine/rendering/backend/stub_gpu_scheduler_base.hpp"

namespace engine::rendering::backend::opengl
{
    /// Stub GPU scheduler for OpenGL contexts; work is executed on the graphics queue.
    class OpenGLGpuScheduler final : public StubGpuSchedulerBase
    {
    public:
        QueueType select_queue(const RenderPass&) override
        {
            return QueueType::Graphics;
        }
    };
}

