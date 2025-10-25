#include "engine/runtime/api.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace compute = engine::compute;
namespace runtime = engine::runtime;

namespace
{
    struct CommandLineOptions
    {
        bool show_help{false};
        bool pretty_json{false};
        std::size_t frames{256U};
        double timestep{1.0 / 60.0};
        std::optional<std::filesystem::path> output_path{};
    };

    [[nodiscard]] std::string_view timing_domain_to_string(compute::TimingDomain domain) noexcept
    {
        switch (domain)
        {
        case compute::TimingDomain::Cpu:
            return "cpu";
        case compute::TimingDomain::Gpu:
            return "gpu";
        case compute::TimingDomain::Unknown:
        default:
            return "unknown";
        }
    }

    void print_usage(std::ostream& stream)
    {
        stream << "engine_compute_runtime_sample\n"
               << "Capture dispatcher telemetry from RuntimeHost and write a JSON summary." << '\n'
               << '\n'
               << "Usage:\n"
               << "  engine_compute_runtime_sample [--frames N] [--dt SECONDS] [--output FILE] [--pretty]\n"
               << '\n'
               << "Options:\n"
               << "  --frames N   Number of ticks to execute (default: 256)\n"
               << "  --dt SECONDS Simulation timestep for each tick (default: 1/60)\n"
               << "  --output FILE Write JSON payload to FILE instead of stdout\n"
               << "  --pretty     Emit indented JSON when writing to FILE\n"
               << "  --help       Show this message\n";
    }

    [[nodiscard]] double parse_double(std::string_view value)
    {
        std::string buffer{value};
        char* end = nullptr;
        const double parsed = std::strtod(buffer.c_str(), &end);
        if (end == buffer.c_str() || !std::isfinite(parsed))
        {
            throw std::invalid_argument{"Expected floating point value"};
        }
        return parsed;
    }

    [[nodiscard]] std::size_t parse_size(std::string_view value)
    {
        std::string buffer{value};
        char* end = nullptr;
        const unsigned long long parsed = std::strtoull(buffer.c_str(), &end, 10);  // NOLINT
        if (end == buffer.c_str())
        {
            throw std::invalid_argument{"Expected positive integer"};
        }
        if (parsed > std::numeric_limits<std::size_t>::max())
        {
            throw std::out_of_range{"Value exceeds size_t range"};
        }
        return static_cast<std::size_t>(parsed);
    }

    CommandLineOptions parse_command_line(int argc, char* argv[])
    {
        CommandLineOptions options{};
        for (int index = 1; index < argc; ++index)
        {
            const std::string_view argument{argv[index]};
            if (argument == "--help" || argument == "-h")
            {
                options.show_help = true;
                continue;
            }
            if (argument == "--pretty")
            {
                options.pretty_json = true;
                continue;
            }
            if (argument == "--frames")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--frames requires a value"};
                }
                options.frames = parse_size(argv[++index]);
                continue;
            }
            if (argument == "--dt")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--dt requires a value"};
                }
                options.timestep = parse_double(argv[++index]);
                continue;
            }
            if (argument == "--output")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--output requires a path"};
                }
                options.output_path = std::filesystem::path{argv[++index]};
                continue;
            }

            std::ostringstream stream;
            stream << "Unknown argument: " << argument;
            throw std::invalid_argument(stream.str());
        }

        if (options.frames == 0U)
        {
            throw std::invalid_argument{"--frames must be greater than zero"};
        }
        if (!(options.timestep > 0.0))
        {
            throw std::invalid_argument{"--dt must be positive"};
        }

        return options;
    }

    [[nodiscard]] std::string escape_json(std::string_view text)
    {
        std::ostringstream stream;
        stream << std::hex << std::setfill('0');

        for (unsigned char ch : text)
        {
            switch (ch)
            {
            case '\\':
                stream << "\\\\";
                break;
            case '"':
                stream << "\\\"";
                break;
            case '\b':
                stream << "\\b";
                break;
            case '\f':
                stream << "\\f";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (std::iscntrl(static_cast<int>(ch)) != 0)
                {
                    stream << "\\u" << std::setw(4) << static_cast<int>(ch);
                }
                else
                {
                    stream << static_cast<char>(ch);
                }
                break;
            }
        }

        return stream.str();
    }

    void write_indent(std::ostream& stream, std::size_t level, bool pretty)
    {
        if (!pretty)
        {
            return;
        }
        for (std::size_t count = 0; count < level; ++count)
        {
            stream << "  ";
        }
    }

    struct DispatchSample
    {
        std::string name{};
        std::string category{};
        double duration_ms{0.0};
    };

    struct FrameSample
    {
        std::size_t index{0U};
        double simulation_time{0.0};
        double timestep{0.0};
        double total_ms{0.0};
        std::vector<DispatchSample> dispatches{};
        std::map<std::string, double> category_totals{};
    };

    struct SummaryStats
    {
        double mean_ms{0.0};
        double min_ms{0.0};
        double max_ms{0.0};
        double stddev_ms{0.0};
        double total_ms{0.0};
        std::size_t samples{0U};

        [[nodiscard]] double jitter_percent() const noexcept
        {
            if (samples == 0U || mean_ms <= 0.0)
            {
                return 0.0;
            }
            return (stddev_ms / mean_ms) * 100.0;
        }
    };

    struct RunSummary
    {
        std::map<std::string, SummaryStats> dispatches{};
        std::map<std::string, SummaryStats> categories{};
        SummaryStats frame_totals{};
    };

    struct RunResult
    {
        std::vector<FrameSample> frames{};
        RunSummary summary{};
        std::string clock_name{};
        compute::TimingDomain clock_domain{compute::TimingDomain::Unknown};
    };

    [[nodiscard]] SummaryStats compute_summary_stats(const std::vector<double>& values)
    {
        SummaryStats stats{};
        if (values.empty())
        {
            return stats;
        }

        stats.samples = values.size();
        stats.min_ms = *std::min_element(values.begin(), values.end());
        stats.max_ms = *std::max_element(values.begin(), values.end());
        stats.total_ms = std::accumulate(values.begin(), values.end(), 0.0);
        stats.mean_ms = stats.total_ms / static_cast<double>(values.size());

        double variance = 0.0;
        for (double value : values)
        {
            const double delta = value - stats.mean_ms;
            variance += delta * delta;
        }
        variance /= static_cast<double>(values.size());
        stats.stddev_ms = std::sqrt(std::max(0.0, variance));
        return stats;
    }

    [[nodiscard]] std::string dispatch_category(std::string_view name)
    {
        const auto separator = name.find('.');
        if (separator == std::string_view::npos)
        {
            return std::string{name};
        }
        return std::string{name.substr(0, separator)};
    }

    [[nodiscard]] RunResult run_sample(const CommandLineOptions& options, runtime::RuntimeHost& host)
    {
        RunResult result{};

        std::map<std::string, std::vector<double>> durations_by_dispatch{};
        std::map<std::string, std::vector<double>> durations_by_category{};
        std::vector<double> frame_totals{};
        frame_totals.reserve(options.frames);

        result.frames.reserve(options.frames);

        for (std::size_t index = 0; index < options.frames; ++index)
        {
            const runtime::runtime_frame_state frame_state = host.tick(options.timestep);
            FrameSample frame{};
            frame.index = index;
            frame.simulation_time = frame_state.simulation_time;
            frame.timestep = options.timestep;

            const compute::ExecutionReport& report = frame_state.dispatch_report;
            if (result.clock_name.empty())
            {
                result.clock_name = report.clock_name;
                result.clock_domain = report.clock_domain;
            }

            const std::size_t dispatch_count = report.execution_order.size();
            frame.dispatches.reserve(dispatch_count);

            for (std::size_t dispatch_index = 0; dispatch_index < dispatch_count; ++dispatch_index)
            {
                const std::string& name = report.execution_order[dispatch_index];
                const double duration_s = report.kernel_durations.size() > dispatch_index
                                               ? report.kernel_durations[dispatch_index]
                                               : 0.0;
                const double duration_ms = duration_s * 1000.0;
                const std::string category = dispatch_category(name);

                frame.dispatches.push_back(DispatchSample{
                    .name = name,
                    .category = category,
                    .duration_ms = duration_ms,
                });

                frame.category_totals[category] += duration_ms;
                frame.total_ms += duration_ms;
                durations_by_dispatch[name].push_back(duration_ms);
                durations_by_category[category].push_back(duration_ms);
            }

            frame_totals.push_back(frame.total_ms);
            result.frames.push_back(std::move(frame));
        }

        result.summary.frame_totals = compute_summary_stats(frame_totals);
        for (auto& [name, values] : durations_by_dispatch)
        {
            result.summary.dispatches[name] = compute_summary_stats(values);
        }
        for (auto& [category, values] : durations_by_category)
        {
            result.summary.categories[category] = compute_summary_stats(values);
        }

        return result;
    }

    void print_text_summary(const RunResult& result)
    {
        std::cout << "=== Compute Dispatcher Runtime Sample ===\n";
        std::cout << "Frames: " << result.summary.frame_totals.samples << '\n';
        std::cout << "Average frame dispatch time: " << std::fixed << std::setprecision(3)
                  << result.summary.frame_totals.mean_ms << " ms\n";
        std::cout << "Clock: " << result.clock_name << " ("
                  << timing_domain_to_string(result.clock_domain) << ")\n";

        std::vector<std::pair<std::string, SummaryStats>> top_dispatches{};
        top_dispatches.reserve(result.summary.dispatches.size());
        for (const auto& [name, stats] : result.summary.dispatches)
        {
            top_dispatches.emplace_back(name, stats);
        }
        std::sort(top_dispatches.begin(), top_dispatches.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second.mean_ms > rhs.second.mean_ms;
        });
        const std::size_t count = std::min<std::size_t>(top_dispatches.size(), 5U);
        if (count > 0U)
        {
            std::cout << '\n' << "Top dispatcher kernels by average duration:" << '\n';
            for (std::size_t index = 0; index < count; ++index)
            {
                const auto& [name, stats] = top_dispatches[index];
                std::cout << "  - " << name << ": " << std::fixed << std::setprecision(3)
                          << stats.mean_ms << " ms (jitter " << std::setprecision(2)
                          << stats.jitter_percent() << "%)\n";
            }
        }

        if (!result.summary.categories.empty())
        {
            std::cout << '\n' << "Category totals:" << '\n';
            for (const auto& [category, stats] : result.summary.categories)
            {
                std::cout << "  - " << category << ": total " << std::fixed << std::setprecision(3)
                          << stats.total_ms << " ms across " << stats.samples << " samples\n";
            }
        }

        std::cout << std::endl;
    }

    void write_summary_stats(std::ostream& stream,
                             const SummaryStats& stats,
                             std::size_t indent_level,
                             bool pretty)
    {
        write_indent(stream, indent_level, pretty);
        stream << "{";
        if (pretty)
        {
            stream << '\n';
        }

        auto emit_field = [&](std::string_view key, double value, bool last) {
            write_indent(stream, indent_level + 1U, pretty);
            stream << '"' << key << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << std::fixed << std::setprecision(6) << value;
            if (!last)
            {
                stream << ',';
            }
            if (pretty)
            {
                stream << '\n';
            }
        };

        emit_field("mean_ms", stats.mean_ms, false);
        emit_field("min_ms", stats.min_ms, false);
        emit_field("max_ms", stats.max_ms, false);
        emit_field("stddev_ms", stats.stddev_ms, false);
        emit_field("total_ms", stats.total_ms, false);

        write_indent(stream, indent_level + 1U, pretty);
        stream << '"' << "samples" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << stats.samples;
        if (pretty)
        {
            stream << '\n';
        }

        write_indent(stream, indent_level, pretty);
        stream << "}";
    }

    void write_json_report(const CommandLineOptions& options,
                           const RunResult& result,
                           const runtime::RuntimeDiagnostics& diagnostics,
                           std::ostream& stream)
    {
        const bool pretty = options.pretty_json;
        stream << std::fixed << std::setprecision(6);
        stream << "{";
        if (pretty)
        {
            stream << '\n';
        }

        // Metadata
        write_indent(stream, 1U, pretty);
        stream << '"' << "metadata" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "{";
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "frames" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << result.summary.frame_totals.samples << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "timestep" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        const double timestep = result.frames.empty() ? options.timestep : result.frames.front().timestep;
        stream << timestep << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "dispatcher_size" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        const std::size_t dispatcher_size = result.frames.empty() ? 0U : result.frames.front().dispatches.size();
        stream << dispatcher_size << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "clock" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "{";
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "name" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << '"' << escape_json(result.clock_name) << '"' << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "domain" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << '"' << timing_domain_to_string(result.clock_domain) << '"' << '\n';
        write_indent(stream, 2U, pretty);
        stream << "}";
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 1U, pretty);
        stream << "},";
        if (pretty)
        {
            stream << '\n';
        }

        // Frames
        write_indent(stream, 1U, pretty);
        stream << '"' << "frames" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        for (std::size_t frame_index = 0; frame_index < result.frames.size(); ++frame_index)
        {
            const FrameSample& frame = result.frames[frame_index];
            write_indent(stream, 2U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "index" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << frame.index << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "simulation_time" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << frame.simulation_time << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "timestep" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << frame.timestep << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "total_ms" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << frame.total_ms << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "dispatches" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << "[";
            if (pretty)
            {
                stream << '\n';
            }
            for (std::size_t dispatch_index = 0; dispatch_index < frame.dispatches.size(); ++dispatch_index)
            {
                const DispatchSample& dispatch = frame.dispatches[dispatch_index];
                write_indent(stream, 4U, pretty);
                stream << "{";
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "name" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << '"' << escape_json(dispatch.name) << '"' << ',';
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "category" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << '"' << escape_json(dispatch.category) << '"' << ',';
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "duration_ms" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << dispatch.duration_ms << '\n';
                write_indent(stream, 4U, pretty);
                stream << "}";
                if (dispatch_index + 1U < frame.dispatches.size())
                {
                    stream << ',';
                }
                if (pretty)
                {
                    stream << '\n';
                }
            }
            write_indent(stream, 3U, pretty);
            stream << "],";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "category_totals_ms" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << "[";
            if (pretty)
            {
                stream << '\n';
            }
            std::size_t category_index = 0;
            for (const auto& [category, total] : frame.category_totals)
            {
                write_indent(stream, 4U, pretty);
                stream << "{";
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "category" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << '"' << escape_json(category) << '"' << ',';
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "duration_ms" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << total << '\n';
                write_indent(stream, 4U, pretty);
                stream << "}";
                if (++category_index < frame.category_totals.size())
                {
                    stream << ',';
                }
                if (pretty)
                {
                    stream << '\n';
                }
            }
            write_indent(stream, 3U, pretty);
            stream << "]\n";
            write_indent(stream, 2U, pretty);
            stream << "}";
            if (frame_index + 1U < result.frames.size())
            {
                stream << ',';
            }
            if (pretty)
            {
                stream << '\n';
            }
        }
        write_indent(stream, 1U, pretty);
        stream << "],";
        if (pretty)
        {
            stream << '\n';
        }

        // Summary
        write_indent(stream, 1U, pretty);
        stream << '"' << "summary" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "{";
        if (pretty)
        {
            stream << '\n';
        }

        // Dispatch summary
        write_indent(stream, 2U, pretty);
        stream << '"' << "dispatches" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        std::size_t dispatch_index = 0;
        for (const auto& [name, stats] : result.summary.dispatches)
        {
            write_indent(stream, 3U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "name" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(name) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "stats" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            write_summary_stats(stream, stats, 4U, pretty);
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << "}";
            if (++dispatch_index < result.summary.dispatches.size())
            {
                stream << ',';
            }
            if (pretty)
            {
                stream << '\n';
            }
        }
        write_indent(stream, 2U, pretty);
        stream << "],";
        if (pretty)
        {
            stream << '\n';
        }

        // Category summary
        write_indent(stream, 2U, pretty);
        stream << '"' << "categories" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        std::size_t summary_category_index = 0;
        for (const auto& [category, stats] : result.summary.categories)
        {
            write_indent(stream, 3U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "name" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(category) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "stats" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            write_summary_stats(stream, stats, 4U, pretty);
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << "}";
            if (++summary_category_index < result.summary.categories.size())
            {
                stream << ',';
            }
            if (pretty)
            {
                stream << '\n';
            }
        }
        write_indent(stream, 2U, pretty);
        stream << "],";
        if (pretty)
        {
            stream << '\n';
        }

        // Frame totals
        write_indent(stream, 2U, pretty);
        stream << '"' << "frame_totals_ms" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        write_summary_stats(stream, result.summary.frame_totals, 2U, pretty);
        stream << ',';
        if (pretty)
        {
            stream << '\n';
        }

        // Diagnostics subset
        write_indent(stream, 2U, pretty);
        stream << '"' << "stage_timings" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        for (std::size_t stage_index = 0; stage_index < diagnostics.stage_timings.size(); ++stage_index)
        {
            const runtime::RuntimeStageTiming& timing = diagnostics.stage_timings[stage_index];
            write_indent(stream, 3U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "name" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(timing.name) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "last_ms" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << timing.last_ms << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "average_ms" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << timing.average_ms << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "max_ms" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << timing.max_ms << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "samples" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << timing.sample_count << '\n';
            write_indent(stream, 3U, pretty);
            stream << "}";
            if (stage_index + 1U < diagnostics.stage_timings.size())
            {
                stream << ',';
            }
            if (pretty)
            {
                stream << '\n';
            }
        }
        write_indent(stream, 2U, pretty);
        stream << "]\n";

        write_indent(stream, 1U, pretty);
        stream << "}";
        stream << ',';
        if (pretty)
        {
            stream << '\n';
        }

        // Diagnostics overview
        write_indent(stream, 1U, pretty);
        stream << '"' << "diagnostics" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "{";
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "tick_count" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << diagnostics.tick_count << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "average_tick_ms" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << diagnostics.average_tick_ms << '\n';
        write_indent(stream, 1U, pretty);
        stream << "}";
        if (pretty)
        {
            stream << '\n';
        }

        stream << "}";
        if (pretty)
        {
            stream << '\n';
        }
        stream << "}" << std::endl;
    }
}  // namespace

int main(int argc, char* argv[])
{
    try
    {
        const CommandLineOptions options = parse_command_line(argc, argv);
        if (options.show_help)
        {
            print_usage(std::cout);
            return 0;
        }

        runtime::RuntimeHost host{};
        host.initialize();

        RunResult result = run_sample(options, host);
        const runtime::RuntimeDiagnostics diagnostics = host.diagnostics();
        host.shutdown();

        print_text_summary(result);

        if (options.output_path)
        {
            const std::filesystem::path parent = options.output_path->parent_path();
            if (!parent.empty())
            {
                std::filesystem::create_directories(parent);
            }
            std::ofstream file{*options.output_path, std::ios::binary};
            if (!file)
            {
                std::cerr << "Failed to open output file: " << options.output_path->string() << std::endl;
                return 1;
            }
            write_json_report(options, result, diagnostics, file);
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << std::endl;
        print_usage(std::cerr);
        return 1;
    }
}
