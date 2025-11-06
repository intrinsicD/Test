#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/resources/synchronization.hpp"

namespace engine::rendering
{
    /**
     * \brief Resource categories managed by frame-graph planner nodes.
     */
    enum class ResourceKind
    {
        Texture,
        Buffer,
        TopLevelAccelerationStructure,
        AccelerationScratch,
        External,
    };

    /**
     * \brief Declarative description of a resource created by a node.
     */
    struct ResourceDesc
    {
        std::string name;
        ResourceKind kind{ResourceKind::Texture};
        ResourceFormat format{ResourceFormat::Unknown};
        ResourceDimension dimension{ResourceDimension::Texture2D};
        std::uint32_t width{1};
        std::uint32_t height{1};
        std::uint32_t depth{1};
        std::uint32_t array_layers{1};
        std::uint32_t mip_levels{1};
        ResourceSampleCount sample_count{ResourceSampleCount::Count1};
        bool transient{true};
    };

    /**
     * \brief Access declaration for a resource consumed by a node.
     */
    struct ResourceUse
    {
        std::string name;
        resources::PipelineStage stage{resources::PipelineStage::Graphics};
        resources::Access access{resources::Access::Read};
        ResourceState state{ResourceState::Undefined};

        [[nodiscard]] bool is_read_only() const noexcept
        {
            return access == resources::Access::Read;
        }

        [[nodiscard]] bool is_write() const noexcept
        {
            return access == resources::Access::Write;
        }
    };

    /**
     * \brief Declarative signature describing a planner node.
     */
    struct NodeDescriptor
    {
        std::string id;
        std::vector<ResourceDesc> creates;
        std::vector<ResourceUse> reads;
        std::vector<ResourceUse> writes;
        std::vector<std::string> tags;
        QueueType preferred_queue{QueueType::Graphics};

        [[nodiscard]] bool declares_resource(std::string_view resource_name) const noexcept
        {
            return find_created(resource_name) != nullptr || find_read(resource_name) != nullptr ||
                find_write(resource_name) != nullptr;
        }

        [[nodiscard]] const ResourceDesc* find_created(std::string_view resource_name) const noexcept
        {
            const auto it = std::find_if(creates.begin(), creates.end(),
                [&](const ResourceDesc& desc) { return desc.name == resource_name; });
            return it != creates.end() ? std::addressof(*it) : nullptr;
        }

        [[nodiscard]] const ResourceUse* find_read(std::string_view resource_name) const noexcept
        {
            const auto it = std::find_if(reads.begin(), reads.end(),
                [&](const ResourceUse& use) { return use.name == resource_name; });
            return it != reads.end() ? std::addressof(*it) : nullptr;
        }

        [[nodiscard]] const ResourceUse* find_write(std::string_view resource_name) const noexcept
        {
            const auto it = std::find_if(writes.begin(), writes.end(),
                [&](const ResourceUse& use) { return use.name == resource_name; });
            return it != writes.end() ? std::addressof(*it) : nullptr;
        }
    };

    class NodeContext;

    /**
     * \brief Interface implemented by planner nodes to integrate with the frame-graph.
     */
    class INode
    {
    public:
        virtual ~INode() = default;

        [[nodiscard]] virtual const NodeDescriptor& Reflect() const = 0;
        virtual void Compile(NodeContext& context) = 0;
        virtual void Execute(NodeContext& context) = 0;
    };
} // namespace engine::rendering
