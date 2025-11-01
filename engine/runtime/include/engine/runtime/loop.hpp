#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace engine::runtime
{
    /// Phase classification for runtime loop stages.
    enum class RuntimeLoopPhase
    {
        Simulation,
        Presentation,
        Diagnostics,
    };

    /// Callback signature executed for each stage.
    using RuntimeLoopStageFunction = std::function<void(double)>;

    /// Declarative description of a runtime loop stage.
    struct RuntimeLoopStage
    {
        std::string name;
        RuntimeLoopPhase phase{RuntimeLoopPhase::Simulation};
        RuntimeLoopStageFunction function{};
        std::vector<std::string> dependencies{};
        bool record_in_execution_report{true};
    };

    /// Compiled runtime loop plan ordered topologically.
    class RuntimeLoopPlan
    {
    public:
        RuntimeLoopPlan() = default;
        explicit RuntimeLoopPlan(std::vector<RuntimeLoopStage> stages);

        [[nodiscard]] const std::vector<RuntimeLoopStage>& stages() const noexcept;

    private:
        std::vector<RuntimeLoopStage> stages_{};
    };

    /// Builder that validates and compiles runtime loop stages.
    class RuntimeLoopBuilder
    {
    public:
        void add_stage(std::string name,
                       RuntimeLoopPhase phase,
                       RuntimeLoopStageFunction function,
                       std::vector<std::string> dependencies = {},
                       bool record_in_execution_report = true);

        [[nodiscard]] RuntimeLoopPlan build() const;

    private:
        std::vector<RuntimeLoopStage> stages_{};
    };
} // namespace engine::runtime
