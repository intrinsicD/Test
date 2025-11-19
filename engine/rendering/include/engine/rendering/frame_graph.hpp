#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/rendering/barrier_optimizer.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/synchronization.hpp"

namespace engine::rendering
{
    class FrameGraph;
    class CommandEncoder;

    /// Handle identifying resources declared inside the frame-graph.


    struct FrameGraphResourceDescriptor
    {
        std::string name;
        ResourceLifetime lifetime{ResourceLifetime::Transient};
        ResourceFormat format{ResourceFormat::Unknown};
        ResourceDimension dimension{ResourceDimension::Unknown};
        ResourceUsage usage{ResourceUsage::None};
        ResourceState initial_state{ResourceState::Undefined};
        ResourceState final_state{ResourceState::Undefined};
        std::uint32_t width{1};
        std::uint32_t height{1};
        std::uint32_t depth{1};
        std::uint32_t array_layers{1};
        std::uint32_t mip_levels{1};
        ResourceSampleCount sample_count{ResourceSampleCount::Count1};
        std::uint64_t size_bytes{0};
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
        CommandEncoder* encoder{nullptr};

        [[nodiscard]] std::string_view pass_name() const;
        [[nodiscard]] std::span<const FrameGraphResourceHandle> reads() const;
        [[nodiscard]] std::span<const FrameGraphResourceHandle> writes() const;
        [[nodiscard]] FrameGraphResourceInfo describe(FrameGraphResourceHandle handle) const;
        [[nodiscard]] CommandBufferHandle command_buffer_handle() const noexcept;
        [[nodiscard]] QueueType queue_type() const noexcept;
        [[nodiscard]] PassPhase pass_phase() const noexcept;
        [[nodiscard]] ValidationSeverity validation_severity() const noexcept;
        [[nodiscard]] CommandEncoder& command_encoder() const;
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

    struct BarrierStatistics
    {
        std::uint64_t begin_before{0};
        std::uint64_t begin_after{0};
        std::uint64_t end_before{0};
        std::uint64_t end_after{0};

        [[nodiscard]] std::uint64_t begin_eliminated() const noexcept
        {
            return begin_before - begin_after;
        }

        [[nodiscard]] std::uint64_t end_eliminated() const noexcept
        {
            return end_before - end_after;
        }

        [[nodiscard]] std::uint64_t total_before() const noexcept
        {
            return begin_before + end_before;
        }

        [[nodiscard]] std::uint64_t total_after() const noexcept
        {
            return begin_after + end_after;
        }

        [[nodiscard]] std::uint64_t total_eliminated() const noexcept
        {
            return total_before() - total_after();
        }
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

    struct FrameGraphCacheStats
    {
        std::uint64_t hits{0};
        std::uint64_t misses{0};
    };

    /// Frame-graph implementation responsible for scheduling and execution.
    class FrameGraph
    {
    public:
        FrameGraph();

        void reset();

        FrameGraphResourceHandle create_resource(FrameGraphResourceDescriptor descriptor);
        FrameGraphResourceHandle create_resource(std::string name,
                                                 ResourceLifetime lifetime = ResourceLifetime::Transient);

        std::size_t add_pass(std::unique_ptr<RenderPass> pass);

        void compile();
        void execute(RenderExecutionContext& context);

        [[nodiscard]] const std::vector<std::size_t>& execution_order() const noexcept;
        [[nodiscard]] const std::vector<ResourceEvent>& resource_events() const noexcept;
        [[nodiscard]] const BarrierStatistics& barrier_statistics() const noexcept;
        [[nodiscard]] FrameGraphResourceInfo resource_info(FrameGraphResourceHandle handle) const;
        [[nodiscard]] std::span<const FrameGraphResourceHandle> pass_reads(std::size_t pass_index);
        [[nodiscard]] std::span<const FrameGraphResourceHandle> pass_writes(std::size_t pass_index);
        [[nodiscard]] std::string_view pass_name(std::size_t pass_index) const;
        [[nodiscard]] std::string serialize() const;
        [[nodiscard]] FrameGraphCacheStats cache_stats() const noexcept;
        void clear_cache() noexcept;

    private:
        struct CompilationCache
        {
            std::size_t resource_hash{0};
            std::size_t pass_hash{0};
            std::vector<std::size_t> execution_order;
            std::vector<std::vector<resources::Barrier>> pass_begin_barriers;
            std::vector<std::vector<resources::Barrier>> pass_end_barriers;
            std::vector<std::size_t> resource_first_use;
            std::vector<std::size_t> resource_last_use;
        };

        struct ResourceNode
        {
            std::string name;
            ResourceLifetime lifetime{ResourceLifetime::Transient};
            ResourceFormat format{ResourceFormat::Unknown};
            ResourceDimension dimension{ResourceDimension::Unknown};
            ResourceUsage usage{ResourceUsage::None};
            ResourceState initial_state{ResourceState::Undefined};
            ResourceState final_state{ResourceState::Undefined};
            std::uint32_t width{1};
            std::uint32_t height{1};
            std::uint32_t depth{1};
            std::uint32_t array_layers{1};
            std::uint32_t mip_levels{1};
            ResourceSampleCount sample_count{ResourceSampleCount::Count1};
            std::uint64_t size_bytes{0};
            std::size_t writer{std::numeric_limits<std::size_t>::max()};
            std::vector<std::size_t> readers;
            std::vector<std::size_t> reader_dependencies;
            std::vector<std::size_t> writer_history;
            std::vector<std::pair<std::size_t, std::size_t>> reader_to_writer_edges;
            std::vector<std::size_t> readers_since_last_write;
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
        BarrierOptimizer barrier_optimizer_{};
        BarrierStatistics barrier_statistics_{};
        bool compiled_{false};
        std::optional<CompilationCache> compilation_cache_{};
        std::uint64_t cache_hits_{0};
        std::uint64_t cache_misses_{0};

        friend class FrameGraphPassBuilder;
        friend struct FrameGraphPassExecutionContext;

        [[nodiscard]] std::size_t compute_resource_hash() const noexcept;
        [[nodiscard]] std::size_t compute_pass_hash() const noexcept;
        bool try_restore_from_cache(std::size_t resource_hash, std::size_t pass_hash);
        void update_cache(std::size_t resource_hash, std::size_t pass_hash);
    };
}