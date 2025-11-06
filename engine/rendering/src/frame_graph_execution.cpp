#include "engine/rendering/frame_graph_planner.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/rendering/command_encoder.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering
{
    namespace
    {
        constexpr std::size_t kInvalidIndex = std::numeric_limits<std::size_t>::max();

        class PlannerPassAdapter final : public RenderPass
        {
        public:
            explicit PlannerPassAdapter(const NodeDescriptor& descriptor)
                : RenderPass(std::string{descriptor.id}, descriptor.preferred_queue)
                , descriptor_(descriptor)
            {
            }

            void setup(FrameGraphPassBuilder&) override
            {
            }

            void execute(FrameGraphPassExecutionContext&) override
            {
            }

        private:
            const NodeDescriptor& descriptor_;
        };

        class EncoderScope
        {
        public:
            EncoderScope(CommandEncoderProvider& provider, CommandEncoderDescriptor descriptor,
                std::unique_ptr<CommandEncoder> encoder) noexcept
                : provider_(&provider)
                , descriptor_(descriptor)
                , encoder_(std::move(encoder))
            {
            }

            EncoderScope(const EncoderScope&) = delete;
            EncoderScope& operator=(const EncoderScope&) = delete;

            EncoderScope(EncoderScope&& other) noexcept
                : provider_(std::exchange(other.provider_, nullptr))
                , descriptor_(other.descriptor_)
                , encoder_(std::move(other.encoder_))
            {
            }

            EncoderScope& operator=(EncoderScope&& other) noexcept
            {
                if (this != &other)
                {
                    reset();
                    provider_ = std::exchange(other.provider_, nullptr);
                    descriptor_ = other.descriptor_;
                    encoder_ = std::move(other.encoder_);
                }
                return *this;
            }

            ~EncoderScope()
            {
                reset();
            }

            [[nodiscard]] CommandEncoder* get() const noexcept
            {
                return encoder_.get();
            }

            void reset()
            {
                if (provider_ != nullptr && encoder_ != nullptr)
                {
                    auto local = std::move(encoder_);
                    provider_->end_encoder(descriptor_, std::move(local));
                }
            }

        private:
            CommandEncoderProvider* provider_{nullptr};
            CommandEncoderDescriptor descriptor_{};
            std::unique_ptr<CommandEncoder> encoder_{};
        };

        [[nodiscard]] ResourceUsage usage_from_state(ResourceState state, resources::Access access) noexcept
        {
            using resources::Access;

            switch (state)
            {
            case ResourceState::ShaderRead:
            case ResourceState::CommonRead:
                return Access::Write == access ? ResourceUsage::ShaderWrite : ResourceUsage::ShaderRead;
            case ResourceState::ShaderWrite:
            case ResourceState::CommonWrite:
                return ResourceUsage::ShaderWrite;
            case ResourceState::ColorAttachment:
                return ResourceUsage::ColorAttachment;
            case ResourceState::DepthStencilAttachment:
                return ResourceUsage::DepthStencilAttachment;
            case ResourceState::CopySource:
                return ResourceUsage::TransferSource;
            case ResourceState::CopyDestination:
                return ResourceUsage::TransferDestination;
            case ResourceState::Present:
                return ResourceUsage::Present;
            case ResourceState::Undefined:
            default:
                return ResourceUsage::None;
            }
        }

        struct ResourceUsageAggregation
        {
            ResourceUsage usage{ResourceUsage::None};
            bool has_initial{false};
            ResourceState initial_state{ResourceState::Undefined};
            ResourceState final_state{ResourceState::Undefined};
        };

        [[nodiscard]] bool descriptors_compatible(const FrameGraphPlanner::PlannedResource& lhs,
            const FrameGraphPlanner::PlannedResource& rhs) noexcept
        {
            const auto& a = lhs.descriptor;
            const auto& b = rhs.descriptor;
            return a.kind == b.kind && a.format == b.format && a.dimension == b.dimension && a.width == b.width &&
                a.height == b.height && a.depth == b.depth && a.array_layers == b.array_layers &&
                a.mip_levels == b.mip_levels && a.sample_count == b.sample_count;
        }

        struct RuntimeResourceState
        {
            bool has_state{false};
            resources::PipelineStage stage{resources::PipelineStage::Graphics};
            resources::Access access{resources::Access::None};
            ResourceState state{ResourceState::Undefined};
        };
    }

    struct NodeContext::State
    {
        RenderExecutionContext* render{nullptr};
        CommandEncoderProvider* encoders{nullptr};
        CommandEncoder* encoder{nullptr};
        QueueType queue{QueueType::Graphics};
        const NodeDescriptor* descriptor{nullptr};
        const std::unordered_map<std::string, std::size_t, TransparentStringHash, std::equal_to<>>* lookup{nullptr};
        const std::vector<FrameGraphResourceHandle>* resource_handles{nullptr};
        const std::vector<FrameGraphResourceInfo>* handle_infos{nullptr};
    };

    NodeContext::NodeContext(State* state) noexcept
        : state_(state)
    {
    }

    RenderExecutionContext& NodeContext::render_context() const
    {
        if (state_ == nullptr || state_->render == nullptr)
        {
            throw std::logic_error{"NodeContext accessed without active render context"};
        }
        return *state_->render;
    }

    CommandEncoder& NodeContext::command_encoder() const
    {
        if (state_ == nullptr || state_->encoder == nullptr)
        {
            throw std::logic_error{"NodeContext does not have an active command encoder"};
        }
        return *state_->encoder;
    }

    QueueType NodeContext::queue() const noexcept
    {
        return state_ != nullptr ? state_->queue : QueueType::Graphics;
    }

    const NodeDescriptor& NodeContext::descriptor() const
    {
        if (state_ == nullptr || state_->descriptor == nullptr)
        {
            throw std::logic_error{"NodeContext accessed without descriptor"};
        }
        return *state_->descriptor;
    }

    FrameGraphResourceHandle NodeContext::resource_handle(std::string_view name) const
    {
        if (state_ == nullptr || state_->lookup == nullptr || state_->resource_handles == nullptr)
        {
            throw std::logic_error{"NodeContext cannot resolve resources for inactive state"};
        }

        const auto it = state_->lookup->find(name);
        if (it == state_->lookup->end())
        {
            throw std::out_of_range{"NodeContext::resource_handle unknown resource"};
        }
        const auto& handles = *state_->resource_handles;
        const auto index = it->second;
        if (index >= handles.size())
        {
            throw std::out_of_range{"NodeContext::resource_handle index out of range"};
        }
        return handles[index];
    }

    const FrameGraphResourceInfo& NodeContext::resource_info(FrameGraphResourceHandle handle) const
    {
        if (state_ == nullptr || state_->handle_infos == nullptr)
        {
            throw std::logic_error{"NodeContext cannot resolve resource info for inactive state"};
        }
        if (!handle.valid())
        {
            throw std::out_of_range{"NodeContext::resource_info received invalid handle"};
        }
        const auto& infos = *state_->handle_infos;
        if (handle.index >= infos.size())
        {
            throw std::out_of_range{"NodeContext::resource_info handle out of range"};
        }
        return infos[handle.index];
    }

    const FrameGraphResourceInfo& NodeContext::resource_info(std::string_view name) const
    {
        return resource_info(resource_handle(name));
    }

    void FrameGraphPlanner::Plan::execute(RenderExecutionContext& context, ExecutionTelemetry* telemetry)
    {
        if (passes_.empty())
        {
            return;
        }

        ExecutionTelemetry telemetry_data{};
        auto* telemetry_out = telemetry != nullptr ? telemetry : &telemetry_data;

        std::vector<ResourceUsageAggregation> usage_aggregation(resources_.size());
        for (const auto& pass : passes_)
        {
            const auto accumulate_usage = [&](std::size_t resource_index, const ResourceUse& use)
            {
                auto& record = usage_aggregation[resource_index];
                record.usage |= usage_from_state(use.state, use.access);
                if (!record.has_initial)
                {
                    record.has_initial = true;
                    record.initial_state = use.state;
                }
                record.final_state = use.state;
            };

            for (const auto resource_index : pass.reads)
            {
                const auto& resource = resources_[resource_index];
                const auto* use = pass.descriptor.find_read(resource.name);
                if (use == nullptr)
                {
                    throw std::logic_error{"Planner pass missing read declaration during execution"};
                }
                accumulate_usage(resource_index, *use);
            }

            for (const auto resource_index : pass.writes)
            {
                const auto& resource = resources_[resource_index];
                const auto* use = pass.descriptor.find_write(resource.name);
                if (use == nullptr)
                {
                    throw std::logic_error{"Planner pass missing write declaration during execution"};
                }
                accumulate_usage(resource_index, *use);
            }
        }

        std::unordered_map<std::size_t, std::size_t> alias_to_handle;
        alias_to_handle.reserve(resources_.size());
        std::vector<FrameGraphResourceHandle> resource_handles(resources_.size());
        std::vector<std::size_t> handle_for_resource(resources_.size(), kInvalidIndex);
        std::size_t next_handle_index = 0;

        for (std::size_t index = 0; index < resources_.size(); ++index)
        {
            const auto& resource = resources_[index];
            std::size_t handle_index = next_handle_index;
            if (resource.transient && resource.alias != kInvalidIndex)
            {
                const auto [it, inserted] = alias_to_handle.emplace(resource.alias, next_handle_index);
                if (inserted)
                {
                    ++next_handle_index;
                    handle_index = it->second;
                }
                else
                {
                    handle_index = it->second;
                }
            }
            else
            {
                handle_index = next_handle_index++;
            }

            resource_handles[index].index = handle_index;
            handle_for_resource[index] = handle_index;
        }

        std::vector<FrameGraphResourceInfo> handle_infos(next_handle_index);
        std::vector<bool> handle_initialised(next_handle_index, false);
        std::vector<std::size_t> handle_representative(next_handle_index, kInvalidIndex);

        for (std::size_t index = 0; index < resources_.size(); ++index)
        {
            const auto handle_index = handle_for_resource[index];
            FrameGraphResourceInfo info{};
            const auto& resource = resources_[index];
            const auto& usage = usage_aggregation[index];
            info.name = resource.name;
            info.lifetime = resource.external ? ResourceLifetime::External : ResourceLifetime::Transient;
            info.format = resource.descriptor.format;
            info.dimension = resource.descriptor.dimension;
            info.usage = usage.usage;
            info.initial_state = usage.has_initial ? usage.initial_state : ResourceState::Undefined;
            info.final_state = usage.has_initial ? usage.final_state : ResourceState::Undefined;
            info.width = resource.descriptor.width;
            info.height = resource.descriptor.height;
            info.depth = resource.descriptor.depth;
            info.array_layers = resource.descriptor.array_layers;
            info.mip_levels = resource.descriptor.mip_levels;
            info.sample_count = resource.descriptor.sample_count;
            info.size_bytes = resource.descriptor.dimension == ResourceDimension::Buffer ? resource.descriptor.width : 0U;

            if (!handle_initialised[handle_index])
            {
                handle_infos[handle_index] = info;
                handle_initialised[handle_index] = true;
                handle_representative[handle_index] = index;
            }
            else
            {
                const auto representative_index = handle_representative[handle_index];
                if (representative_index == kInvalidIndex ||
                    !descriptors_compatible(resource, resources_[representative_index]))
                {
                    throw std::runtime_error{"Incompatible resource descriptors share the same transient alias"};
                }
            }
        }

        NodeContext::State node_state{};
        node_state.render = &context;
        node_state.encoders = &context.encoders;
        node_state.lookup = &resource_lookup_;
        node_state.resource_handles = &resource_handles;
        node_state.handle_infos = &handle_infos;

        NodeContext node_context{&node_state};

        for (auto& pass : passes_)
        {
            node_state.descriptor = &pass.descriptor;
            node_state.queue = pass.descriptor.preferred_queue;
            node_state.encoder = nullptr;
            pass.node->Compile(node_context);
        }

        std::vector<RuntimeResourceState> runtime_state(resources_.size());
        std::vector<bool> resource_live(resources_.size(), false);

        resources::TimelineSemaphore timeline{"FrameGraphPlannerTimeline", 0};
        resources::Fence frame_fence{"FrameGraphPlannerFence", 0};
        std::uint64_t timeline_value = 0;

        context.device_resources.begin_frame();

        for (std::size_t pass_index = 0; pass_index < passes_.size(); ++pass_index)
        {
            auto& pass = passes_[pass_index];
            PlannerPassAdapter pass_adapter{pass.descriptor};
            const auto queue = context.scheduler.select_queue(pass_adapter, pass.descriptor.preferred_queue);
            const auto command_buffer = context.scheduler.request_command_buffer(queue, pass.descriptor.id);
            CommandEncoderDescriptor encoder_descriptor{pass.descriptor.id, queue, command_buffer};
            auto encoder = context.encoders.begin_encoder(encoder_descriptor);
            if (encoder == nullptr)
            {
                throw std::runtime_error{"CommandEncoderProvider returned null encoder"};
            }

            EncoderScope encoder_scope{context.encoders, encoder_descriptor, std::move(encoder)};

            const auto acquire_if_needed = [&](std::size_t resource_index)
            {
                auto& resource = resources_[resource_index];
                if (!resource.transient)
                {
                    return;
                }
                if (!resource_live[resource_index] && resource.first_use == pass_index)
                {
                    resource_live[resource_index] = true;
                    context.device_resources.on_transient_acquire(resource_handles[resource_index],
                        handle_infos[handle_for_resource[resource_index]]);
                    ++telemetry_out->transient_acquires;
                }
            };

            const auto release_if_needed = [&](std::size_t resource_index)
            {
                auto& resource = resources_[resource_index];
                if (!resource.transient)
                {
                    return;
                }
                if (resource_live[resource_index] && resource.last_use == pass_index)
                {
                    resource_live[resource_index] = false;
                    context.device_resources.on_transient_release(resource_handles[resource_index],
                        handle_infos[handle_for_resource[resource_index]]);
                    ++telemetry_out->transient_releases;
                }
            };

            const auto update_state = [&](std::size_t resource_index, const ResourceUse& use,
                                           std::vector<resources::Barrier>& barriers)
            {
                auto& state = runtime_state[resource_index];
                if (!state.has_state)
                {
                    state.stage = use.stage;
                    state.access = use.access;
                    state.state = use.state;
                    state.has_state = true;
                    return;
                }

                if (state.stage != use.stage || state.access != use.access || state.state != use.state)
                {
                    resources::Barrier barrier{};
                    barrier.resource = resource_handles[resource_index];
                    barrier.source_stage = state.stage;
                    barrier.destination_stage = use.stage;
                    barrier.source_access = state.access;
                    barrier.destination_access = use.access;
                    barriers.push_back(barrier);
                }

                state.stage = use.stage;
                state.access = use.access;
                state.state = use.state;
            };

            std::vector<resources::Barrier> begin_barriers;
            begin_barriers.reserve(pass.reads.size() + pass.writes.size());

            for (const auto resource_index : pass.creates)
            {
                acquire_if_needed(resource_index);
            }

            for (const auto resource_index : pass.reads)
            {
                acquire_if_needed(resource_index);
                const auto* use = pass.descriptor.find_read(resources_[resource_index].name);
                if (use == nullptr)
                {
                    throw std::logic_error{"Planner pass missing read declaration during execution"};
                }
                update_state(resource_index, *use, begin_barriers);
            }

            for (const auto resource_index : pass.writes)
            {
                acquire_if_needed(resource_index);
                const auto* use = pass.descriptor.find_write(resources_[resource_index].name);
                if (use == nullptr)
                {
                    throw std::logic_error{"Planner pass missing write declaration during execution"};
                }
                update_state(resource_index, *use, begin_barriers);
            }

            node_state.descriptor = &pass.descriptor;
            node_state.queue = queue;
            node_state.encoder = encoder_scope.get();

            pass.node->Execute(node_context);

            node_state.encoder = nullptr;

            encoder_scope.reset();

            std::vector<resources::Barrier> end_barriers;

            GpuSubmitInfo submit{};
            submit.pass_name = pass.descriptor.id;
            submit.queue = queue;
            submit.command_buffer = command_buffer;
            submit.begin_barriers = std::move(begin_barriers);
            submit.end_barriers = std::move(end_barriers);

            if (timeline_value > 0)
            {
                submit.waits.push_back(resources::SemaphoreWait{&timeline, timeline_value});
            }

            const auto submission_value = timeline_value + 1;
            submit.signals.push_back(resources::SemaphoreSignal{&timeline, submission_value});
            submit.fence = &frame_fence;
            submit.fence_value = submission_value;

            context.scheduler.submit(submit);
            context.scheduler.recycle(command_buffer);
            timeline_value = submission_value;
            ++telemetry_out->submissions;

            for (const auto resource_index : pass.reads)
            {
                release_if_needed(resource_index);
            }

            for (const auto resource_index : pass.writes)
            {
                release_if_needed(resource_index);
            }

            for (const auto resource_index : pass.creates)
            {
                release_if_needed(resource_index);
            }
        }

        context.device_resources.end_frame();
    }
}
