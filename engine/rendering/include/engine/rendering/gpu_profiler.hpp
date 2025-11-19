#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering
{
    /**
     * \brief Lightweight profiler that associates frame-graph passes with GPU timestamp samples.
     *
     * The profiler records begin/end timestamps for every pass executed by the frame-graph and
     * exposes the results as per-pass telemetry consumable by RuntimeDiagnostics and tooling
     * panels. Backends lacking native timestamp query support fall back to CPU monotonic clocks,
     * ensuring panels still render relative ordering even when precise GPU timings are
     * unavailable.
     */
    class GpuProfiler
    {
    public:
        struct PassTiming
        {
            std::string pass_name{};
            QueueType queue{QueueType::Graphics};
            CommandBufferHandle command_buffer{};
            double gpu_time_ms{0.0};
            std::uint64_t timestamp_begin_ns{0};
            std::uint64_t timestamp_end_ns{0};
        };

        GpuProfiler() = default;

        void set_enabled(bool enabled) noexcept;
        void begin_frame(resources::GraphicsApi api) noexcept;
        void end_frame() noexcept;
        void begin_pass(std::string_view pass_name, QueueType queue, CommandBufferHandle handle);
        void end_pass(CommandBufferHandle handle);
        [[nodiscard]] const std::vector<PassTiming>& pass_timings() const noexcept;
        [[nodiscard]] std::vector<PassTiming> consume_pass_timings();

    private:
        using Clock = std::chrono::steady_clock;

        struct PendingPass
        {
            std::string name{};
            QueueType queue{QueueType::Graphics};
            CommandBufferHandle command_buffer{};
            std::uint64_t begin_ns{0};
        };

        [[nodiscard]] static std::uint64_t now_ns() noexcept;

        bool enabled_{true};
        resources::GraphicsApi api_{resources::GraphicsApi::Unknown};
        std::uint64_t frame_index_{0};
        std::unordered_map<std::size_t, PendingPass> pending_{};
        std::vector<PassTiming> completed_{};
    };
} // namespace engine::rendering

