#include "engine/rendering/frame_graph.hpp"
#include "engine/core/log.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "engine/rendering/command_encoder.hpp"

namespace engine::rendering
{
    namespace
    {
        template <typename T>
        constexpr void hash_combine(std::size_t& seed, const T& value) noexcept
        {
            seed ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        }

        template <typename Enum>
        constexpr void hash_enum(std::size_t& seed, Enum value) noexcept
        {
            using Underlying = std::underlying_type_t<Enum>;
            hash_combine(seed, static_cast<Underlying>(value));
        }

        class EncoderScope
        {
        public:
            EncoderScope(CommandEncoderProvider& provider,
                         CommandEncoderDescriptor descriptor,
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

        [[nodiscard]] std::string to_string(ResourceLifetime lifetime)
        {
            switch (lifetime)
            {
            case ResourceLifetime::External:
                return "External";
            case ResourceLifetime::Transient:
                return "Transient";
            }
            return "Unknown";
        }

        [[nodiscard]] std::string to_string(ResourceFormat format)
        {
            std::ostringstream stream;
            stream << format;
            return stream.str();
        }

        [[nodiscard]] std::string to_string(ResourceDimension dimension)
        {
            std::ostringstream stream;
            stream << dimension;
            return stream.str();
        }

        [[nodiscard]] std::string to_string(ResourceUsage usage)
        {
            std::ostringstream stream;
            stream << usage;
            return stream.str();
        }

        [[nodiscard]] std::string to_string(ResourceState state)
        {
            std::ostringstream stream;
            stream << state;
            return stream.str();
        }

        [[nodiscard]] std::string to_string(QueueType queue)
        {
            std::ostringstream stream;
            stream << queue;
            return stream.str();
        }

        [[nodiscard]] std::string to_string(PassPhase phase)
        {
            std::ostringstream stream;
            stream << phase;
            return stream.str();
        }

        [[nodiscard]] std::string to_string(ValidationSeverity severity)
        {
            std::ostringstream stream;
            stream << severity;
            return stream.str();
        }

        [[nodiscard]] std::string escape_json(std::string_view value)
        {
            std::string escaped;
            escaped.reserve(value.size());
            for (const char ch : value)
            {
                switch (ch)
                {
                case '\\':
                    escaped.append("\\\\");
                    break;
                case '\"':
                    escaped.append("\\\"");
                    break;
                case '\n':
                    escaped.append("\\n");
                    break;
                case '\r':
                    escaped.append("\\r");
                    break;
                case '\t':
                    escaped.append("\\t");
                    break;
                default:
                    escaped.push_back(ch);
                    break;
                }
            }
            return escaped;
        }

        [[nodiscard]] std::string quoted(std::string_view value)
        {
            std::string result;
            result.reserve(value.size() + 2);
            result.push_back('\"');
            result.append(escape_json(value));
            result.push_back('\"');
            return result;
        }

        [[nodiscard]] bool queue_supports_read(QueueType queue, ResourceUsage usage) noexcept
        {
            switch (queue)
            {
            case QueueType::Graphics:
                return has_flag(usage, ResourceUsage::ShaderRead) ||
                    has_flag(usage, ResourceUsage::TransferSource) ||
                    has_flag(usage, ResourceUsage::ColorAttachment) ||
                    has_flag(usage, ResourceUsage::DepthStencilAttachment) ||
                    has_flag(usage, ResourceUsage::Present);
            case QueueType::Compute:
                return has_flag(usage, ResourceUsage::ShaderRead) ||
                    has_flag(usage, ResourceUsage::TransferSource);
            case QueueType::Transfer:
                return has_flag(usage, ResourceUsage::TransferSource);
            }
            return false;
        }

        [[nodiscard]] bool queue_supports_write(QueueType queue, ResourceUsage usage) noexcept
        {
            switch (queue)
            {
            case QueueType::Graphics:
                return has_flag(usage, ResourceUsage::ShaderWrite) ||
                    has_flag(usage, ResourceUsage::ColorAttachment) ||
                    has_flag(usage, ResourceUsage::DepthStencilAttachment) ||
                    has_flag(usage, ResourceUsage::TransferDestination) ||
                    has_flag(usage, ResourceUsage::Present);
            case QueueType::Compute:
                return has_flag(usage, ResourceUsage::ShaderWrite) ||
                    has_flag(usage, ResourceUsage::TransferDestination);
            case QueueType::Transfer:
                return has_flag(usage, ResourceUsage::TransferDestination);
            }
            return false;
        }
    }

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
            resource.reader_dependencies.push_back(resource.writer);
        }
        if (!resource.writer_history.empty())
        {
            if (std::find(resource.readers_since_last_write.begin(), resource.readers_since_last_write.end(), pass_index_)
                == resource.readers_since_last_write.end())
            {
                resource.readers_since_last_write.push_back(pass_index_);
            }
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
        const bool first_writer = resource.writer_history.empty();
        const bool is_new_writer = first_writer || resource.writer_history.back() != pass_index_;
        if (is_new_writer)
        {
            resource.writer_history.push_back(pass_index_);
        }

        if (first_writer)
        {
            for (auto& dependency : resource.reader_dependencies)
            {
                if (dependency == std::numeric_limits<std::size_t>::max())
                {
                    dependency = pass_index_;
                }
            }
        }

        for (const auto reader : resource.readers_since_last_write)
        {
            if (reader != pass_index_)
            {
                resource.reader_to_writer_edges.emplace_back(reader, pass_index_);
            }
        }
        resource.readers_since_last_write.clear();
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

    PassPhase FrameGraphPassExecutionContext::pass_phase() const noexcept
    {
        return graph.passes_[pass_index].pass->phase();
    }

    ValidationSeverity FrameGraphPassExecutionContext::validation_severity() const noexcept
    {
        return graph.passes_[pass_index].pass->validation_severity();
    }

    CommandEncoder& FrameGraphPassExecutionContext::command_encoder() const
    {
        if (encoder == nullptr)
        {
            throw std::logic_error{
                "FrameGraphPassExecutionContext::command_encoder accessed without an active encoder"
            };
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
        if (descriptor.name.empty())
        {
            throw std::invalid_argument{"FrameGraph::create_resource requires a non-empty resource name"};
        }

        const auto duplicate = std::find_if(resources_.begin(), resources_.end(),
                                            [&](const ResourceNode& resource)
                                            {
                                                return resource.name == descriptor.name;
                                            });

        if (duplicate != resources_.end())
        {
            throw std::invalid_argument{
                "FrameGraph::create_resource received duplicate resource name: '" +
                descriptor.name + "'"
            };
        }

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
        node.width = descriptor.width;
        node.height = descriptor.height;
        node.depth = descriptor.depth;
        node.array_layers = descriptor.array_layers;
        node.mip_levels = descriptor.mip_levels;
        node.sample_count = descriptor.sample_count;
        node.size_bytes = descriptor.size_bytes;
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
            if (resource.dimension == ResourceDimension::Buffer)
            {
                if (resource.size_bytes == 0)
                {
                    throw std::logic_error("FrameGraph buffer resource '" + resource.name + "' missing size metadata");
                }
            }
            else
            {
                if (resource.width == 0 || resource.height == 0 || resource.depth == 0)
                {
                    throw std::logic_error("FrameGraph texture resource '" + resource.name +
                        "' missing extent metadata");
                }
                if (resource.array_layers == 0)
                {
                    throw std::logic_error("FrameGraph texture resource '" + resource.name +
                        "' missing array layer metadata");
                }
                if (resource.mip_levels == 0)
                {
                    throw std::logic_error("FrameGraph texture resource '" + resource.name +
                        "' missing mip level metadata");
                }
            }
        }

        for (std::size_t pass_index = 0; pass_index < passes_.size(); ++pass_index)
        {
            const auto queue = passes_[pass_index].pass->queue();
            const auto& pass = passes_[pass_index];

            auto validate_access = [&](FrameGraphResourceHandle handle, bool is_write)
            {
                if (!handle.valid() || handle.index >= resources_.size())
                {
                    throw std::logic_error{"FrameGraph pass references invalid resource handle"};
                }

                const auto& resource = resources_[handle.index];
                const auto usage = resource.usage;
                const bool supported = is_write
                                           ? queue_supports_write(queue, usage)
                                           : queue_supports_read(queue, usage);

                if (!supported)
                {
                    std::ostringstream message;
                    message << "FrameGraph pass '" << pass.pass->name() << "' on queue "
                        << to_string(queue) << (is_write
                                                    ? " cannot write resource '"
                                                    : " cannot read resource '")
                        << resource.name << "' with usage '" << to_string(usage) << "'";
                    throw std::logic_error(message.str());
                }
            };

            for (const auto handle : pass.reads)
            {
                validate_access(handle, false);
            }
            for (const auto handle : pass.writes)
            {
                validate_access(handle, true);
            }
        }

        const auto resource_hash = compute_resource_hash();
        const auto pass_hash = compute_pass_hash();
        if (try_restore_from_cache(resource_hash, pass_hash))
        {
            compiled_ = true;
            return;
        }

        ++cache_misses_;

        std::vector<std::vector<std::size_t>> adjacency(passes_.size());
        std::vector<std::size_t> indegree(passes_.size(), 0);

        for (std::size_t resource_index = 0; resource_index < resources_.size(); ++resource_index)
        {
            auto& resource = resources_[resource_index];
            for (std::size_t writer_index = 1; writer_index < resource.writer_history.size(); ++writer_index)
            {
                const auto predecessor = resource.writer_history[writer_index - 1];
                const auto successor = resource.writer_history[writer_index];
                adjacency[predecessor].push_back(successor);
            }

            for (std::size_t reader_index = 0; reader_index < resource.readers.size(); ++reader_index)
            {
                const auto reader = resource.readers[reader_index];
                const auto dependency = reader_index < resource.reader_dependencies.size()
                    ? resource.reader_dependencies[reader_index]
                    : resource.writer;
                if (dependency != std::numeric_limits<std::size_t>::max())
                {
                    adjacency[dependency].push_back(reader);
                }
            }

            for (const auto& edge : resource.reader_to_writer_edges)
            {
                adjacency[edge.first].push_back(edge.second);
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
                               resources::Access destination_access)
        {
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

            auto update_use = [&](FrameGraphResourceHandle handle)
            {
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
                    writer == std::numeric_limits<std::size_t>::max()
                        ? resources::Access::Read
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
        update_cache(resource_hash, pass_hash);
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
        context.scheduler.begin_frame();

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

            EncoderScope encoder_scope{context.encoders, encoder_descriptor, std::move(encoder)};

            auto signal_acquire = [&](FrameGraphResourceHandle handle)
            {
                auto& resource = resources_[handle.index];
                if (resource.lifetime == ResourceLifetime::Transient &&
                    resource.first_use == order_index && !alive[handle.index])
                {
                    alive[handle.index] = true;
                    resource_events_.push_back(ResourceEvent{
                        ResourceEvent::Type::Acquire, resource.name,
                        std::string(pass.pass->name())
                    });
                    context.device_resources.on_transient_acquire(handle, resource_info(handle));
                }
            };

            auto signal_release = [&](FrameGraphResourceHandle handle)
            {
                auto& resource = resources_[handle.index];
                if (resource.lifetime == ResourceLifetime::Transient &&
                    resource.last_use == order_index && alive[handle.index])
                {
                    alive[handle.index] = false;
                    resource_events_.push_back(ResourceEvent{
                        ResourceEvent::Type::Release, resource.name,
                        std::string(pass.pass->name())
                    });
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
            pass_context.encoder = encoder_scope.get();

            ENGINE_INFO("FrameGraph: Executing pass '{}' (reads: {}, writes: {})",
                       pass.pass->name(), pass.reads.size(), pass.writes.size());

            // Execute the render pass logic
            pass.pass->execute(pass_context);

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

            encoder_scope.reset();

            const auto stage_for_queue = [](QueueType queue_type)
            {
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
            timeline_value = submission_value;
        }

        context.scheduler.end_frame();
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
        return FrameGraphResourceInfo{
            resource.name,
            resource.lifetime,
            resource.format,
            resource.dimension,
            resource.usage,
            resource.initial_state,
            resource.final_state,
            resource.width,
            resource.height,
            resource.depth,
            resource.array_layers,
            resource.mip_levels,
            resource.sample_count,
            resource.size_bytes
        };
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

    std::string FrameGraph::serialize() const
    {
        if (!compiled_)
        {
            throw std::logic_error{"FrameGraph::serialize requires a compiled graph"};
        }

        std::ostringstream stream;
        stream << "{\n";

        stream << "  \"resources\": [\n";
        for (std::size_t index = 0; index < resources_.size(); ++index)
        {
            const auto& resource = resources_[index];
            stream << "    {\n";
            stream << "      \"name\": " << quoted(resource.name) << ",\n";
            stream << "      \"lifetime\": " << quoted(to_string(resource.lifetime)) << ",\n";
            stream << "      \"format\": " << quoted(to_string(resource.format)) << ",\n";
            stream << "      \"dimension\": " << quoted(to_string(resource.dimension)) << ",\n";
            stream << "      \"width\": " << resource.width << ",\n";
            stream << "      \"height\": " << resource.height << ",\n";
            stream << "      \"depth\": " << resource.depth << ",\n";
            stream << "      \"array_layers\": " << resource.array_layers << ",\n";
            stream << "      \"mip_levels\": " << resource.mip_levels << ",\n";
            stream << "      \"sample_count\": " << static_cast<std::uint32_t>(resource.sample_count) << ",\n";
            stream << "      \"size_bytes\": " << resource.size_bytes << ",\n";
            stream << "      \"usage\": " << quoted(to_string(resource.usage)) << ",\n";
            stream << "      \"initial_state\": " << quoted(to_string(resource.initial_state)) << ",\n";
            stream << "      \"final_state\": " << quoted(to_string(resource.final_state)) << "\n";
            stream << "    }";
            if (index + 1 < resources_.size())
            {
                stream << ',';
            }
            stream << "\n";
        }
        stream << "  ],\n";

        stream << "  \"passes\": [\n";
        for (std::size_t index = 0; index < passes_.size(); ++index)
        {
            const auto& pass = passes_[index];
            stream << "    {\n";
            stream << "      \"name\": " << quoted(std::string(pass.pass->name())) << ",\n";
            stream << "      \"queue\": " << quoted(to_string(pass.pass->queue())) << ",\n";
            stream << "      \"phase\": " << quoted(to_string(pass.pass->phase())) << ",\n";
            stream << "      \"validation\": " << quoted(to_string(pass.pass->validation_severity())) << ",\n";
            stream << "      \"reads\": [";
            for (std::size_t read_index = 0; read_index < pass.reads.size(); ++read_index)
            {
                const auto handle = pass.reads[read_index];
                const auto& resource = resources_.at(handle.index);
                if (read_index > 0)
                {
                    stream << ", ";
                }
                stream << quoted(resource.name);
            }
            stream << "],\n";
            stream << "      \"writes\": [";
            for (std::size_t write_index = 0; write_index < pass.writes.size(); ++write_index)
            {
                const auto handle = pass.writes[write_index];
                const auto& resource = resources_.at(handle.index);
                if (write_index > 0)
                {
                    stream << ", ";
                }
                stream << quoted(resource.name);
            }
            stream << "]\n";
            stream << "    }";
            if (index + 1 < passes_.size())
            {
                stream << ',';
            }
            stream << "\n";
        }
        stream << "  ],\n";

        stream << "  \"execution_order\": [";
        for (std::size_t index = 0; index < execution_order_.size(); ++index)
        {
            if (index > 0)
            {
                stream << ", ";
            }
            stream << quoted(std::string(pass_name(execution_order_[index])));
        }
        stream << "]\n";

        stream << "}\n";
        return stream.str();
    }

    FrameGraphCacheStats FrameGraph::cache_stats() const noexcept
    {
        return FrameGraphCacheStats{cache_hits_, cache_misses_};
    }

    void FrameGraph::clear_cache() noexcept
    {
        compilation_cache_.reset();
        cache_hits_ = 0;
        cache_misses_ = 0;
    }

    std::size_t FrameGraph::compute_resource_hash() const noexcept
    {
        std::size_t seed = resources_.size();
        for (const auto& resource : resources_)
        {
            hash_combine(seed, std::string_view{resource.name});
            hash_enum(seed, resource.lifetime);
            hash_enum(seed, resource.format);
            hash_enum(seed, resource.dimension);
            hash_enum(seed, resource.usage);
            hash_enum(seed, resource.initial_state);
            hash_enum(seed, resource.final_state);
            hash_combine(seed, resource.width);
            hash_combine(seed, resource.height);
            hash_combine(seed, resource.depth);
            hash_combine(seed, resource.array_layers);
            hash_combine(seed, resource.mip_levels);
            hash_enum(seed, resource.sample_count);
            hash_combine(seed, resource.size_bytes);
        }
        return seed;
    }

    std::size_t FrameGraph::compute_pass_hash() const noexcept
    {
        std::size_t seed = passes_.size();
        for (const auto& pass : passes_)
        {
            hash_combine(seed, std::string_view{pass.pass->name()});
            hash_enum(seed, pass.pass->queue());
            hash_enum(seed, pass.pass->phase());
            hash_enum(seed, pass.pass->validation_severity());

            const auto hash_handles = [&](const std::vector<FrameGraphResourceHandle>& handles)
            {
                hash_combine(seed, handles.size());
                for (const auto handle : handles)
                {
                    hash_combine(seed, handle.index);
                    if (handle.index < resources_.size())
                    {
                        hash_combine(seed, std::string_view{resources_[handle.index].name});
                    }
                }
            };

            hash_handles(pass.reads);
            hash_handles(pass.writes);
        }

        return seed;
    }

    bool FrameGraph::try_restore_from_cache(std::size_t resource_hash, std::size_t pass_hash)
    {
        if (!compilation_cache_)
        {
            return false;
        }

        const auto& cache = *compilation_cache_;
        if (cache.resource_hash != resource_hash || cache.pass_hash != pass_hash)
        {
            return false;
        }

        if (cache.execution_order.size() != passes_.size() ||
            cache.pass_begin_barriers.size() != passes_.size() ||
            cache.pass_end_barriers.size() != passes_.size() ||
            cache.resource_first_use.size() != resources_.size() ||
            cache.resource_last_use.size() != resources_.size())
        {
            return false;
        }

        execution_order_ = cache.execution_order;
        pass_begin_barriers_ = cache.pass_begin_barriers;
        pass_end_barriers_ = cache.pass_end_barriers;
        for (std::size_t index = 0; index < resources_.size(); ++index)
        {
            resources_[index].first_use = cache.resource_first_use[index];
            resources_[index].last_use = cache.resource_last_use[index];
        }

        ++cache_hits_;
        return true;
    }

    void FrameGraph::update_cache(std::size_t resource_hash, std::size_t pass_hash)
    {
        if (!compilation_cache_)
        {
            compilation_cache_.emplace();
        }

        auto& cache = *compilation_cache_;
        cache.resource_hash = resource_hash;
        cache.pass_hash = pass_hash;
        cache.execution_order = execution_order_;
        cache.pass_begin_barriers = pass_begin_barriers_;
        cache.pass_end_barriers = pass_end_barriers_;
        cache.resource_first_use.resize(resources_.size());
        cache.resource_last_use.resize(resources_.size());
        for (std::size_t index = 0; index < resources_.size(); ++index)
        {
            cache.resource_first_use[index] = resources_[index].first_use;
            cache.resource_last_use[index] = resources_[index].last_use;
        }
    }
}