#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "engine/runtime/loop.hpp"

namespace engine::runtime
{
    /// Lightweight snapshot of a runtime loop stage for tooling consumption.
    struct RuntimeLoopInspectionStage
    {
        std::string name;
        RuntimeLoopPhase phase{RuntimeLoopPhase::Simulation};
        std::vector<std::string> dependencies{};
        bool record_in_execution_report{true};
    };

    /// Collection of stages that describe the compiled runtime loop plan.
    struct RuntimeLoopInspectionReport
    {
        std::vector<RuntimeLoopInspectionStage> stages{};
    };

    /// Build an inspection report for the provided \p plan.
    [[nodiscard]] RuntimeLoopInspectionReport inspect_loop_plan(const RuntimeLoopPlan& plan);

    /// Serialise \p plan into a JSON document for diagnostics tooling.
    [[nodiscard]] std::string serialize_loop_plan(const RuntimeLoopPlan& plan);

    /// Utility converting loop phases to human-readable strings.
    [[nodiscard]] constexpr std::string_view to_string(RuntimeLoopPhase phase) noexcept
    {
        switch (phase)
        {
        case RuntimeLoopPhase::Simulation:
            return "simulation";
        case RuntimeLoopPhase::Presentation:
            return "presentation";
        case RuntimeLoopPhase::Diagnostics:
            return "diagnostics";
        }
        return "simulation";
    }
}
