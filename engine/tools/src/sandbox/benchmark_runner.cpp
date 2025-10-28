#include "engine/tools/sandbox/benchmark_runner.hpp"

#include <algorithm>
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

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace engine::tools::sandbox
{
    namespace
    {
        std::string trim_copy(std::string_view input)
        {
            std::size_t begin = 0;
            std::size_t end = input.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(input[begin])) != 0)
            {
                ++begin;
            }
            while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0)
            {
                --end;
            }
            return std::string{input.substr(begin, end - begin)};
        }

        std::string sanitise_filename_component(std::time_t timestamp)
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
        filename << "benchmark_summary_" << sanitise_filename_component(timestamp) << '_' << unique_suffix << ".json";
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

    std::optional<SandboxBenchmarkResult> PrototypeHarnessBenchmarkRunner::parse_summary(
        const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
        {
            return std::nullopt;
        }

        std::optional<std::string> dataset;
        std::optional<std::string> preset;
        std::optional<std::string> shading;
        std::optional<int> frames;
        std::optional<double> timestep;

        std::string line;
        while (std::getline(stream, line))
        {
            const auto first_quote = line.find('"');
            if (first_quote == std::string::npos)
            {
                continue;
            }
            const auto second_quote = line.find('"', first_quote + 1);
            if (second_quote == std::string::npos)
            {
                continue;
            }
            const auto key = line.substr(first_quote + 1, second_quote - first_quote - 1);
            const auto colon = line.find(':', second_quote);
            if (colon == std::string::npos)
            {
                continue;
            }

            std::string value = trim_copy(line.substr(colon + 1));
            if (!value.empty() && value.back() == ',')
            {
                value.pop_back();
            }

            if (!value.empty() && value.front() == '"' && value.back() == '"')
            {
                const auto unquoted = value.substr(1, value.size() - 2);
                if (key == "dataset")
                {
                    dataset = unquoted;
                }
                else if (key == "rendering_preset")
                {
                    preset = unquoted;
                }
                else if (key == "shading_mode")
                {
                    shading = unquoted;
                }
                continue;
            }

            if (value.empty())
            {
                continue;
            }

            try
            {
                if (key == "frames")
                {
                    frames = std::stoi(value);
                }
                else if (key == "timestep_seconds")
                {
                    timestep = std::stod(value);
                }
            }
            catch (...)
            {
                // Ignore parse errors for individual fields.
            }
        }

        SandboxBenchmarkResult result{};
        result.success = true;
        result.headline = "Benchmark succeeded";

        std::ostringstream details;
        details << "dataset=" << (dataset ? *dataset : std::string{"<none>"});
        details << " preset=" << (preset ? *preset : std::string{"<unspecified>"});
        details << " shading=" << (shading ? *shading : std::string{"<unspecified>"});
        if (frames)
        {
            details << " frames=" << *frames;
        }
        if (timestep)
        {
            details << " dt=";
            details.setf(std::ios::fixed, std::ios::floatfield);
            details << std::setprecision(6) << *timestep;
        }
        details << "\nSummary saved to " << path.string();
        result.details = details.str();
        return result;
    }
}

