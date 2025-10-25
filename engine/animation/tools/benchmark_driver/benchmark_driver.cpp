#include "engine/animation/api.hpp"
#include "engine/animation/benchmarking/statistics.hpp"
#include "engine/animation/benchmarking/telemetry.hpp"
#include "engine/compute/api.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <utility>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace animation = engine::animation;
namespace benchmarking = engine::animation::benchmarking;
namespace compute = engine::compute;

namespace
{
    using Clock = std::chrono::steady_clock;

    enum class Scenario
    {
        CpuBaseline,
        GpuAsync,
    };

    [[nodiscard]] std::string_view to_string(Scenario scenario) noexcept
    {
        switch (scenario)
        {
        case Scenario::CpuBaseline:
            return "cpu_baseline";
        case Scenario::GpuAsync:
            return "gpu_async";
        }
        return "cpu_baseline";
    }

    struct CommandLineOptions
    {
        bool show_help{false};
        std::size_t frames{1024U};
        double timestep{1.0 / 60.0};
        std::optional<std::filesystem::path> clip_path{};
        std::optional<std::filesystem::path> output_path{};
        bool pretty{false};
        std::optional<std::size_t> rig_joints{};
        double jitter_budget_ms{0.5};
        Scenario scenario{Scenario::CpuBaseline};
    };

    struct FrameTelemetry
    {
        std::size_t index{0U};
        double simulation_time{0.0};
        double duration_ms{0.0};
        std::vector<benchmarking::DispatchTelemetry> dispatches{};
        std::vector<benchmarking::AggregatedTelemetry> category_totals{};
        std::vector<benchmarking::AggregatedTelemetry> queue_totals{};
    };

    struct CaptureResult
    {
        std::vector<FrameTelemetry> frames{};
        benchmarking::FrameTimingSummary summary{};
        std::size_t pose_joint_count{0U};
        bool cuda_available{false};
    };

    [[nodiscard]] std::size_t parse_size(std::string_view value, std::string_view flag)
    {
        std::size_t result{};
        const auto* begin = value.data();
        const auto* end = begin + value.size();
        const auto [ptr, ec] = std::from_chars(begin, end, result);
        if (ec != std::errc{} || ptr != end)
        {
            std::ostringstream stream;
            stream << "Invalid numeric value for " << flag << ": " << value;
            throw std::invalid_argument(stream.str());
        }
        return result;
    }

    [[nodiscard]] double parse_double(std::string_view value, std::string_view flag)
    {
        double result{};
        const auto* begin = value.data();
        const auto* end = begin + value.size();
        const auto [ptr, ec] = std::from_chars(begin, end, result);
        if (ec != std::errc{} || ptr != end)
        {
            std::ostringstream stream;
            stream << "Invalid floating-point value for " << flag << ": " << value;
            throw std::invalid_argument(stream.str());
        }
        return result;
    }

    [[nodiscard]] Scenario parse_scenario(std::string_view value)
    {
        if (value == "cpu_baseline")
        {
            return Scenario::CpuBaseline;
        }
        if (value == "gpu_async")
        {
            return Scenario::GpuAsync;
        }

        std::ostringstream stream;
        stream << "Unknown scenario: " << value;
        throw std::invalid_argument(stream.str());
    }

    CommandLineOptions parse_command_line(int argc, char** argv)
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
            if (argument == "--frames")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--frames requires a value"};
                }
                options.frames = parse_size(argv[++index], "--frames");
                continue;
            }
            if (argument == "--dt")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--dt requires a value"};
                }
                options.timestep = parse_double(argv[++index], "--dt");
                continue;
            }
            if (argument == "--clip")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--clip requires a value"};
                }
                options.clip_path = std::filesystem::path{argv[++index]};
                continue;
            }
            if (argument == "--output")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--output requires a value"};
                }
                options.output_path = std::filesystem::path{argv[++index]};
                continue;
            }
            if (argument == "--rig-joints")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--rig-joints requires a value"};
                }
                options.rig_joints = parse_size(argv[++index], "--rig-joints");
                continue;
            }
            if (argument == "--scenario")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--scenario requires a value"};
                }
                options.scenario = parse_scenario(argv[++index]);
                continue;
            }
            if (argument == "--jitter-budget-ms")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--jitter-budget-ms requires a value"};
                }
                options.jitter_budget_ms = parse_double(argv[++index], "--jitter-budget-ms");
                continue;
            }
            if (argument == "--pretty")
            {
                options.pretty = true;
                continue;
            }

            std::ostringstream stream;
            stream << "Unknown argument: " << argument;
            throw std::invalid_argument(stream.str());
        }
        return options;
    }

    void print_help()
    {
        std::cout << "Animation Benchmark Driver\n\n";
        std::cout << "Usage:\n";
        std::cout << "  engine_animation_benchmark_driver [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --frames N            Number of frames to sample (default: 1024)\n";
        std::cout << "  --dt SECONDS         Simulation timestep per frame (default: 1/60)\n";
        std::cout << "  --clip FILE          Animation clip JSON file to load\n";
        std::cout << "  --rig-joints N       Rig joint count metadata (defaults to track count)\n";
        std::cout << "  --output FILE        JSON output path for telemetry\n";
        std::cout << "  --scenario NAME      Benchmark scenario (cpu_baseline | gpu_async)\n";
        std::cout << "  --jitter-budget-ms VALUE  Frame jitter budget (default: 0.5)\n";
        std::cout << "  --pretty             Pretty-print JSON output\n";
        std::cout << "  --help               Display this message\n";
    }

    [[nodiscard]] animation::AnimationClip load_clip(const CommandLineOptions& options)
    {
        if (!options.clip_path)
        {
            return animation::make_default_clip();
        }

        return animation::load_clip_json(*options.clip_path);
    }

    [[nodiscard]] CaptureResult capture_cpu_frames(const animation::AnimationClip& clip,
                                                   std::size_t frames,
                                                   double timestep)
    {
        animation::AnimationController controller = animation::make_linear_controller(clip);
        CaptureResult result{};
        result.frames.reserve(frames);

        std::vector<double> frame_durations{};
        frame_durations.reserve(frames);

        for (std::size_t index = 0U; index < frames; ++index)
        {
            const auto start = Clock::now();
            animation::advance_controller(controller, timestep);
            const animation::AnimationRigPose pose = animation::evaluate_controller(controller);
            const auto end = Clock::now();

            const double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            frame_durations.push_back(duration_ms);

            FrameTelemetry frame{};
            frame.index = index;
            frame.simulation_time = static_cast<double>(index + 1U) * timestep;
            frame.duration_ms = duration_ms;
            frame.dispatches.push_back(benchmarking::DispatchTelemetry{
                .name = "animation.sample_clip",
                .category = "animation.sample",
                .queue = "cpu",
                .duration_ms = duration_ms,
            });
            frame.category_totals = benchmarking::aggregate_category_totals(frame.dispatches);
            frame.queue_totals = benchmarking::aggregate_queue_totals(frame.dispatches);
            result.pose_joint_count = pose.joints.size();
            result.frames.push_back(std::move(frame));
        }

        result.summary = benchmarking::compute_frame_timing_summary(frame_durations);
        return result;
    }

    [[nodiscard]] compute::ClockConfiguration make_gpu_clock_configuration()
    {
        compute::ClockConfiguration configuration{};
        configuration.name = "gpu_simulated_clock";
        configuration.domain = compute::TimingDomain::Gpu;
        configuration.measure = [](const compute::kernel_callback& callback) -> double {
            const auto start = Clock::now();
            if (callback)
            {
                callback();
            }
            const auto end = Clock::now();
            return std::chrono::duration<double, std::milli>(end - start).count();
        };
        return configuration;
    }

    [[nodiscard]] CaptureResult capture_gpu_frames(const animation::AnimationClip& clip,
                                                   std::size_t frames,
                                                   double timestep)
    {
        animation::AnimationController controller = animation::make_linear_controller(clip);
        CaptureResult result{};
        result.frames.reserve(frames);
        result.cuda_available = compute::is_cuda_dispatcher_available();

        std::vector<double> frame_durations{};
        frame_durations.reserve(frames);

        animation::AnimationRigPose pose{};

        for (std::size_t index = 0U; index < frames; ++index)
        {
            const auto cpu_start = Clock::now();
            animation::advance_controller(controller, timestep);
            const auto cpu_end = Clock::now();
            const double submission_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

            auto dispatcher = compute::make_cuda_dispatcher();
            dispatcher->set_clock(make_gpu_clock_configuration());
            dispatcher->clear();
            dispatcher->add_kernel("animation.sample_clip.gpu", [&controller, &pose]() {
                pose = animation::evaluate_controller(controller);
            });

            const compute::ExecutionReport report = dispatcher->dispatch();
            const double kernel_ms = !report.kernel_durations.empty() ? report.kernel_durations.front() : 0.0;

            FrameTelemetry frame{};
            frame.index = index;
            frame.simulation_time = static_cast<double>(index + 1U) * timestep;
            frame.duration_ms = submission_ms + kernel_ms;
            frame.dispatches.push_back(benchmarking::DispatchTelemetry{
                .name = "animation.submit_sample",
                .category = "animation.control",
                .queue = "cpu",
                .duration_ms = submission_ms,
            });
            frame.dispatches.push_back(benchmarking::DispatchTelemetry{
                .name = "animation.sample_clip.gpu",
                .category = "animation.sample",
                .queue = "gpu",
                .duration_ms = kernel_ms,
            });
            frame.category_totals = benchmarking::aggregate_category_totals(frame.dispatches);
            frame.queue_totals = benchmarking::aggregate_queue_totals(frame.dispatches);
            result.pose_joint_count = pose.joints.size();
            result.frames.push_back(std::move(frame));

            frame_durations.push_back(submission_ms + kernel_ms);
        }

        result.summary = benchmarking::compute_frame_timing_summary(frame_durations);
        return result;
    }

    [[nodiscard]] std::string escape_json(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (const unsigned char ch : value)
        {
            switch (ch)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
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
                escaped.push_back(static_cast<char>(ch));
                break;
            }
        }
        return escaped;
    }

    void emit_indentation(std::ostream& stream, int depth, bool pretty)
    {
        if (!pretty)
        {
            return;
        }

        stream << '\n';
        for (int level = 0; level < depth; ++level)
        {
            stream << "  ";
        }
    }

    void write_summary(std::ostream& stream,
                       const benchmarking::FrameTimingSummary& summary,
                       int depth,
                       bool pretty)
    {
        emit_indentation(stream, depth, pretty);
        stream << '"' << "frame_totals_ms" << '"' << ':';
        stream << (pretty ? " {" : "{");

        emit_indentation(stream, depth + 1, pretty);
        stream << '"' << "samples" << '"' << ':' << summary.samples << ',';
        emit_indentation(stream, depth + 1, pretty);
        stream << '"' << "total_ms" << '"' << ':' << std::setprecision(15) << summary.total_ms << ',';
        emit_indentation(stream, depth + 1, pretty);
        stream << '"' << "mean_ms" << '"' << ':' << summary.mean_ms << ',';
        emit_indentation(stream, depth + 1, pretty);
        stream << '"' << "min_ms" << '"' << ':' << summary.min_ms << ',';
        emit_indentation(stream, depth + 1, pretty);
        stream << '"' << "max_ms" << '"' << ':' << summary.max_ms << ',';
        emit_indentation(stream, depth + 1, pretty);
        stream << '"' << "stddev_ms" << '"' << ':' << summary.stddev_ms;
        emit_indentation(stream, depth, pretty);
        stream << (pretty ? " }" : "}");
    }

    void write_frames(std::ostream& stream,
                      const std::vector<FrameTelemetry>& frames,
                      double timestep,
                      int depth,
                      bool pretty)
    {
        emit_indentation(stream, depth, pretty);
        stream << '"' << "frames" << '"' << ':' << (pretty ? " [" : "[");

        for (std::size_t index = 0; index < frames.size(); ++index)
        {
            const FrameTelemetry& frame = frames[index];
            emit_indentation(stream, depth + 1, pretty);
            stream << '{';

            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "index" << '"' << ':' << frame.index << ',';
            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "simulation_time" << '"' << ':' << frame.simulation_time << ',';
            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "timestep" << '"' << ':' << timestep << ',';
            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "total_ms" << '"' << ':' << frame.duration_ms << ',';

            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "dispatches" << '"' << ':' << (pretty ? " [" : "[");
            for (std::size_t dispatch_index = 0; dispatch_index < frame.dispatches.size(); ++dispatch_index)
            {
                const auto& dispatch = frame.dispatches[dispatch_index];
                emit_indentation(stream, depth + 3, pretty);
                stream << '{';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "name" << '"' << ':' << '"' << escape_json(dispatch.name) << '"' << ',';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "category" << '"' << ':' << '"' << escape_json(dispatch.category) << '"' << ',';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "queue" << '"' << ':' << '"' << escape_json(dispatch.queue) << '"' << ',';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "duration_ms" << '"' << ':' << dispatch.duration_ms;
                emit_indentation(stream, depth + 3, pretty);
                stream << '}';
                if (dispatch_index + 1U < frame.dispatches.size())
                {
                    stream << ',';
                }
            }
            emit_indentation(stream, depth + 2, pretty);
            stream << (pretty ? " ]," : "],");

            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "category_totals_ms" << '"' << ':' << (pretty ? " [" : "[");
            for (std::size_t category_index = 0; category_index < frame.category_totals.size(); ++category_index)
            {
                const auto& total = frame.category_totals[category_index];
                emit_indentation(stream, depth + 3, pretty);
                stream << '{';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "category" << '"' << ':' << '"' << escape_json(total.label) << '"' << ',';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "duration_ms" << '"' << ':' << total.duration_ms;
                emit_indentation(stream, depth + 3, pretty);
                stream << '}';
                if (category_index + 1U < frame.category_totals.size())
                {
                    stream << ',';
                }
            }
            emit_indentation(stream, depth + 2, pretty);
            stream << (pretty ? " ]," : "],");

            emit_indentation(stream, depth + 2, pretty);
            stream << '"' << "queue_totals_ms" << '"' << ':' << (pretty ? " [" : "[");
            for (std::size_t queue_index = 0; queue_index < frame.queue_totals.size(); ++queue_index)
            {
                const auto& total = frame.queue_totals[queue_index];
                emit_indentation(stream, depth + 3, pretty);
                stream << '{';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "queue" << '"' << ':' << '"' << escape_json(total.label) << '"' << ',';
                emit_indentation(stream, depth + 4, pretty);
                stream << '"' << "duration_ms" << '"' << ':' << total.duration_ms;
                emit_indentation(stream, depth + 3, pretty);
                stream << '}';
                if (queue_index + 1U < frame.queue_totals.size())
                {
                    stream << ',';
                }
            }
            emit_indentation(stream, depth + 2, pretty);
            stream << (pretty ? " ]" : "]");

            emit_indentation(stream, depth + 1, pretty);
            stream << '}';
            if (index + 1U < frames.size())
            {
                stream << ',';
            }
        }

        emit_indentation(stream, depth, pretty);
        stream << (pretty ? " ]" : "]");
    }

    void write_json(std::ostream& stream,
                    const CommandLineOptions& options,
                    const animation::AnimationClip& clip,
                    const CaptureResult& capture)
    {
        const bool pretty = options.pretty;
        const std::size_t track_count = clip.tracks.size();
        const std::size_t rig_joints = options.rig_joints.value_or(std::max(track_count, capture.pose_joint_count));
        const bool jitter_exceeded = options.jitter_budget_ms >= 0.0
                                     && capture.summary.stddev_ms > options.jitter_budget_ms;
        const std::string_view scenario_name = to_string(options.scenario);
        const char* task_id = options.scenario == Scenario::GpuAsync ? "AN-230.2" : "AN-230.1";

        stream << '{';

        emit_indentation(stream, 1, pretty);
        stream << '"' << "metadata" << '"' << ':' << (pretty ? " {" : "{");
        emit_indentation(stream, 2, pretty);
        stream << '"' << "task" << '"' << ':' << '"' << task_id << '"' << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "clip_name" << '"' << ':' << '"' << escape_json(clip.name) << '"' << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "clip_duration" << '"' << ':' << clip.duration << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "track_count" << '"' << ':' << track_count << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "rig_joint_count" << '"' << ':' << rig_joints << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "frames" << '"' << ':' << options.frames << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "timestep" << '"' << ':' << options.timestep << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "scenario" << '"' << ':' << '"' << scenario_name << '"' << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "generator" << '"' << ':' << '"' << "engine_animation_benchmark_driver" << '"';
        stream << ',';
        emit_indentation(stream, 2, pretty);
        stream << '"' << "cuda_available" << '"' << ':' << (capture.cuda_available ? "true" : "false");
        if (options.clip_path)
        {
            stream << ',';
            emit_indentation(stream, 2, pretty);
            stream << '"' << "clip_source" << '"' << ':' << '"' << escape_json(options.clip_path->string()) << '"';
        }
        emit_indentation(stream, 1, pretty);
        stream << (pretty ? " }," : "},");

        emit_indentation(stream, 1, pretty);
        stream << '"' << "summary" << '"' << ':' << (pretty ? " {" : "{");
        write_summary(stream, capture.summary, 2, pretty);
        emit_indentation(stream, 1, pretty);
        stream << (pretty ? " }," : "},");

        emit_indentation(stream, 1, pretty);
        stream << '"' << "frame_jitter_ms" << '"' << ':' << capture.summary.stddev_ms << ',';
        emit_indentation(stream, 1, pretty);
        stream << '"' << "frame_jitter_budget_ms" << '"' << ':' << options.jitter_budget_ms << ',';
        emit_indentation(stream, 1, pretty);
        stream << '"' << "frame_jitter_exceeds_budget" << '"' << ':' << (jitter_exceeded ? "true" : "false") << ',';

        write_frames(stream, capture.frames, options.timestep, 1, pretty);

        emit_indentation(stream, 0, pretty);
        stream << '}';
        if (pretty)
        {
            stream << '\n';
        }
    }
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const CommandLineOptions options = parse_command_line(argc, argv);
        if (options.show_help)
        {
            print_help();
            return 0;
        }

        if (options.frames == 0U)
        {
            throw std::invalid_argument{"--frames must be greater than zero"};
        }
        if (!(options.timestep > 0.0))
        {
            throw std::invalid_argument{"--dt must be greater than zero"};
        }

        const animation::AnimationClip clip = load_clip(options);
        if (clip.tracks.empty())
        {
            throw std::runtime_error{"Animation clip contains no tracks"};
        }

        const CaptureResult capture = options.scenario == Scenario::GpuAsync
                                           ? capture_gpu_frames(clip, options.frames, options.timestep)
                                           : capture_cpu_frames(clip, options.frames, options.timestep);
        const double fps = capture.summary.mean_ms > 0.0 ? 1000.0 / capture.summary.mean_ms : 0.0;

        std::cout << "Animation Benchmark" << '\n';
        std::cout << "Scenario: " << to_string(options.scenario) << '\n';
        std::cout << "Clip: " << (clip.name.empty() ? "<unnamed>" : clip.name) << '\n';
        std::cout << "Tracks: " << clip.tracks.size() << '\n';
        std::cout << "Frames sampled: " << capture.summary.samples << '\n';
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Average sample time: " << capture.summary.mean_ms << " ms" << '\n';
        std::cout << "Minimum sample time: " << capture.summary.min_ms << " ms" << '\n';
        std::cout << "Maximum sample time: " << capture.summary.max_ms << " ms" << '\n';
        std::cout << "Frame jitter σ: " << capture.summary.stddev_ms << " ms" << '\n';
        std::cout << "Approximate throughput: " << fps << " FPS" << '\n';

        if (options.output_path)
        {
            if (options.output_path->has_parent_path())
            {
                std::filesystem::create_directories(options.output_path->parent_path());
            }
            std::ofstream file(*options.output_path, std::ios::binary);
            if (!file)
            {
                std::ostringstream stream;
                stream << "Failed to open output file: " << options.output_path->string();
                throw std::runtime_error(stream.str());
            }
            write_json(file, options, clip, capture);
            std::cout << "Telemetry written to " << options.output_path->string() << '\n';
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
