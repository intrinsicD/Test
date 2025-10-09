#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "../../rendering/resources/synchronization.hpp"

namespace engine::rendering
{
    class FrameGraph;

    /// Handle identifying resources declared inside the frame-graph.


    /// Lifetime category of a frame-graph resource.
    enum class ResourceLifetime
    {
        External,
        Transient,
    };

    /// Immutable descriptor exposed to passes when querying resource metadata.
    struct FrameGraphResourceInfo
    {
        std::string_view name;
        ResourceLifetime lifetime{ResourceLifetime::Transient};
    };

    class FrameGraphPassBuilder
    {
    public:
        FrameGraphPassBuilder(FrameGraph& graph, std::size_t pass_index);

        FrameGraphResourceHandle read(FrameGraphResourceHandle handle);
        FrameGraphResourceHandle write(FrameGraphResourceHandle handle);

    private:
        FrameGraph& graph_;
        std::size_t pass_index_;
    };

    struct FrameGraphPassExecutionContext
    {
        RenderExecutionContext& render;
        FrameGraph& graph;
        std::size_t pass_index{std::numeric_limits<std::size_t>::max()};
        CommandBufferHandle command_buffer{};
        QueueType queue{QueueType::Graphics};

        [[nodiscard]] std::string_view pass_name() const;
        [[nodiscard]] std::span<const FrameGraphResourceHandle> reads() const;
        [[nodiscard]] std::span<const FrameGraphResourceHandle> writes() const;
        [[nodiscard]] FrameGraphResourceInfo describe(FrameGraphResourceHandle handle) const;
        [[nodiscard]] CommandBufferHandle command_buffer_handle() const noexcept;
        [[nodiscard]] QueueType queue_type() const noexcept;
    };

    /// Event emitted whenever the lifetime of a transient resource changes.
    struct ResourceEvent
    {
        enum class Type
        {
            Acquire,
            Release,
        };

        Type type{Type::Acquire};
        std::string resource_name;
        std::string pass_name;
    };

    inline std::ostream& operator<<(std::ostream& os, ResourceEvent::Type type)
    {
        switch (type)
        {
        case ResourceEvent::Type::Acquire:
            return os << "Acquire";
        case ResourceEvent::Type::Release:
            return os << "Release";
        }
        return os;
    }

    /// Frame-graph implementation responsible for scheduling and execution.
    class FrameGraph
    {
    public:
        FrameGraph();

        void reset();

        FrameGraphResourceHandle create_resource(std::string name,
                                                 ResourceLifetime lifetime = ResourceLifetime::Transient);

        std::size_t add_pass(std::unique_ptr<RenderPass> pass);

        void compile();
        void execute(RenderExecutionContext& context);

        [[nodiscard]] const std::vector<std::size_t>& execution_order() const noexcept;
        [[nodiscard]] const std::vector<ResourceEvent>& resource_events() const noexcept;
        [[nodiscard]] FrameGraphResourceInfo resource_info(FrameGraphResourceHandle handle) const;
        [[nodiscard]] std::span<const FrameGraphResourceHandle> pass_reads(std::size_t pass_index);
        [[nodiscard]] std::span<const FrameGraphResourceHandle> pass_writes(std::size_t pass_index);
        [[nodiscard]] std::string_view pass_name(std::size_t pass_index) const;

    private:
        struct ResourceNode
        {
            std::string name;
            ResourceLifetime lifetime{ResourceLifetime::Transient};
            std::size_t writer{std::numeric_limits<std::size_t>::max()};
            std::vector<std::size_t> readers;
            std::size_t first_use{std::numeric_limits<std::size_t>::max()};
            std::size_t last_use{std::numeric_limits<std::size_t>::max()};
        };

        struct PassNode
        {
            std::unique_ptr<RenderPass> pass;
            std::vector<FrameGraphResourceHandle> reads;
            std::vector<FrameGraphResourceHandle> writes;
        };

        std::vector<ResourceNode> resources_;
        std::vector<PassNode> passes_;
        std::vector<std::size_t> execution_order_;
        std::vector<ResourceEvent> resource_events_;
        std::vector<std::vector<resources::Barrier>> pass_begin_barriers_;
        std::vector<std::vector<resources::Barrier>> pass_end_barriers_;
        bool compiled_{false};

        friend class FrameGraphPassBuilder;
        friend struct FrameGraphPassExecutionContext;
    };
}
