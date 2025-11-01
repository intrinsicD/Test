#include "engine/runtime/loop_inspector.hpp"

#include <array>
#include <sstream>
#include <string_view>

namespace
{
    std::string escape_json_string(std::string_view value)
    {
        std::string escaped{};
        escaped.reserve(value.size());
        constexpr std::array<char, 16> hex_digits{
            {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}};
        for (const unsigned char character : value)
        {
            switch (character)
            {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (character <= 0x1F)
                {
                    escaped += "\\u00";
                    escaped.push_back(hex_digits[(character >> 4U) & 0x0FU]);
                    escaped.push_back(hex_digits[character & 0x0FU]);
                }
                else
                {
                    escaped.push_back(static_cast<char>(character));
                }
                break;
            }
        }
        return escaped;
    }
}

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
            stream << "      \"name\": \"" << escape_json_string(stage.name) << "\",\n";
            stream << "      \"phase\": \"" << to_string(stage.phase) << "\",\n";
            stream << "      \"record_in_execution_report\": "
                   << (stage.record_in_execution_report ? "true" : "false") << ",\n";
            stream << "      \"dependencies\": [";
            for (std::size_t dependency_index = 0; dependency_index < stage.dependencies.size();
                 ++dependency_index)
            {
                stream << '\"' << escape_json_string(stage.dependencies[dependency_index]) << '\"';
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
