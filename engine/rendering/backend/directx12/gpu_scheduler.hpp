#pragma once

#include <string_view>

#include "engine/rendering/backend/stub_gpu_scheduler_base.hpp"

namespace engine::rendering::backend::directx12
{
    /// Stub GPU scheduler modelling DirectX 12 command queue selection.
    class DirectX12GpuScheduler final : public StubGpuSchedulerBase
    {
    public:
        QueueType select_queue(const RenderPass& pass) override
        {
            const auto name = pass.name();
            if (name.find("Copy") != std::string_view::npos)
            {
                return QueueType::Transfer;
            }
            if (name.find("Compute") != std::string_view::npos)
            {
                return QueueType::Compute;
            }
            return QueueType::Graphics;
        }
    };
}

