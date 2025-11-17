#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/rendering/backend/native_scheduler_base.hpp"
#include "engine/rendering/command/command_buffer_pool.hpp"
#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace
{
    using namespace engine::rendering;

    class TrackingResourceProvider final : public resources::IGpuResourceProvider
    {
    public:
        explicit TrackingResourceProvider(resources::GraphicsApi api = resources::GraphicsApi::Unknown)
            : api_(api)
        {
        }

        resources::GraphicsApi api() const noexcept override { return api_; }
        void begin_frame() override { ++frames_begun_; }
        void end_frame() override { ++frames_completed_; }

        resources::QueueNativeHandle queue_handle(QueueType queue) const override
        {
            resources::QueueNativeHandle handle{};
            handle.api = api_;
            handle.queue = queue;
            handle.value = static_cast<std::uintptr_t>(queue);
            return handle;
        }

        resources::CommandBufferNativeHandle allocate_command_buffer(QueueType queue, std::string_view label,
                                                                     CommandBufferHandle handle) override
        {
            ++allocate_calls_;
            resources::CommandBufferNativeHandle native{};
            native.api = api_;
            native.queue = queue;
            native.value = ++next_command_buffer_value_;
            native.index = handle.index;
            native.label = std::string{label};
            handles_.insert_or_assign(handle.index, queue);
            return native;
        }

        void recycle_command_buffer(CommandBufferHandle handle) override
        {
            ++recycle_calls_;
            handles_.erase(handle.index);
        }

        resources::FenceNativeHandle resolve_fence(const resources::Fence& fence) override
        {
            resources::FenceNativeHandle native{};
            native.api = api_;
            native.value = reinterpret_cast<std::uintptr_t>(&fence);
            return native;
        }

        resources::TimelineSemaphoreNativeHandle resolve_semaphore(
            const resources::TimelineSemaphore& semaphore) override
        {
            resources::TimelineSemaphoreNativeHandle native{};
            native.api = api_;
            native.value = reinterpret_cast<std::uintptr_t>(&semaphore);
            return native;
        }

        resources::GpuResourceUsage usage_snapshot() const noexcept override { return {}; }
        void on_transient_acquire(FrameGraphResourceHandle, const FrameGraphResourceInfo&) override {}
        void on_transient_release(FrameGraphResourceHandle, const FrameGraphResourceInfo&) override {}

        [[nodiscard]] std::size_t allocate_calls() const noexcept { return allocate_calls_; }
        [[nodiscard]] std::size_t recycle_calls() const noexcept { return recycle_calls_; }
        [[nodiscard]] std::size_t live_handles() const noexcept { return handles_.size(); }
        [[nodiscard]] std::size_t frames_begun() const noexcept { return frames_begun_; }
        [[nodiscard]] std::size_t frames_completed() const noexcept { return frames_completed_; }

    private:
        resources::GraphicsApi api_;
        mutable std::unordered_map<std::size_t, QueueType> handles_{};
        std::size_t allocate_calls_{0};
        std::size_t recycle_calls_{0};
        std::size_t frames_begun_{0};
        std::size_t frames_completed_{0};
        std::uint64_t next_command_buffer_value_{0};
    };

    struct DummySubmission
    {
        resources::CommandBufferNativeHandle native{};
    };

    class TestScheduler final : public backend::NativeSchedulerBase<TestScheduler, DummySubmission>
    {
    public:
        using Base = backend::NativeSchedulerBase<TestScheduler, DummySubmission>;
        using Base::NativeSchedulerBase;
        using typename Base::EncoderRecord;

        QueueType select_queue(const RenderPass&, QueueType preferred) override
        {
            return preferred;
        }

        DummySubmission build_submission(const GpuSubmitInfo&, const EncoderRecord& encoder)
        {
            DummySubmission submission{};
            submission.native = encoder.native;
            return submission;
        }
    };
}

TEST(CommandScheduler, ReusesHandlesAndTracksHits)
{
    TrackingResourceProvider provider{};
    TestScheduler scheduler(provider);

    scheduler.begin_frame();
    const auto first = scheduler.request_command_buffer(QueueType::Graphics, "first_pass");
    scheduler.recycle(first);
    scheduler.end_frame();

    scheduler.begin_frame();
    const auto second = scheduler.request_command_buffer(QueueType::Graphics, "second_pass");
    scheduler.recycle(second);
    scheduler.end_frame();

    EXPECT_EQ(first.index, second.index);
    auto metrics = scheduler.command_buffer_pool_metrics();
    EXPECT_EQ(metrics.requests, 2U);
    EXPECT_EQ(metrics.hits, 1U);
    EXPECT_GE(metrics.hit_rate(), 0.49);
    EXPECT_EQ(provider.recycle_calls(), 2U);
}

TEST(CommandScheduler, TrimsHandlesPastRetentionWindow)
{
    TrackingResourceProvider provider{};
    TestScheduler scheduler(provider);
    scheduler.set_command_buffer_retention_frames(1);

    scheduler.begin_frame();
    const auto handle = scheduler.request_command_buffer(QueueType::Graphics, "frame0");
    scheduler.recycle(handle);
    scheduler.end_frame();

    scheduler.begin_frame();
    scheduler.end_frame();

    scheduler.begin_frame();
    scheduler.end_frame();

    auto metrics = scheduler.command_buffer_pool_metrics();
    EXPECT_EQ(metrics.trims, 1U);
    EXPECT_EQ(metrics.pooled_handles, 0U);
}
