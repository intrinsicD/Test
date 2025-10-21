#include "engine/compute/dependency_analysis.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace engine::compute {
namespace {
    enum class VisitState : std::uint8_t {
        Unvisited,
        Visiting,
        Visited,
    };

    void append_cycle_path(std::vector<kernel_id>::const_iterator begin,
                           std::vector<kernel_id>::const_iterator end,
                           std::vector<kernel_id>& destination)
    {
        destination.assign(begin, end);
        if (!destination.empty())
        {
            destination.push_back(*begin);
        }
    }

    void depth_first_search(const DependencyGraph& graph,
                            kernel_id node,
                            std::vector<VisitState>& state,
                            std::vector<kernel_id>& stack,
                            CycleDetectionResult& result)
    {
        if (result.has_cycle)
        {
            return;
        }

        state[node] = VisitState::Visiting;
        stack.push_back(node);

        const auto& metadata = graph.nodes[node];
        for (const auto dependency : metadata.dependencies)
        {
            if (dependency >= graph.nodes.size())
            {
                continue;
            }

            const auto dependency_state = state[dependency];
            if (dependency_state == VisitState::Unvisited)
            {
                depth_first_search(graph, dependency, state, stack, result);
                if (result.has_cycle)
                {
                    return;
                }
            }
            else if (dependency_state == VisitState::Visiting)
            {
                result.has_cycle = true;
                const auto it = std::find(stack.begin(), stack.end(), dependency);
                if (it != stack.end())
                {
                    append_cycle_path(it, stack.end(), result.cycle);
                }
                else
                {
                    result.cycle = {dependency, dependency};
                }
                return;
            }
        }

        stack.pop_back();
        state[node] = VisitState::Visited;
    }
}  // namespace

CycleDetectionResult detect_cycles(const DependencyGraph& graph) noexcept
{
    CycleDetectionResult result{};
    const auto node_count = graph.nodes.size();
    std::vector<VisitState> state(node_count, VisitState::Unvisited);
    std::vector<kernel_id> stack;
    stack.reserve(node_count);

    for (kernel_id node = 0; node < node_count; ++node)
    {
        if (state[node] == VisitState::Unvisited)
        {
            depth_first_search(graph, node, state, stack, result);
            if (result.has_cycle)
            {
                break;
            }
        }
    }

    return result;
}

}  // namespace engine::compute
