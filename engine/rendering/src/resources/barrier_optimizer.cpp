#include "engine/rendering/barrier_optimizer.hpp"

#include <utility>

namespace engine::rendering
{
    BarrierOptimizer::OptimizedBarriers BarrierOptimizer::optimize(std::vector<resources::Barrier> barriers) const
    {
        OptimizedBarriers result{};
        if (barriers.empty())
        {
            result.barriers = std::move(barriers);
            return result;
        }

        std::vector<ResourceTransition> states;
        result.barriers.reserve(barriers.size());

        for (auto& barrier : barriers)
        {
            if (!barrier.resource.valid())
            {
                continue;
            }

            ensure_capacity(states, barrier.resource.index);
            auto& state = states[barrier.resource.index];

            if (state.valid && state.stage == barrier.destination_stage &&
                state.access == barrier.destination_access)
            {
                ++result.eliminated_count;
                continue;
            }

            if (is_no_op(barrier))
            {
                ++result.eliminated_count;
                state.valid = true;
                state.stage = barrier.destination_stage;
                state.access = barrier.destination_access;
                continue;
            }

            if (!result.barriers.empty() && can_merge(result.barriers.back(), barrier))
            {
                auto& merged = result.barriers.back();
                merged.destination_stage = barrier.destination_stage;
                merged.destination_access = barrier.destination_access;
                ++result.eliminated_count;
            }
            else
            {
                result.barriers.push_back(barrier);
            }

            state.valid = true;
            state.stage = barrier.destination_stage;
            state.access = barrier.destination_access;
        }

        return result;
    }

    bool BarrierOptimizer::is_no_op(const resources::Barrier& barrier) noexcept
    {
        return barrier.source_stage == barrier.destination_stage &&
            barrier.source_access == barrier.destination_access;
    }

    bool BarrierOptimizer::can_merge(const resources::Barrier& lhs,
                                     const resources::Barrier& rhs) noexcept
    {
        return lhs.resource == rhs.resource &&
            lhs.destination_stage == rhs.source_stage &&
            lhs.destination_access == rhs.source_access;
    }

    void BarrierOptimizer::ensure_capacity(std::vector<ResourceTransition>& states, std::size_t index)
    {
        if (index >= states.size())
        {
            states.resize(index + 1);
        }
    }
}
