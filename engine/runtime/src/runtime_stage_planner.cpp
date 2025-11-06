#include "engine/runtime/runtime_loop_plan.hpp"

#include <utility>

namespace engine::runtime
{
    RuntimeValidationResult RuntimeStagePlanner::configure_plan(const RuntimeLoopPlan& plan)
    {
        plan_ = &plan;
        handles_.clear();
        handles_.reserve(plan.stages().size());
        for (std::size_t index = 0; index < plan.stages().size(); ++index)
        {
            const auto& stage = plan.stages()[index];
            RuntimeStageHandle handle{};
            handle.name = stage.name;
            handle.phase = stage.phase;
            handle.thread_affinity = stage.thread_affinity;
            handle.record_in_execution_report = stage.record_in_execution_report;
            handle.index = index;
            handles_.push_back(handle);
        }
        reset_iteration();
        return RuntimeValidationResult{};
    }

    void RuntimeStagePlanner::clear_plan() noexcept
    {
        plan_ = nullptr;
        handles_.clear();
        next_index_ = 0;
    }

    void RuntimeStagePlanner::reset_iteration() noexcept
    {
        next_index_ = 0;
    }

    bool RuntimeStagePlanner::has_plan() const noexcept
    {
        return plan_ != nullptr;
    }

    const RuntimeLoopPlan* RuntimeStagePlanner::plan() const noexcept
    {
        return plan_;
    }

    RuntimeResult<std::optional<RuntimeStageExecution>> RuntimeStagePlanner::next_stage()
    {
        if (plan_ == nullptr)
        {
            return RuntimeResult<std::optional<RuntimeStageExecution>>{
                make_runtime_error(RuntimeError::loop_stage_planner_unconfigured,
                                    "RuntimeStagePlanner requires configure_plan() before iteration")};
        }
        if (next_index_ >= handles_.size())
        {
            return RuntimeResult<std::optional<RuntimeStageExecution>>{std::optional<RuntimeStageExecution>{}};
        }

        if (next_index_ >= plan_->stages().size())
        {
            clear_plan();
            return RuntimeResult<std::optional<RuntimeStageExecution>>{
                make_runtime_error(RuntimeError::loop_stage_planner_invalid_iteration,
                                    "RuntimeStagePlanner encountered an out-of-range stage index")};
        }

        RuntimeStageExecution execution{};
        execution.handle = handles_[next_index_];
        execution.function = plan_->stages()[next_index_].function;
        ++next_index_;
        return RuntimeResult<std::optional<RuntimeStageExecution>>{std::optional<RuntimeStageExecution>{std::move(execution)}};
    }
} // namespace engine::runtime
