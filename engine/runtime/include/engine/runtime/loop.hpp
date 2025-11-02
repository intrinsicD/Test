#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/runtime/errors.hpp"

namespace engine::runtime
{
    /// Hint describing where a runtime loop stage prefers to execute.
    enum class RuntimeLoopThreadAffinity
    {
        MainThread,
        WorkerThread,
        Any,
    };

    /// Phase classification for runtime loop stages.
    enum class RuntimeLoopPhase
    {
        Simulation,
        Presentation,
        Diagnostics,
    };

    /// Number of execution phases supported by the runtime loop.
    [[nodiscard]] constexpr std::size_t runtime_loop_phase_count() noexcept
    {
        return 3U;
    }

    /// Map a runtime loop phase to its zero-based index.
    [[nodiscard]] constexpr std::size_t runtime_loop_phase_index(RuntimeLoopPhase phase) noexcept
    {
        return static_cast<std::size_t>(phase);
    }

    /// Callback signature executed for each stage.
    using RuntimeLoopStageFunction = std::function<void(double)>;

    /// Declarative description of a runtime loop stage.
    struct RuntimeLoopStage
    {
        std::string name;
        RuntimeLoopPhase phase{RuntimeLoopPhase::Simulation};
        RuntimeLoopStageFunction function{};
        std::vector<std::string> dependencies{};
        RuntimeLoopThreadAffinity thread_affinity{RuntimeLoopThreadAffinity::MainThread};
        bool record_in_execution_report{true};
    };

    /// Compiled runtime loop plan ordered topologically.
    class RuntimeLoopPlan
    {
    public:
        RuntimeLoopPlan() = default;
        explicit RuntimeLoopPlan(std::vector<RuntimeLoopStage> stages);

        [[nodiscard]] const std::vector<RuntimeLoopStage>& stages() const noexcept;
        [[nodiscard]] std::vector<const RuntimeLoopStage*>
        stages_for_phase(RuntimeLoopPhase phase) const;

    private:
        std::vector<RuntimeLoopStage> stages_{};
    };

    /// Builder that validates and compiles runtime loop stages.
    class RuntimeLoopBuilder
    {
    public:
        [[nodiscard]] RuntimeValidationResult add_stage(std::string name,
                                                        RuntimeLoopPhase phase,
                                                        RuntimeLoopStageFunction function,
                                                        std::vector<std::string> dependencies = {},
                                                        bool record_in_execution_report = true,
                                                        RuntimeLoopThreadAffinity thread_affinity =
                                                            RuntimeLoopThreadAffinity::MainThread);

        [[nodiscard]] RuntimeResult<RuntimeLoopPlan> build() const;

    private:
        std::vector<RuntimeLoopStage> stages_{};
    };
} // namespace engine::runtime
