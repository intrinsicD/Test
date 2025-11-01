#include "engine/runtime/loop.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_map>

namespace engine::runtime
{
    RuntimeLoopPlan::RuntimeLoopPlan(std::vector<RuntimeLoopStage> stages)
        : stages_(std::move(stages))
    {
    }

    const std::vector<RuntimeLoopStage>& RuntimeLoopPlan::stages() const noexcept
    {
        return stages_;
    }

    void RuntimeLoopBuilder::add_stage(std::string name,
                                       RuntimeLoopPhase phase,
                                       RuntimeLoopStageFunction function,
                                       std::vector<std::string> dependencies,
                                       bool record_in_execution_report)
    {
        if (name.empty())
        {
            throw std::invalid_argument{"RuntimeLoopBuilder stage name must not be empty"};
        }

        const auto duplicate = std::any_of(
            stages_.begin(),
            stages_.end(),
            [&](const RuntimeLoopStage& stage) { return stage.name == name; });
        if (duplicate)
        {
            throw std::invalid_argument{"RuntimeLoopBuilder cannot register duplicate stage name"};
        }

        RuntimeLoopStage stage{};
        stage.name = std::move(name);
        stage.phase = phase;
        stage.function = std::move(function);
        stage.dependencies = std::move(dependencies);
        stage.record_in_execution_report = record_in_execution_report;
        stages_.push_back(std::move(stage));
    }

    RuntimeLoopPlan RuntimeLoopBuilder::build() const
    {
        if (stages_.empty())
        {
            return RuntimeLoopPlan{};
        }

        std::unordered_map<std::string, std::size_t> name_to_index{};
        name_to_index.reserve(stages_.size());
        for (std::size_t index = 0; index < stages_.size(); ++index)
        {
            name_to_index.emplace(stages_[index].name, index);
        }

        std::vector<std::size_t> indegree(stages_.size(), 0U);
        std::vector<std::vector<std::size_t>> adjacency(stages_.size());
        for (std::size_t index = 0; index < stages_.size(); ++index)
        {
            for (const auto& dependency : stages_[index].dependencies)
            {
                const auto it = name_to_index.find(dependency);
                if (it == name_to_index.end())
                {
                    throw std::invalid_argument("RuntimeLoopBuilder stage '" + stages_[index].name
                                                 + "' depends on unknown stage '" + dependency + "'");
                }
                adjacency[it->second].push_back(index);
                ++indegree[index];
            }
        }

        std::queue<std::size_t> ready{};
        for (std::size_t index = 0; index < stages_.size(); ++index)
        {
            if (indegree[index] == 0U)
            {
                ready.push(index);
            }
        }

        std::vector<std::size_t> order{};
        order.reserve(stages_.size());
        while (!ready.empty())
        {
            const auto node = ready.front();
            ready.pop();
            order.push_back(node);
            for (const auto successor : adjacency[node])
            {
                if (--indegree[successor] == 0U)
                {
                    ready.push(successor);
                }
            }
        }

        if (order.size() != stages_.size())
        {
            throw std::runtime_error{"RuntimeLoopBuilder detected a cycle in stage dependencies"};
        }

        std::vector<RuntimeLoopStage> sorted{};
        sorted.reserve(order.size());
        for (const auto index : order)
        {
            const auto& stage = stages_[index];
            RuntimeLoopStage compiled{};
            compiled.name = stage.name;
            compiled.phase = stage.phase;
            compiled.function = stage.function;
            compiled.dependencies = stage.dependencies;
            compiled.record_in_execution_report = stage.record_in_execution_report;
            sorted.push_back(std::move(compiled));
        }

        return RuntimeLoopPlan{std::move(sorted)};
    }
} // namespace engine::runtime
