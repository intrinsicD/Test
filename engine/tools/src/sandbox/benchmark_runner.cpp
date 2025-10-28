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

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace engine::tools::sandbox
{
    namespace
    {
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

        const auto dataset = extract_string_field(content, "dataset");
        const auto preset = extract_string_field(content, "rendering_preset");
        const auto shading = extract_string_field(content, "shading_mode");
        const auto frames = extract_numeric_field<int>(content, "frames");
        const auto timestep = extract_numeric_field<double>(content, "timestep_seconds");

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

