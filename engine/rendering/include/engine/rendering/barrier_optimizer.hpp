#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "engine/rendering/resources/synchronization.hpp"

namespace engine::rendering
{
    /// Helper that removes redundant barriers emitted during frame-graph compilation.
    class BarrierOptimizer
    {
    public:
        struct OptimizedBarriers
        {
            std::vector<resources::Barrier> barriers{};
            std::uint32_t eliminated_count{0};
        };

        [[nodiscard]] OptimizedBarriers optimize(std::vector<resources::Barrier> barriers) const;

    private:
        struct ResourceTransition
        {
            resources::PipelineStage stage{resources::PipelineStage::Graphics};
            resources::Access access{resources::Access::None};
            bool valid{false};
        };

        [[nodiscard]] static bool is_no_op(const resources::Barrier& barrier) noexcept;
        [[nodiscard]] static bool can_merge(const resources::Barrier& lhs,
                                            const resources::Barrier& rhs) noexcept;
        static void ensure_capacity(std::vector<ResourceTransition>& states, std::size_t index);
    };
}
