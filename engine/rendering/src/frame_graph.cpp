#include "engine/rendering/frame_graph.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>

#include "engine/rendering/command_encoder.hpp"

namespace engine::rendering
{
    FrameGraphPassBuilder::FrameGraphPassBuilder(FrameGraph& graph, std::size_t pass_index)
        : graph_(graph), pass_index_(pass_index)
    {
    }

    FrameGraphResourceHandle FrameGraphPassBuilder::read(FrameGraphResourceHandle handle)
    {
        if (!handle.valid() || handle.index >= graph_.resources_.size())
        {
            throw std::out_of_range{"FrameGraphPassBuilder::read received invalid resource handle"};
        }

        auto& node = graph_.passes_[pass_index_];
        if (std::find(node.reads.begin(), node.reads.end(), handle) == node.reads.end())
        {
            node.reads.push_back(handle);
        }

        auto& resource = graph_.resources_[handle.index];
        if (std::find(resource.readers.begin(), resource.readers.end(), pass_index_) == resource.readers.end())
        {
            resource.readers.push_back(pass_index_);
        }

        return handle;
    }

    FrameGraphResourceHandle FrameGraphPassBuilder::write(FrameGraphResourceHandle handle)
    {
        if (!handle.valid() || handle.index >= graph_.resources_.size())
        {
            throw std::out_of_range{"FrameGraphPassBuilder::write received invalid resource handle"};
        }

        auto& node = graph_.passes_[pass_index_];
        if (std::find(node.writes.begin(), node.writes.end(), handle) == node.writes.end())
        {
            node.writes.push_back(handle);
        }

        auto& resource = graph_.resources_[handle.index];
        if (resource.writer != std::numeric_limits<std::size_t>::max() && resource.writer != pass_index_)
        {
            throw std::logic_error{"FrameGraph resource already has a writer"};
        }
        resource.writer = pass_index_;

        return handle;
    }

    std::string_view FrameGraphPassExecutionContext::pass_name() const
    {
        return graph.pass_name(pass_index);
    }

    std::span<const FrameGraphResourceHandle> FrameGraphPassExecutionContext::reads() const
    {
        return graph.pass_reads(pass_index);
    }

    std::span<const FrameGraphResourceHandle> FrameGraphPassExecutionContext::writes() const
    {
        return graph.pass_writes(pass_index);
    }

    FrameGraphResourceInfo FrameGraphPassExecutionContext::describe(FrameGraphResourceHandle handle) const
    {
        return graph.resource_info(handle);
    }

    CommandBufferHandle FrameGraphPassExecutionContext::command_buffer_handle() const noexcept
    {
        return command_buffer;
    }

    QueueType FrameGraphPassExecutionContext::queue_type() const noexcept
    {
        return queue;
    }

    CommandEncoder& FrameGraphPassExecutionContext::command_encoder() const
    {
        if (encoder == nullptr)
        {
            throw std::logic_error{
                "FrameGraphPassExecutionContext::command_encoder accessed without an active encoder"};
        }
        return *encoder;
    }

    FrameGraph::FrameGraph() = default;

    void FrameGraph::reset()
    {
        resources_.clear();
        passes_.clear();
        execution_order_.clear();
        resource_events_.clear();
        pass_begin_barriers_.clear();
        pass_end_barriers_.clear();
        compiled_ = false;
    }

    FrameGraphResourceHandle FrameGraph::create_resource(FrameGraphResourceDescriptor descriptor)
    {
        compiled_ = false;
        resources_.push_back(ResourceNode{});
        auto& node = resources_.back();
        node.name = std::move(descriptor.name);
        node.lifetime = descriptor.lifetime;
        node.format = descriptor.format;
        node.dimension = descriptor.dimension;
        node.usage = descriptor.usage;
        node.initial_state = descriptor.initial_state;
        node.final_state = descriptor.final_state;
        return FrameGraphResourceHandle{resources_.size() - 1};
    }

    FrameGraphResourceHandle FrameGraph::create_resource(std::string name, ResourceLifetime lifetime)
    {
        FrameGraphResourceDescriptor descriptor{};
        descriptor.name = std::move(name);
        descriptor.lifetime = lifetime;
        return create_resource(std::move(descriptor));
    }

    std::size_t FrameGraph::add_pass(std::unique_ptr<RenderPass> pass)
    {
        if (!pass)
        {
            throw std::invalid_argument{"FrameGraph::add_pass expects a valid RenderPass"};
        }

        compiled_ = false;
        passes_.push_back(PassNode{});
        auto& node = passes_.back();
        node.pass = std::move(pass);
        const std::size_t index = passes_.size() - 1;

        FrameGraphPassBuilder builder{*this, index};
        node.pass->setup(builder);
        return index;
    }

    void FrameGraph::compile()
    {
        execution_order_.clear();
        resource_events_.clear();

        if (passes_.empty())
        {
            compiled_ = true;
            return;
        }

        for (const auto& resource : resources_)
        {
            if (resource.name.empty())
            {
                throw std::logic_error{"FrameGraph resource missing debug name"};
            }
            if (resource.dimension == ResourceDimension::Unknown)
            {
                throw std::logic_error("FrameGraph resource '" + resource.name + "' missing dimension metadata");
            }
            if (!any(resource.usage))
            {
                throw std::logic_error("FrameGraph resource '" + resource.name + "' missing usage metadata");
            }
            if (resource.dimension != ResourceDimension::Buffer && resource.format == ResourceFormat::Unknown)
            {
                throw std::logic_error("FrameGraph resource '" + resource.name + "' missing format metadata");
            }
            if (resource.initial_state == ResourceState::Undefined)
            {
                throw std::logic_error("FrameGraph resource '" + resource.name + "' missing initial state metadata");
            }
            if (resource.final_state == ResourceState::Undefined)
            {
                throw std::logic_error("FrameGraph resource '" + resource.name + "' missing final state metadata");
            }
        }

        std::vector<std::vector<std::size_t>> adjacency(passes_.size());
        std::vector<std::size_t> indegree(passes_.size(), 0);

        for (std::size_t resource_index = 0; resource_index < resources_.size(); ++resource_index)
        {
            auto& resource = resources_[resource_index];
            if (resource.writer != std::numeric_limits<std::size_t>::max())
            {
                for (std::size_t reader : resource.readers)
                {
                    adjacency[resource.writer].push_back(reader);
                }
            }
        }

        for (std::size_t pass_index = 0; pass_index < adjacency.size(); ++pass_index)
        {
            auto& edges = adjacency[pass_index];
            std::sort(edges.begin(), edges.end());
            edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
            for (std::size_t target : edges)
            {
                ++indegree[target];
            }
        }

        std::queue<std::size_t> ready;
        for (std::size_t i = 0; i < indegree.size(); ++i)
        {
            if (indegree[i] == 0)
            {
                ready.push(i);
            }
        }

        while (!ready.empty())
        {
            const std::size_t node_index = ready.front();
            ready.pop();
            execution_order_.push_back(node_index);

            for (std::size_t edge : adjacency[node_index])
            {
                if (--indegree[edge] == 0)
                {
                    ready.push(edge);
                }
            }
        }

        if (execution_order_.size() != passes_.size())
        {
            throw std::logic_error{"FrameGraph contains cyclic dependencies"};
        }

        for (auto& resource : resources_)
        {
            resource.first_use = std::numeric_limits<std::size_t>::max();
            resource.last_use = std::numeric_limits<std::size_t>::max();
        }

        pass_begin_barriers_.assign(passes_.size(), {});
        pass_end_barriers_.assign(passes_.size(), {});

        auto make_barrier = [](FrameGraphResourceHandle handle, resources::Access source_access,
                               resources::Access destination_access) {
            resources::Barrier barrier{};
            barrier.resource = handle;
            barrier.source_stage = resources::PipelineStage::Graphics;
            barrier.destination_stage = resources::PipelineStage::Graphics;
            barrier.source_access = source_access;
            barrier.destination_access = destination_access;
            return barrier;
        };

        for (std::size_t order_index = 0; order_index < execution_order_.size(); ++order_index)
        {
            const std::size_t pass_index = execution_order_[order_index];
            const auto& pass = passes_[pass_index];

            auto update_use = [&](FrameGraphResourceHandle handle) {
                auto& resource = resources_[handle.index];
                resource.first_use = std::min(resource.first_use, order_index);
                if (resource.last_use == std::numeric_limits<std::size_t>::max())
                {
                    resource.last_use = order_index;
                }
                else
                {
                    resource.last_use = std::max(resource.last_use, order_index);
                }
            };

            auto& begin_barriers = pass_begin_barriers_[pass_index];
            auto& end_barriers = pass_end_barriers_[pass_index];

            for (const auto handle : pass.reads)
            {
                update_use(handle);
                const auto& resource = resources_[handle.index];
                const auto writer = resource.writer;
                const auto source_access =
                    writer == std::numeric_limits<std::size_t>::max() ? resources::Access::Read
                                                                      : resources::Access::Write;
                begin_barriers.push_back(make_barrier(handle, source_access, resources::Access::Read));
            }

            for (const auto handle : pass.writes)
            {
                update_use(handle);
                begin_barriers.push_back(make_barrier(handle, resources::Access::Read, resources::Access::Write));
                end_barriers.push_back(make_barrier(handle, resources::Access::Write, resources::Access::Read));
            }
        }

        compiled_ = true;
    }

    void FrameGraph::execute(RenderExecutionContext& context)
    {
        if (!compiled_)
        {
            compile();
        }

        if (execution_order_.empty())
        {
            return;
        }

        resource_events_.clear();
        std::vector<bool> alive(resources_.size(), false);
        resources::TimelineSemaphore frame_semaphore{"FrameGraphTimeline", 0};
        resources::Fence frame_fence{"FrameGraphFence", 0};
        std::uint64_t timeline_value = 0;

        context.device_resources.begin_frame();

        for (std::size_t order_index = 0; order_index < execution_order_.size(); ++order_index)
        {
            const std::size_t pass_index = execution_order_[order_index];
            auto& pass = passes_[pass_index];

            const auto queue = context.scheduler.select_queue(*pass.pass, pass.pass->queue());
            const auto command_buffer = context.scheduler.request_command_buffer(queue, pass.pass->name());

            CommandEncoderDescriptor encoder_descriptor{pass.pass->name(), queue, command_buffer};
            auto encoder = context.encoders.begin_encoder(encoder_descriptor);
            if (encoder == nullptr)
            {
                throw std::runtime_error{"CommandEncoderProvider returned null encoder"};
            }

            auto signal_acquire = [&](FrameGraphResourceHandle handle) {
                auto& resource = resources_[handle.index];
                if (resource.lifetime == ResourceLifetime::Transient &&
                    resource.first_use == order_index && !alive[handle.index])
                {
                    alive[handle.index] = true;
                    resource_events_.push_back(ResourceEvent{ResourceEvent::Type::Acquire, resource.name,
                                                              std::string(pass.pass->name())});
                    context.device_resources.on_transient_acquire(handle, resource_info(handle));
                }
            };

            auto signal_release = [&](FrameGraphResourceHandle handle) {
                auto& resource = resources_[handle.index];
                if (resource.lifetime == ResourceLifetime::Transient &&
                    resource.last_use == order_index && alive[handle.index])
                {
                    alive[handle.index] = false;
                    resource_events_.push_back(ResourceEvent{ResourceEvent::Type::Release, resource.name,
                                                              std::string(pass.pass->name())});
                    context.device_resources.on_transient_release(handle, resource_info(handle));
                }
            };

            for (const auto handle : pass.reads)
            {
                signal_acquire(handle);
            }

            FrameGraphPassExecutionContext pass_context{context, *this, pass_index};
            pass_context.command_buffer = command_buffer;
            pass_context.queue = queue;
            pass_context.encoder = encoder.get();
            pass.pass->execute(pass_context);
            pass_context.encoder = nullptr;

            for (const auto handle : pass.reads)
            {
                signal_release(handle);
            }
            for (const auto handle : pass.writes)
            {
                signal_acquire(handle);
            }
            for (const auto handle : pass.writes)
            {
                signal_release(handle);
            }

            auto begin_barriers = pass_begin_barriers_[pass_index];
            auto end_barriers = pass_end_barriers_[pass_index];

            const auto stage_for_queue = [](QueueType queue_type) {
                switch (queue_type)
                {
                case QueueType::Graphics:
                    return resources::PipelineStage::Graphics;
                case QueueType::Compute:
                    return resources::PipelineStage::Compute;
                case QueueType::Transfer:
                    return resources::PipelineStage::Transfer;
                }
                return resources::PipelineStage::Graphics;
            };

            const auto queue_stage = stage_for_queue(queue);
            for (auto& barrier : begin_barriers)
            {
                barrier.destination_stage = queue_stage;
            }
            for (auto& barrier : end_barriers)
            {
                barrier.source_stage = queue_stage;
            }

            GpuSubmitInfo submit_info{};
            submit_info.pass_name = pass.pass->name();
            submit_info.queue = queue;
            submit_info.command_buffer = command_buffer;
            submit_info.begin_barriers = std::move(begin_barriers);
            submit_info.end_barriers = std::move(end_barriers);

            if (timeline_value > 0)
            {
                submit_info.waits.push_back(resources::SemaphoreWait{&frame_semaphore, timeline_value});
            }

            const auto submission_value = timeline_value + 1;
            submit_info.signals.push_back(resources::SemaphoreSignal{&frame_semaphore, submission_value});
            submit_info.fence = &frame_fence;
            submit_info.fence_value = submission_value;

            context.scheduler.submit(submit_info);
            context.scheduler.recycle(command_buffer);
            context.encoders.end_encoder(encoder_descriptor, std::move(encoder));
            timeline_value = submission_value;
        }

        context.device_resources.end_frame();
    }

    const std::vector<std::size_t>& FrameGraph::execution_order() const noexcept
    {
        return execution_order_;
    }

    const std::vector<ResourceEvent>& FrameGraph::resource_events() const noexcept
    {
        return resource_events_;
    }

    FrameGraphResourceInfo FrameGraph::resource_info(FrameGraphResourceHandle handle) const
    {
        if (!handle.valid() || handle.index >= resources_.size())
        {
            throw std::out_of_range{"FrameGraph::resource_info received invalid handle"};
        }

        const auto& resource = resources_[handle.index];
        return FrameGraphResourceInfo{resource.name,
                                      resource.lifetime,
                                      resource.format,
                                      resource.dimension,
                                      resource.usage,
                                      resource.initial_state,
                                      resource.final_state};
    }

    std::span<const FrameGraphResourceHandle> FrameGraph::pass_reads(std::size_t pass_index)
    {
        if (pass_index >= passes_.size())
        {
            throw std::out_of_range{"FrameGraph::pass_reads invalid pass index"};
        }

        return passes_[pass_index].reads;
    }

    std::span<const FrameGraphResourceHandle> FrameGraph::pass_writes(std::size_t pass_index)
    {
        if (pass_index >= passes_.size())
        {
            throw std::out_of_range{"FrameGraph::pass_writes invalid pass index"};
        }

        return passes_[pass_index].writes;
    }

    std::string_view FrameGraph::pass_name(std::size_t pass_index) const
    {
        if (pass_index >= passes_.size())
        {
            throw std::out_of_range{"FrameGraph::pass_name invalid pass index"};
        }

        return passes_[pass_index].pass->name();
    }
}
