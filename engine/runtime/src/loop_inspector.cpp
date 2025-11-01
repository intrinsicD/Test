#include "engine/runtime/loop_inspector.hpp"

#include <sstream>

namespace engine::runtime
{
    RuntimeLoopInspectionReport inspect_loop_plan(const RuntimeLoopPlan& plan)
    {
        RuntimeLoopInspectionReport report{};
        report.stages.reserve(plan.stages().size());
        for (const auto& stage : plan.stages())
        {
            RuntimeLoopInspectionStage inspected{};
            inspected.name = stage.name;
            inspected.phase = stage.phase;
            inspected.dependencies = stage.dependencies;
            inspected.record_in_execution_report = stage.record_in_execution_report;
            report.stages.push_back(std::move(inspected));
        }
        return report;
    }

    std::string serialize_loop_plan(const RuntimeLoopPlan& plan)
    {
        const auto report = inspect_loop_plan(plan);
        std::ostringstream stream{};
        stream << "{\n";
        stream << "  \"stages\": [\n";
        for (std::size_t index = 0; index < report.stages.size(); ++index)
        {
            const auto& stage = report.stages[index];
            stream << "    {\n";
            stream << "      \"name\": \"" << stage.name << "\",\n";
            stream << "      \"phase\": \"" << to_string(stage.phase) << "\",\n";
            stream << "      \"record_in_execution_report\": "
                   << (stage.record_in_execution_report ? "true" : "false") << ",\n";
            stream << "      \"dependencies\": [";
            for (std::size_t dependency_index = 0; dependency_index < stage.dependencies.size();
                 ++dependency_index)
            {
                stream << '\"' << stage.dependencies[dependency_index] << '\"';
                if (dependency_index + 1 < stage.dependencies.size())
                {
                    stream << ", ";
                }
            }
            stream << "]\n";
            stream << "    }";
            if (index + 1 < report.stages.size())
            {
                stream << ',';
            }
            stream << "\n";
        }
        stream << "  ]\n";
        stream << "}\n";
        return stream.str();
    }
}
