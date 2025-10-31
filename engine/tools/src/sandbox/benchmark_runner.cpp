#include "engine/tools/sandbox/benchmark_runner.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace engine::tools::sandbox
{
    namespace
    {
        std::string format_timestamp_component(std::time_t timestamp)
        {
            std::tm time_info{};
#if defined(_WIN32)
            localtime_s(&time_info, &timestamp);
    #else
            localtime_r(&timestamp, &time_info);
    #endif
            std::ostringstream stream;
            stream << std::put_time(&time_info, "%Y%m%d_%H%M%S");
            return stream.str();
        }
    } // namespace

    PrototypeHarnessBenchmarkRunner::PrototypeHarnessBenchmarkRunner(std::vector<std::string> command_prefix,
                                                                     std::filesystem::path summary_directory)
        : command_prefix_(std::move(command_prefix)), summary_directory_(std::move(summary_directory))
    {
    }

    SandboxBenchmarkResult PrototypeHarnessBenchmarkRunner::run(const SandboxPreferences& preferences) const
    {
        if (command_prefix_.empty())
        {
            return SandboxBenchmarkResult{
                .success = false,
                .headline = "Benchmark command not configured",
                .details = "Provide a command prefix for the prototyping harness before running benchmarks.",
            };
        }

        std::error_code directory_error{};
        std::filesystem::create_directories(summary_directory_, directory_error);
        if (directory_error)
        {
            SandboxBenchmarkResult result{};
            result.success = false;
            result.headline = "Failed to create benchmark summary directory";
            result.details = directory_error.message();
            return result;
        }

        const auto summary_path = make_summary_path();

        std::vector<std::string> command = command_prefix_;
        command.emplace_back("--frames");
        command.emplace_back(std::to_string(std::max(preferences.benchmark_frames, 1)));
        command.emplace_back("--dt");
        const float safe_timestep = std::max(preferences.benchmark_timestep, std::numeric_limits<float>::epsilon());
        command.emplace_back(format_timestep(safe_timestep));
        command.emplace_back("--summary-json");
        command.emplace_back(summary_path.string());
        if (!preferences.selected_dataset.empty())
        {
            command.emplace_back("--dataset");
            command.emplace_back(preferences.selected_dataset);
        }
        if (!preferences.selected_algorithm_variant.empty())
        {
            command.emplace_back("--runtime-profile");
            command.emplace_back(preferences.selected_algorithm_variant);
        }
        if (!preferences.selected_preset.empty())
        {
            command.emplace_back("--rendering-preset");
            command.emplace_back(preferences.selected_preset);
        }
        if (!preferences.shading_mode.empty())
        {
            command.emplace_back("--shading-mode");
            command.emplace_back(preferences.shading_mode);
        }
        for (const auto& [key, value] : preferences.overlays)
        {
            command.emplace_back("--overlay");
            command.emplace_back(key + '=' + std::string{value ? "1" : "0"});
        }

        const int exit_code = execute_command(command);
        if (exit_code != 0)
        {
            SandboxBenchmarkResult result{};
            result.success = false;
            std::ostringstream headline;
            headline << "Benchmark failed (exit code " << exit_code << ')';
            result.headline = headline.str();

            std::ostringstream details;
            details << "Command:";
            for (const auto& token : command)
            {
                details << ' ' << token;
            }
            if (std::filesystem::exists(summary_path))
            {
                details << "\nPartial summary saved to " << summary_path.string();
            }
            else
            {
                details << "\nNo summary was produced.";
            }
            result.details = details.str();
            return result;
        }

        if (auto parsed = parse_summary(summary_path))
        {
            return *parsed;
        }

        SandboxBenchmarkResult fallback{};
        fallback.success = true;
        fallback.headline = "Benchmark succeeded";
        fallback.details = "Execution summary saved to " + summary_path.string();
        return fallback;
    }

    std::filesystem::path PrototypeHarnessBenchmarkRunner::make_summary_path() const
    {
        const auto now = std::chrono::system_clock::now();
        const auto timestamp = std::chrono::system_clock::to_time_t(now);
        const auto unique_suffix = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

        std::ostringstream filename;
        filename << "benchmark_summary_" << format_timestamp_component(timestamp) << '_' << unique_suffix << ".json";
        return summary_directory_ / filename.str();
    }

    std::string PrototypeHarnessBenchmarkRunner::format_timestep(float timestep)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(6) << timestep;
        return stream.str();
    }

    std::string PrototypeHarnessBenchmarkRunner::quote_argument(const std::string& argument)
    {
        std::string quoted;
        quoted.reserve(argument.size() + 2);
        quoted.push_back('"');
        for (char ch : argument)
        {
            if (ch == '"' || ch == '\\')
            {
                quoted.push_back('\\');
            }
            quoted.push_back(ch);
        }
        quoted.push_back('"');
        return quoted;
    }

    int PrototypeHarnessBenchmarkRunner::execute_command(const std::vector<std::string>& command)
    {
        std::ostringstream builder;
        bool first = true;
        for (const auto& token : command)
        {
            if (!first)
            {
                builder << ' ';
            }
            builder << quote_argument(token);
            first = false;
        }

        const std::string command_line = builder.str();
        const int result = std::system(command_line.c_str());
        if (result == -1)
        {
            return -1;
        }
    #ifdef _WIN32
        return result;
    #else
        if (WIFEXITED(result))
        {
            return WEXITSTATUS(result);
        }
        if (WIFSIGNALED(result))
        {
            return 128 + WTERMSIG(result);
        }
        return result;
    #endif
    }

    namespace
    {
        [[nodiscard]] std::optional<std::string> extract_string_field(const std::string& content,
                                                                      std::string_view key)
        {
            const std::string pattern = '"' + std::string{key} + '"';
            const auto key_pos = content.find(pattern);
            if (key_pos == std::string::npos)
            {
                return std::nullopt;
            }

            const auto colon = content.find(':', key_pos + pattern.size());
            if (colon == std::string::npos)
            {
                return std::nullopt;
            }

            const auto value_begin = content.find_first_not_of(" \t\r\n", colon + 1);
            if (value_begin == std::string::npos || content[value_begin] != '"')
            {
                return std::nullopt;
            }

            auto cursor = value_begin + 1;
            std::string value;
            value.reserve(32);
            while (cursor < content.size())
            {
                const char ch = content[cursor];
                if (ch == '\\')
                {
                    if (cursor + 1 < content.size())
                    {
                        value.push_back(content[cursor + 1]);
                        cursor += 2;
                        continue;
                    }
                    break;
                }
                if (ch == '"')
                {
                    return value;
                }
                value.push_back(ch);
                ++cursor;
            }
            return std::nullopt;
        }

        template <typename T>
        [[nodiscard]] std::optional<T> extract_numeric_field(const std::string& content, std::string_view key)
        {
            const std::string pattern = '"' + std::string{key} + '"';
            const auto key_pos = content.find(pattern);
            if (key_pos == std::string::npos)
            {
                return std::nullopt;
            }

            const auto colon = content.find(':', key_pos + pattern.size());
            if (colon == std::string::npos)
            {
                return std::nullopt;
            }

            const auto value_begin = content.find_first_not_of(" \t\r\n", colon + 1);
            if (value_begin == std::string::npos)
            {
                return std::nullopt;
            }

            auto value_end = value_begin;
            while (value_end < content.size())
            {
                const char ch = content[value_end];
                if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '.' || ch == '-' || ch == '+' || ch == 'e'
                    || ch == 'E')
                {
                    ++value_end;
                    continue;
                }
                break;
            }

            if (value_end == value_begin)
            {
                return std::nullopt;
            }

            const std::string_view token(content.data() + value_begin, static_cast<std::size_t>(value_end - value_begin));
            T parsed{};
            const auto result = std::from_chars(token.data(), token.data() + token.size(), parsed);
            if (result.ec != std::errc{})
            {
                return std::nullopt;
            }
            return parsed;
        }
    } // namespace

    std::optional<SandboxBenchmarkResult> PrototypeHarnessBenchmarkRunner::parse_summary(
        const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
        {
            return std::nullopt;
        }

        const std::string content{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
        if (content.empty())
        {
            return std::nullopt;
        }

        const auto scenario = extract_string_field(content, "scenario");
        const auto dataset = extract_string_field(content, "dataset");
        const auto preset = extract_string_field(content, "rendering_preset");
        const auto shading = extract_string_field(content, "shading_mode");
        const auto runtime_profile = extract_string_field(content, "runtime_profile");
        const auto frames = extract_numeric_field<int>(content, "frames");
        const auto timestep = extract_numeric_field<double>(content, "timestep_seconds");
        const auto average_tick_ms = extract_numeric_field<double>(content, "average_tick_ms");
        const auto run_index = extract_numeric_field<int>(content, "run_index");
        const auto run_count = extract_numeric_field<int>(content, "run_count");

        SandboxBenchmarkResult result{};
        result.success = true;
        result.headline = "Benchmark succeeded";

        std::ostringstream details;
        details << "scenario=" << (scenario ? *scenario : std::string{"<unspecified>"});
        details << " runtime=" << (runtime_profile ? *runtime_profile : std::string{"<unspecified>"});
        details << " dataset=" << (dataset ? *dataset : std::string{"<none>"});
        details << " preset=" << (preset ? *preset : std::string{"<unspecified>"});
        details << " shading=" << (shading ? *shading : std::string{"<unspecified>"});
        if (frames)
        {
            details << " frames=" << *frames;
        }
        if (timestep)
        {
            const auto previous_precision = details.precision();
            details << " dt=";
            details.setf(std::ios::fixed, std::ios::floatfield);
            details << std::setprecision(6) << *timestep;
            details << std::setprecision(previous_precision);
        }
        if (average_tick_ms)
        {
            const auto previous_precision = details.precision();
            details << " avg_ms=";
            details << std::setprecision(3) << *average_tick_ms;
            details << std::setprecision(previous_precision);
        }
        if (run_index && run_count && *run_count > 0)
        {
            details << " run=" << *run_index << '/' << *run_count;
        }
        else if (run_index)
        {
            details << " run_index=" << *run_index;
        }
        details << "\nSummary saved to " << path.string();
        result.details = details.str();
        return result;
    }

    ComparativeBenchmarkRunner::ComparativeBenchmarkRunner(std::vector<std::string> command_prefix,
                                                           std::filesystem::path working_directory)
        : command_prefix_(std::move(command_prefix)), working_directory_(std::move(working_directory))
    {
    }

    ComparativeBenchmarkRunner::~ComparativeBenchmarkRunner() = default;

    SandboxBenchmarkResult ComparativeBenchmarkRunner::run(const BenchmarkScenarioDescriptor& scenario,
                                                           const SandboxPreferences& preferences) const
    {
        if (command_prefix_.empty())
        {
            return build_failure_result("Comparative benchmark command not configured",
                                         "Provide a command prefix that launches scripts/benchmarks/"
                                         "run_comparative_benchmarks.py before running comparative benchmarks.");
        }

        std::error_code directory_error{};
        std::filesystem::create_directories(working_directory_, directory_error);
        if (directory_error)
        {
            return build_failure_result("Failed to prepare comparative benchmark workspace", directory_error.message());
        }

        const auto run_directory = make_run_directory();
        std::filesystem::create_directories(run_directory, directory_error);
        if (directory_error)
        {
            return build_failure_result("Failed to create comparative benchmark run directory",
                                         directory_error.message());
        }

        const auto output_directory = run_directory / "outputs";
        std::filesystem::create_directories(output_directory, directory_error);
        if (directory_error)
        {
            return build_failure_result("Failed to prepare comparative benchmark output directory",
                                         directory_error.message());
        }

        const auto config_path = run_directory / "benchmark_config.json";
        if (!write_configuration(scenario, config_path, output_directory))
        {
            return build_failure_result("Failed to write comparative benchmark configuration",
                                         "Unable to emit configuration for run_comparative_benchmarks.");
        }

        const auto summary_path = output_directory / "comparative_summary.json";
        const auto table_path = output_directory / "comparative_summary.csv";

        std::vector<std::string> command = command_prefix_;
        command.emplace_back("--config");
        command.emplace_back(config_path.string());
        command.emplace_back("--output");
        command.emplace_back(summary_path.string());
        command.emplace_back("--table");
        command.emplace_back(table_path.string());

        const int exit_code = execute_command(command);
        if (exit_code != 0 && exit_code != 1)
        {
            std::ostringstream details;
            details << "Command:";
            for (const auto& token : command)
            {
                details << ' ' << token;
            }
            details << "\nExit code: " << exit_code;
            return build_failure_result("Comparative benchmark execution failed", details.str());
        }

        if (auto parsed = parse_summary(summary_path, table_path, scenario, preferences))
        {
            if (exit_code != 0)
            {
                parsed->success = false;
                if (parsed->headline.empty())
                {
                    parsed->headline = "Comparative benchmark detected a regression";
                }
            }
            return *parsed;
        }

        std::ostringstream details;
        details << "Command:";
        for (const auto& token : command)
        {
            details << ' ' << token;
        }
        details << "\nExpected summary at " << summary_path.string();
        return build_failure_result("Comparative benchmark summary missing", details.str());
    }

    std::string ComparativeBenchmarkRunner::quote_argument(const std::string& argument)
    {
        std::string quoted;
        quoted.reserve(argument.size() + 2);
        quoted.push_back('"');
        for (char ch : argument)
        {
            if (ch == '"' || ch == '\\')
            {
                quoted.push_back('\\');
            }
            quoted.push_back(ch);
        }
        quoted.push_back('"');
        return quoted;
    }

    int ComparativeBenchmarkRunner::execute_command(const std::vector<std::string>& command)
    {
        std::ostringstream builder;
        bool first = true;
        for (const auto& token : command)
        {
            if (!first)
            {
                builder << ' ';
            }
            builder << quote_argument(token);
            first = false;
        }

        const std::string command_line = builder.str();
        const int result = std::system(command_line.c_str());
        if (result == -1)
        {
            return -1;
        }
#ifdef _WIN32
        return result;
#else
        if (WIFEXITED(result))
        {
            return WEXITSTATUS(result);
        }
        if (WIFSIGNALED(result))
        {
            return 128 + WTERMSIG(result);
        }
        return result;
#endif
    }

    std::filesystem::path ComparativeBenchmarkRunner::make_run_directory() const
    {
        const auto now = std::chrono::system_clock::now();
        const auto timestamp = std::chrono::system_clock::to_time_t(now);
        const auto unique_suffix = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

        std::ostringstream directory;
        directory << "comparative_run_" << sanitise_filename_component(timestamp) << '_' << unique_suffix;
        return working_directory_ / directory.str();
    }

    std::string ComparativeBenchmarkRunner::sanitise_filename_component(std::time_t timestamp)
    {
        return format_timestamp_component(timestamp);
    }

    std::string ComparativeBenchmarkRunner::escape_json(std::string_view text)
    {
        std::string escaped;
        escaped.reserve(text.size() + 8);
        escaped.push_back('"');
        for (char ch : text)
        {
            switch (ch)
            {
            case '\\':
            case '"':
                escaped.push_back('\\');
                escaped.push_back(ch);
                break;
            case '\n':
                escaped.append("\\n");
                break;
            case '\r':
                escaped.append("\\r");
                break;
            case '\t':
                escaped.append("\\t");
                break;
            default:
                escaped.push_back(ch);
                break;
            }
        }
        escaped.push_back('"');
        return escaped;
    }

    std::string ComparativeBenchmarkRunner::to_lower(std::string_view text)
    {
        std::string lowered{text};
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return lowered;
    }

    bool ComparativeBenchmarkRunner::write_configuration(const BenchmarkScenarioDescriptor& scenario,
                                                         const std::filesystem::path& config_path,
                                                         const std::filesystem::path& output_directory) const
    {
        std::ofstream stream(config_path, std::ios::trunc);
        if (!stream.is_open())
        {
            return false;
        }

        const std::string scenario_name = !scenario.name.empty() ? scenario.name : scenario.identifier;
        stream << "{\n";
        stream << "  \"version\": 1,\n";
        stream << "  \"output_directory\": " << escape_json(output_directory.string()) << ",\n";
        stream << "  \"scenarios\": [\n";
        stream << "    {\n";
        stream << "      \"name\": " << escape_json(scenario_name) << ",\n";
        if (!scenario.dataset.empty())
        {
            stream << "      \"dataset\": " << escape_json(scenario.dataset) << ",\n";
        }

        auto write_command = [&](std::string_view label, const BenchmarkCommandDescriptor& command) {
            stream << "      \"" << label << "\": {\n";
            const std::string output_path = command.output.empty()
                                               ? std::string{"{output_dir}/"} + std::string{label} + "_metrics.json"
                                               : command.output;
            stream << "        \"output\": " << escape_json(output_path);
            if (!command.command.empty())
            {
                stream << ",\n        \"command\": [";
                for (std::size_t i = 0; i < command.command.size(); ++i)
                {
                    stream << escape_json(command.command[i]);
                    if (i + 1 < command.command.size())
                    {
                        stream << ", ";
                    }
                }
                stream << "]\n";
            }
            else
            {
                stream << '\n';
            }
            stream << "      }";
        };

        write_command("engine", scenario.engine);
        stream << ",\n";
        write_command("reference", scenario.reference);
        stream << ",\n";
        stream << "      \"metrics\": [\n";
        for (std::size_t i = 0; i < scenario.metrics.size(); ++i)
        {
            const auto& metric = scenario.metrics[i];
            stream << "        {\n";
            stream << "          \"name\": " << escape_json(metric.name) << ",\n";
            stream << "          \"higher_is_better\": " << (metric.higher_is_better ? "true" : "false") << ",\n";
            const std::string mode = to_lower(metric.threshold.mode);
            stream << "          \"threshold\": {\n";
            stream << "            \"type\": " << escape_json(mode) << ",\n";
            const double limit = metric.threshold.limit;
            if (mode == "relative")
            {
                stream << "            \"max_regression\": " << limit << '\n';
            }
            else
            {
                stream << "            \"max_delta\": " << limit << '\n';
            }
            stream << "          }\n";
            stream << "        }";
            if (i + 1 < scenario.metrics.size())
            {
                stream << ",";
            }
            stream << '\n';
        }
        stream << "      ]\n";
        stream << "    }\n";
        stream << "  ]\n";
        stream << "}\n";

        return stream.good();
    }

    SandboxBenchmarkResult ComparativeBenchmarkRunner::build_failure_result(std::string headline,
                                                                            std::string details) const
    {
        SandboxBenchmarkResult result{};
        result.success = false;
        result.headline = std::move(headline);
        result.details = std::move(details);
        return result;
    }

    namespace
    {
        struct MetricRow
        {
            std::string name;
            bool passed{false};
            double engine_value{0.0};
            double reference_value{0.0};
            double delta{0.0};
            std::optional<double> relative_delta{};
            std::string threshold_mode;
            double threshold_limit{0.0};
            double regression_amount{0.0};
        };

        struct ScenarioRow
        {
            std::string name;
            std::string dataset;
            bool passed{true};
            std::vector<MetricRow> metrics;
        };

        std::vector<std::string> parse_csv_line(const std::string& line)
        {
            std::vector<std::string> fields;
            std::string current;
            bool in_quotes = false;
            for (std::size_t i = 0; i < line.size(); ++i)
            {
                const char ch = line[i];
                if (ch == '"')
                {
                    if (in_quotes && i + 1 < line.size() && line[i + 1] == '"')
                    {
                        current.push_back('"');
                        ++i;
                    }
                    else
                    {
                        in_quotes = !in_quotes;
                    }
                }
                else if (ch == ',' && !in_quotes)
                {
                    fields.push_back(current);
                    current.clear();
                }
                else
                {
                    current.push_back(ch);
                }
            }
            fields.push_back(current);
            return fields;
        }

        std::string trim(std::string_view text)
        {
            std::size_t begin = 0;
            std::size_t end = text.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0)
            {
                ++begin;
            }
            while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
            {
                --end;
            }
            return std::string{text.substr(begin, end - begin)};
        }

        [[nodiscard]] std::optional<double> parse_double(std::string_view text)
        {
            const std::string trimmed = trim(text);
            if (trimmed.empty())
            {
                return std::nullopt;
            }
            double value{};
            const auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
            if (result.ec != std::errc{})
            {
                return std::nullopt;
            }
            return value;
        }
    } // namespace

    std::optional<SandboxBenchmarkResult> ComparativeBenchmarkRunner::parse_summary(
        const std::filesystem::path& summary_path,
        const std::filesystem::path& table_path,
        const BenchmarkScenarioDescriptor& scenario,
        const SandboxPreferences& preferences)
    {
        std::ifstream table_stream(table_path);
        if (!table_stream.is_open())
        {
            return std::nullopt;
        }

        std::string header_line;
        if (!std::getline(table_stream, header_line))
        {
            return std::nullopt;
        }

        const auto headers = parse_csv_line(header_line);
        std::unordered_map<std::string, std::size_t> column_lookup;
        for (std::size_t i = 0; i < headers.size(); ++i)
        {
            column_lookup.emplace(trim(headers[i]), i);
        }

        auto require_column = [&](std::string_view name) -> std::optional<std::size_t> {
            const auto it = column_lookup.find(std::string{name});
            if (it == column_lookup.end())
            {
                return std::nullopt;
            }
            return it->second;
        };

        const auto scenario_col = require_column("scenario");
        const auto dataset_col = require_column("dataset");
        const auto metric_col = require_column("metric");
        const auto passed_col = require_column("passed");
        const auto engine_col = require_column("engine_value");
        const auto reference_col = require_column("reference_value");
        const auto delta_col = require_column("delta");
        const auto relative_col = require_column("relative_delta");
        const auto threshold_mode_col = require_column("threshold_mode");
        const auto threshold_limit_col = require_column("threshold_limit");
        const auto regression_col = require_column("regression_amount");
        if (!scenario_col || !dataset_col || !metric_col || !passed_col || !engine_col || !reference_col || !delta_col
            || !relative_col || !threshold_mode_col || !threshold_limit_col || !regression_col)
        {
            return std::nullopt;
        }

        std::vector<ScenarioRow> scenarios;
        std::unordered_map<std::string, std::size_t> scenario_index;

        std::string line;
        while (std::getline(table_stream, line))
        {
            const auto fields = parse_csv_line(line);
            if (fields.size() <= *passed_col)
            {
                continue;
            }

            const std::string scenario_name = trim(fields[*scenario_col]);
            const std::string dataset = *dataset_col < fields.size() ? trim(fields[*dataset_col]) : std::string{};
            const std::string metric_name = *metric_col < fields.size() ? trim(fields[*metric_col]) : std::string{};
            const std::string passed_token = *passed_col < fields.size() ? trim(fields[*passed_col]) : std::string{};

            MetricRow metric{};
            metric.name = metric_name;
            metric.passed = (passed_token == "True" || passed_token == "true" || passed_token == "1");
            if (*engine_col < fields.size())
            {
                if (auto value = parse_double(fields[*engine_col]))
                {
                    metric.engine_value = *value;
                }
            }
            if (*reference_col < fields.size())
            {
                if (auto value = parse_double(fields[*reference_col]))
                {
                    metric.reference_value = *value;
                }
            }
            if (*delta_col < fields.size())
            {
                if (auto value = parse_double(fields[*delta_col]))
                {
                    metric.delta = *value;
                }
            }
            if (*relative_col < fields.size())
            {
                metric.relative_delta = parse_double(fields[*relative_col]);
            }
            if (*threshold_mode_col < fields.size())
            {
                metric.threshold_mode = trim(fields[*threshold_mode_col]);
            }
            if (*threshold_limit_col < fields.size())
            {
                if (auto value = parse_double(fields[*threshold_limit_col]))
                {
                    metric.threshold_limit = *value;
                }
            }
            if (*regression_col < fields.size())
            {
                if (auto value = parse_double(fields[*regression_col]))
                {
                    metric.regression_amount = *value;
                }
            }

            std::size_t index = 0;
            const auto it = scenario_index.find(scenario_name);
            if (it == scenario_index.end())
            {
                index = scenarios.size();
                ScenarioRow row{};
                row.name = scenario_name;
                row.dataset = dataset;
                scenarios.push_back(std::move(row));
                scenario_index.emplace(scenario_name, index);
            }
            else
            {
                index = it->second;
            }

            auto& row = scenarios[index];
            if (row.dataset.empty())
            {
                row.dataset = dataset;
            }
            row.metrics.push_back(metric);
            row.passed = row.passed && metric.passed;
        }

        if (scenarios.empty())
        {
            return std::nullopt;
        }

        bool overall_pass = true;
        for (const auto& row : scenarios)
        {
            overall_pass = overall_pass && row.passed;
        }

        std::ostringstream details;
        for (const auto& row : scenarios)
        {
            details << (row.passed ? "PASS" : "FAIL") << " " << row.name;
            if (!row.dataset.empty())
            {
                details << " (dataset: " << row.dataset << ')';
            }
            details << '\n';
            for (const auto& metric : row.metrics)
            {
                details << "  - " << metric.name << ": engine=";
                details.setf(std::ios::fixed, std::ios::floatfield);
                details << std::setprecision(4) << metric.engine_value;
                details << " reference=" << std::setprecision(4) << metric.reference_value;
                details << " delta=" << std::setprecision(4) << metric.delta;
                if (metric.relative_delta)
                {
                    details << " rel=" << std::setprecision(4) << *metric.relative_delta;
                }
                details << " (" << (metric.passed ? "PASS" : "FAIL") << ", threshold=" << metric.threshold_mode
                        << "≤" << std::setprecision(4) << metric.threshold_limit << ", regression="
                        << std::setprecision(4) << metric.regression_amount << ")\n";
            }
        }

        details << "Summary saved to " << summary_path.string() << '\n';
        details << "Table saved to " << table_path.string();

        if (!scenario.dataset.empty() && scenario.dataset != preferences.selected_dataset)
        {
            details << "\nWarning: sandbox dataset '" << preferences.selected_dataset
                    << "' differs from scenario dataset '" << scenario.dataset << "'.";
        }
        if (!scenario.runtime_profile.empty() && scenario.runtime_profile != preferences.selected_algorithm_variant)
        {
            details << "\nWarning: sandbox runtime profile '" << preferences.selected_algorithm_variant
                    << "' differs from scenario runtime profile '" << scenario.runtime_profile << "'.";
        }
        if (!scenario.rendering_preset.empty() && scenario.rendering_preset != preferences.selected_preset)
        {
            details << "\nWarning: sandbox preset '" << preferences.selected_preset
                    << "' differs from scenario preset '" << scenario.rendering_preset << "'.";
        }

        SandboxBenchmarkResult result{};
        result.success = overall_pass;
        result.headline = overall_pass ? "Comparative benchmark succeeded" : "Comparative benchmark failed";
        result.details = details.str();
        return result;
    }
}

