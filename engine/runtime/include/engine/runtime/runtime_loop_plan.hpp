#pragma once

#include "engine/runtime/errors.hpp"
#include "engine/runtime/loop.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace engine::runtime
{
    /// Lightweight metadata describing a compiled runtime stage.
    struct RuntimeStageHandle
    {
        std::string_view name{};
        RuntimeLoopPhase phase{RuntimeLoopPhase::Simulation};
        RuntimeLoopThreadAffinity thread_affinity{RuntimeLoopThreadAffinity::MainThread};
        bool record_in_execution_report{true};
        std::size_t index{0};
    };

    /// Execution payload returned by the stage planner.
    struct RuntimeStageExecution
    {
        RuntimeStageHandle handle{};
        RuntimeLoopStageFunction function{};
    };

    /// Iterates a compiled runtime loop plan and exposes per-stage metadata.
    class RuntimeStagePlanner
    {
    public:
        RuntimeStagePlanner() = default;

        [[nodiscard]] RuntimeValidationResult configure_plan(const RuntimeLoopPlan& plan);
        void clear_plan() noexcept;
        void reset_iteration() noexcept;

        [[nodiscard]] bool has_plan() const noexcept;
        [[nodiscard]] const RuntimeLoopPlan* plan() const noexcept;

        [[nodiscard]] RuntimeResult<std::optional<RuntimeStageExecution>> next_stage();

    private:
        const RuntimeLoopPlan* plan_{nullptr};
        std::vector<RuntimeStageHandle> handles_{};
        std::size_t next_index_{0};
    };
} // namespace engine::runtime
