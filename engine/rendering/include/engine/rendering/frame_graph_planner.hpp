#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/rendering/frame_graph_node.hpp"
#include "engine/rendering/frame_graph_registry.hpp"

namespace engine::rendering
{
    struct RenderExecutionContext;

    class FrameGraphPlanner
    {
    public:
        struct PlanRequest
        {
            std::vector<std::string> nodes;
            std::vector<ResourceDesc> external_resources;
        };

        struct PlannedResource
        {
            std::string name;
            ResourceDesc descriptor;
            bool external{false};
            bool transient{false};
            std::size_t alias{std::numeric_limits<std::size_t>::max()};
            std::size_t first_use{std::numeric_limits<std::size_t>::max()};
            std::size_t last_use{std::numeric_limits<std::size_t>::max()};
        };

        struct PlannedPass
        {
            std::string id;
            NodeDescriptor descriptor;
            std::unique_ptr<INode> node;
            QueueType queue{QueueType::Graphics};
            std::vector<std::size_t> creates;
            std::vector<std::size_t> reads;
            std::vector<std::size_t> writes;
        };

        class Plan
        {
        public:
            struct ExecutionTelemetry
            {
                std::size_t transient_acquires{0};
                std::size_t transient_releases{0};
                std::size_t submissions{0};
            };

            Plan();
            Plan(const Plan&) = delete;
            Plan& operator=(const Plan&) = delete;
            Plan(Plan&&) noexcept;
            Plan& operator=(Plan&&) noexcept;
            ~Plan();

            [[nodiscard]] const std::vector<PlannedPass>& passes() const noexcept;
            [[nodiscard]] const std::vector<PlannedResource>& resources() const noexcept;
            [[nodiscard]] std::optional<std::size_t> find_resource(std::string_view name) const;
            [[nodiscard]] std::string to_dot() const;
            void execute(RenderExecutionContext& context, ExecutionTelemetry* telemetry = nullptr);

        private:
            friend class FrameGraphPlanner;

            std::vector<PlannedPass> passes_{};
            std::vector<PlannedResource> resources_{};
            std::unordered_map<std::string, std::size_t, TransparentStringHash, std::equal_to<>> resource_lookup_{};
        };

        explicit FrameGraphPlanner(const FrameGraphNodeRegistry& registry) noexcept;

        [[nodiscard]] Plan plan(const PlanRequest& request) const;

    private:
        const FrameGraphNodeRegistry* registry_{nullptr};
    };
}
