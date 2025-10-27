#include "engine/runtime/api.hpp"

#include "queue_assignment.hpp"
#include "workload_configuration.hpp"

#include "engine/animation/api.hpp"
#include "engine/animation/rigging/rig_binding.hpp"
#include "engine/geometry/api.hpp"
#include "engine/physics/api.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace animation = engine::animation;
namespace compute = engine::compute;
namespace geometry = engine::geometry;
namespace math = engine::math;
namespace physics = engine::physics;
namespace runtime = engine::runtime;

namespace samples = engine::compute::samples;

namespace
{
    constexpr double kBaselineSpeedupTarget = 1.5;
    constexpr std::size_t kMemoryBudgetBytes = 256ULL * 1024ULL * 1024ULL;
    constexpr double kBytesPerMebibyte = 1024.0 * 1024.0;
    constexpr double kDefaultFrameJitterBudgetMs = 0.5;

    using samples::WorkloadProfile;

    enum class DispatcherBackend
    {
        Cpu,
        Cuda,
    };

    [[nodiscard]] std::string_view dispatcher_backend_to_string(DispatcherBackend backend) noexcept
    {
        switch (backend)
        {
        case DispatcherBackend::Cuda:
            return "cuda";
        case DispatcherBackend::Cpu:
        default:
            return "cpu";
        }
    }

    [[nodiscard]] DispatcherBackend parse_dispatcher_backend(std::string_view value)
    {
        std::string lowered;
        lowered.reserve(value.size());
        for (const unsigned char ch : value)
        {
            lowered.push_back(static_cast<char>(std::tolower(ch)));
        }

        if (lowered == "cpu")
        {
            return DispatcherBackend::Cpu;
        }
        if (lowered == "cuda" || lowered == "gpu")
        {
            return DispatcherBackend::Cuda;
        }

        std::ostringstream stream;
        stream << "Unknown dispatcher backend: " << value;
        throw std::invalid_argument(stream.str());
    }

    [[nodiscard]] samples::DispatcherFactory make_dispatcher_factory(DispatcherBackend backend)
    {
        switch (backend)
        {
        case DispatcherBackend::Cuda:
            return []() { return compute::make_cuda_dispatcher(); };
        case DispatcherBackend::Cpu:
        default:
            return []() { return compute::make_cpu_dispatcher(); };
        }
    }

    struct CommandLineOptions
    {
        bool show_help{false};
        bool pretty_json{false};
        std::size_t frames{1024U};
        std::size_t repeat_count{1U};
        double timestep{1.0 / 60.0};
        std::size_t queue_count{1U};
        WorkloadProfile workload{WorkloadProfile::Balanced};
        DispatcherBackend dispatcher_backend{DispatcherBackend::Cpu};
        std::vector<std::string> queue_names{};
        std::map<std::string, std::string> queue_overrides{};
        std::optional<std::filesystem::path> output_directory{};
        std::optional<std::filesystem::path> output_path{};
        bool include_baseline{false};
        double jitter_budget_ms{kDefaultFrameJitterBudgetMs};
    };

    [[nodiscard]] WorkloadProfile parse_workload(std::string_view value)
    {
        std::string lowered;
        lowered.reserve(value.size());
        for (const unsigned char ch : value)
        {
            lowered.push_back(static_cast<char>(std::tolower(ch)));
        }

        if (lowered == "light")
        {
            return WorkloadProfile::Light;
        }
        if (lowered == "balanced" || lowered == "default")
        {
            return WorkloadProfile::Balanced;
        }
        if (lowered == "heavy")
        {
            return WorkloadProfile::Heavy;
        }

        std::ostringstream stream;
        stream << "Unknown workload profile: " << value;
        throw std::invalid_argument(stream.str());
    }

    [[nodiscard]] std::string to_lower_ascii(std::string_view text)
    {
        std::string lowered;
        lowered.reserve(text.size());
        for (const unsigned char ch : text)
        {
            lowered.push_back(static_cast<char>(std::tolower(ch)));
        }
        return lowered;
    }

    [[nodiscard]] std::string trim_copy(std::string_view text)
    {
        std::size_t start = 0U;
        std::size_t end = text.size();
        while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0)
        {
            ++start;
        }
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1U])) != 0)
        {
            --end;
        }
        return std::string{text.substr(start, end - start)};
    }

    [[nodiscard]] std::vector<std::string> parse_queue_names(std::string_view value)
    {
        std::vector<std::string> names{};
        std::size_t offset = 0U;
        while (offset <= value.size())
        {
            const std::size_t delimiter = value.find(',', offset);
            const std::size_t length = delimiter == std::string_view::npos ? value.size() - offset : delimiter - offset;
            const std::string name = trim_copy(value.substr(offset, length));
            if (!name.empty())
            {
                names.push_back(name);
            }
            if (delimiter == std::string_view::npos)
            {
                break;
            }
            offset = delimiter + 1U;
        }
        return names;
    }

    using samples::WorkloadProfileDefinition;

    [[nodiscard]] std::string make_queue_name(std::size_t index)
    {
        std::ostringstream stream;
        stream << "queue-" << index;
        return stream.str();
    }

    [[nodiscard]] std::size_t assign_queue(
        std::string_view category,
        std::size_t queue_count,
        const std::unordered_map<std::string, std::size_t>& overrides)
    {
        if (queue_count <= 1U)
        {
            return 0U;
        }

        const std::string lowered = to_lower_ascii(category);

        if (const auto override = overrides.find(lowered); override != overrides.end())
        {
            return override->second;
        }

        if (lowered == "animation")
        {
            return 0U;
        }
        if (lowered == "physics")
        {
            return std::min<std::size_t>(1U, queue_count - 1U);
        }
        if (lowered == "geometry")
        {
            if (queue_count <= 2U)
            {
                return queue_count - 1U;
            }
            return 2U;
        }

        return engine::compute::samples::deterministic_queue_index(lowered, queue_count);
    }

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
            << "  engine_compute_runtime_sample [--frames N] [--dt SECONDS] [--output FILE]\\\n"
            << "      [--queues COUNT] [--queue-names LIST] [--queue-map category=queue]\\\n"
            << "      [--workload PROFILE] [--repeat COUNT] [--output-dir DIR] [--pretty]\n"
            << '\n'
            << "Options:\n"
            << "  --frames N   Number of ticks to execute (default: 1024)\n"
            << "  --dt SECONDS Simulation timestep for each tick (default: 1/60)\n"
            << "  --repeat COUNT Execute COUNT captures sequentially (default: 1)\n"
            << "  --queues N   Number of logical compute queues recorded in telemetry (default: 1)\n"
            << "  --queue-names LIST   Comma-separated queue names (implies --queues=LIST length)\n"
            << "  --queue-map category=queue   Override queue selection for a category\n"
            << "  --workload PROFILE  Workload intensity: light | balanced | heavy (default: balanced)\n"
            << "  --dispatcher-backend BACKEND  Dispatcher backend: cpu | cuda (default: cpu)\n"
            << "  --jitter-budget-ms VALUE  Maximum allowed frame dispatch jitter σ in milliseconds (default: 0.5)\n"
            << "  --baseline   Capture a single-queue baseline run and report speed-up (target 1.5x)\n"
            << "  --output FILE Write JSON payload to FILE instead of stdout\n"
            << "  --output-dir DIR Directory for JSON payloads when repeating (default: parent of --output)\n"
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
        const unsigned long long parsed = std::strtoull(buffer.c_str(), &end, 10); // NOLINT
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
            if (argument == "--baseline")
            {
                options.include_baseline = true;
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
            if (argument == "--repeat" || argument == "--runs")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--repeat requires a value"};
                }
                options.repeat_count = parse_size(argv[++index]);
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
            if (argument == "--output-dir")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--output-dir requires a path"};
                }
                options.output_directory = std::filesystem::path{argv[++index]};
                continue;
            }
            if (argument == "--queues" || argument == "--queue-count")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--queues requires a value"};
                }
                options.queue_count = parse_size(argv[++index]);
                continue;
            }
            if (argument == "--queue-names")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--queue-names requires a comma-separated list"};
                }
                options.queue_names = parse_queue_names(argv[++index]);
                if (options.queue_names.empty())
                {
                    throw std::invalid_argument{"--queue-names requires at least one entry"};
                }
                continue;
            }
            if (argument == "--queue-map")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--queue-map requires category=queue"};
                }
                const std::string_view mapping{argv[++index]};
                const auto separator = mapping.find('=');
                if (separator == std::string_view::npos)
                {
                    throw std::invalid_argument{"--queue-map expects category=queue"};
                }
                const std::string category = to_lower_ascii(trim_copy(mapping.substr(0, separator)));
                const std::string queue = trim_copy(mapping.substr(separator + 1));
                if (category.empty() || queue.empty())
                {
                    throw std::invalid_argument{"--queue-map entries cannot be empty"};
                }
                options.queue_overrides[category] = queue;
                continue;
            }
            if (argument == "--workload")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--workload requires a profile"};
                }
                options.workload = parse_workload(argv[++index]);
                continue;
            }
            if (argument == "--dispatcher-backend" || argument == "--backend")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--dispatcher-backend requires a value"};
                }
                options.dispatcher_backend = parse_dispatcher_backend(argv[++index]);
                continue;
            }
            if (argument == "--jitter-budget-ms")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--jitter-budget-ms requires a value"};
                }
                options.jitter_budget_ms = parse_double(argv[++index]);
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
        if (options.repeat_count == 0U)
        {
            throw std::invalid_argument{"--repeat must be greater than zero"};
        }
        if (!(options.timestep > 0.0))
        {
            throw std::invalid_argument{"--dt must be positive"};
        }
        if (options.queue_count == 0U)
        {
            throw std::invalid_argument{"--queues must be greater than zero"};
        }
        if (!(options.jitter_budget_ms >= 0.0))
        {
            throw std::invalid_argument{"--jitter-budget-ms must be non-negative"};
        }

        if (options.output_directory && options.output_directory->empty())
        {
            throw std::invalid_argument{"--output-dir cannot be empty"};
        }

        if (options.dispatcher_backend == DispatcherBackend::Cuda && !compute::is_cuda_dispatcher_available())
        {
            throw std::invalid_argument("CUDA dispatcher backend requested but not available in this build");
        }

        if (!options.queue_names.empty())
        {
            if (options.queue_count == 1U)
            {
                options.queue_count = options.queue_names.size();
            }
            else if (options.queue_names.size() != options.queue_count)
            {
                throw std::invalid_argument{"--queue-names size must match --queues"};
            }

            std::set<std::string> unique(options.queue_names.begin(), options.queue_names.end());
            if (unique.size() != options.queue_names.size())
            {
                throw std::invalid_argument{"--queue-names entries must be unique"};
            }
        }

        std::vector<std::string> known_queues = options.queue_names;
        if (known_queues.empty())
        {
            known_queues.reserve(options.queue_count);
            for (std::size_t index = 0; index < options.queue_count; ++index)
            {
                known_queues.push_back(make_queue_name(index));
            }
        }

        std::set<std::string> known_queue_set(known_queues.begin(), known_queues.end());
        for (const auto& [category, queue] : options.queue_overrides)
        {
            if (!known_queue_set.contains(queue))
            {
                std::ostringstream stream;
                stream << "--queue-map references unknown queue '" << queue << "'";
                throw std::invalid_argument(stream.str());
            }
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

    [[nodiscard]] std::size_t digit_count(std::size_t value) noexcept
    {
        std::size_t digits = 0U;
        do
        {
            value /= 10U;
            ++digits;
        }
        while (value > 0U);
        return digits;
    }

    [[nodiscard]] std::optional<std::filesystem::path> make_output_path(
        const CommandLineOptions& options,
        std::size_t run_index)
    {
        if (!options.output_path && !options.output_directory)
        {
            return std::nullopt;
        }

        std::filesystem::path directory{};
        std::string prefix{};
        std::string extension{};

        if (options.output_directory)
        {
            directory = *options.output_directory;
            if (options.output_path)
            {
                prefix = options.output_path->stem().string();
                extension = options.output_path->extension().string();
            }
            else
            {
                prefix = "compute_dispatch";
                extension = ".json";
            }
        }
        else if (options.output_path)
        {
            directory = options.output_path->parent_path();
            prefix = options.output_path->stem().string();
            extension = options.output_path->extension().string();
        }

        if (extension.empty())
        {
            extension = ".json";
        }
        if (prefix.empty())
        {
            prefix = "compute_dispatch";
        }

        if (!options.output_directory && options.repeat_count <= 1U)
        {
            return options.output_path;
        }

        const std::size_t digits = digit_count(options.repeat_count);
        std::ostringstream filename;
        filename << prefix << "-run" << std::setw(static_cast<int>(digits)) << std::setfill('0')
            << (run_index + 1U);
        if (!extension.empty())
        {
            filename << extension;
        }

        std::filesystem::path resolved = directory / filename.str();
        return resolved;
    }

    struct DispatchSample
    {
        std::string name{};
        std::string category{};
        double duration_ms{0.0};
        std::string queue{};
    };

    struct FrameSample
    {
        std::size_t index{0U};
        double simulation_time{0.0};
        double timestep{0.0};
        double total_ms{0.0};
        std::vector<DispatchSample> dispatches{};
        std::map<std::string, double> category_totals{};
        std::map<std::string, double> queue_totals{};
    };

    struct QueueTransition
    {
        std::string producer{};
        std::string consumer{};
        std::string from_queue{};
        std::string to_queue{};
    };

    struct QueueDependencySummary
    {
        std::string from_queue{};
        std::string to_queue{};
        std::size_t edge_count{0U};
        std::vector<std::string> consumer_kernels{};
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

    struct MemoryUsageSummary
    {
        std::size_t vertex_count{0U};
        std::size_t joint_count{0U};
        std::size_t position_bytes{0U};
        std::size_t normal_bytes{0U};
        std::size_t transform_bytes{0U};
        std::size_t total_bytes{0U};

        [[nodiscard]] double total_mebibytes() const noexcept
        {
            return static_cast<double>(total_bytes) / kBytesPerMebibyte;
        }

        [[nodiscard]] double budget_mebibytes() const noexcept
        {
            return static_cast<double>(kMemoryBudgetBytes) / kBytesPerMebibyte;
        }

        [[nodiscard]] bool exceeds_budget() const noexcept
        {
            return total_bytes > kMemoryBudgetBytes;
        }
    };

    struct RunSummary
    {
        std::map<std::string, SummaryStats> dispatches{};
        std::map<std::string, SummaryStats> categories{};
        std::map<std::string, SummaryStats> queues{};
        SummaryStats frame_totals{};
        std::vector<QueueTransition> queue_transitions{};
        std::vector<QueueDependencySummary> queue_dependencies{};
        MemoryUsageSummary memory{};
    };

    struct RunResult
    {
        std::vector<FrameSample> frames{};
        RunSummary summary{};
        std::string clock_name{};
        compute::TimingDomain clock_domain{compute::TimingDomain::Unknown};
        WorkloadProfile workload{WorkloadProfile::Balanced};
        DispatcherBackend dispatcher_backend{DispatcherBackend::Cpu};
        std::vector<std::string> queue_names{};
        std::size_t requested_frames{0U};
        std::map<std::string, std::string> queue_assignments{};
        compute::DependencyGraph dependency_graph{};
        MemoryUsageSummary memory{};
        double jitter_budget_ms{kDefaultFrameJitterBudgetMs};
    };

    [[nodiscard]] MemoryUsageSummary compute_memory_usage(const runtime::RuntimeHost& host)
    {
        MemoryUsageSummary summary{};
        const geometry::SurfaceMesh& mesh = host.current_mesh();
        summary.vertex_count = mesh.positions.size();
        summary.position_bytes = summary.vertex_count * sizeof(math::vec3);
        summary.normal_bytes = mesh.normals.size() * sizeof(math::vec3);

        const animation::AnimationRigPose& pose = host.current_pose();
        summary.joint_count = pose.joints.size();
        constexpr std::size_t kMatrixBytes = sizeof(float) * 16U;
        summary.transform_bytes = summary.joint_count * kMatrixBytes;

        summary.total_bytes = summary.position_bytes + summary.normal_bytes + summary.transform_bytes;
        return summary;
    }

    struct ExecutionOutcome
    {
        RunResult result{};
        runtime::RuntimeDiagnostics diagnostics{};
    };

    [[nodiscard]] double compute_speedup(double baseline_ms, double optimized_ms) noexcept
    {
        if (!(baseline_ms > 0.0) || !(optimized_ms > 0.0))
        {
            return 0.0;
        }
        return baseline_ms / optimized_ms;
    }

    [[nodiscard]] std::vector<QueueTransition> collect_queue_transitions(
        const compute::DependencyGraph& graph,
        const std::unordered_map<std::string, std::string>& queue_lookup)
    {
        std::vector<QueueTransition> transitions{};
        for (const auto& node : graph.nodes)
        {
            const auto consumer_queue = queue_lookup.find(node.name);
            if (consumer_queue == queue_lookup.end())
            {
                continue;
            }

            for (const auto dependency : node.dependencies)
            {
                if (dependency >= graph.nodes.size())
                {
                    continue;
                }

                const auto& producer_node = graph.nodes[dependency];
                const auto producer_queue = queue_lookup.find(producer_node.name);
                if (producer_queue == queue_lookup.end())
                {
                    continue;
                }

                if (producer_queue->second == consumer_queue->second)
                {
                    continue;
                }

                transitions.push_back(QueueTransition{
                    .producer = producer_node.name,
                    .consumer = node.name,
                    .from_queue = producer_queue->second,
                    .to_queue = consumer_queue->second,
                });
            }
        }

        return transitions;
    }

    [[nodiscard]] std::vector<QueueDependencySummary> summarize_queue_dependencies(
        const std::vector<QueueTransition>& transitions)
    {
        std::map<std::pair<std::string, std::string>, QueueDependencySummary> aggregates{};
        for (const auto& transition : transitions)
        {
            const auto key = std::make_pair(transition.from_queue, transition.to_queue);
            auto& entry = aggregates[key];
            entry.from_queue = transition.from_queue;
            entry.to_queue = transition.to_queue;
            entry.edge_count += 1U;
            if (!transition.consumer.empty())
            {
                if (std::find(entry.consumer_kernels.begin(), entry.consumer_kernels.end(), transition.consumer)
                    == entry.consumer_kernels.end())
                {
                    entry.consumer_kernels.push_back(transition.consumer);
                }
            }
        }

        std::vector<QueueDependencySummary> summaries{};
        summaries.reserve(aggregates.size());
        for (auto& [_, summary] : aggregates)
        {
            std::sort(summary.consumer_kernels.begin(), summary.consumer_kernels.end());
            summaries.push_back(std::move(summary));
        }
        std::sort(
            summaries.begin(),
            summaries.end(),
            [](const QueueDependencySummary& lhs, const QueueDependencySummary& rhs)
            {
                if (lhs.edge_count == rhs.edge_count)
                {
                    if (lhs.from_queue == rhs.from_queue)
                    {
                        return lhs.to_queue < rhs.to_queue;
                    }
                    return lhs.from_queue < rhs.from_queue;
                }
                return lhs.edge_count > rhs.edge_count;
            });
        return summaries;
    }

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
        result.workload = options.workload;
        result.dispatcher_backend = options.dispatcher_backend;
        result.requested_frames = options.frames;
        result.jitter_budget_ms = options.jitter_budget_ms;
        const std::size_t queue_count = std::max<std::size_t>(1U, options.queue_count);
        if (!options.queue_names.empty())
        {
            result.queue_names = options.queue_names;
        }
        else
        {
            result.queue_names.reserve(queue_count);
            for (std::size_t index = 0; index < queue_count; ++index)
            {
                result.queue_names.push_back(make_queue_name(index));
            }
        }

        std::unordered_map<std::string, std::size_t> queue_lookup{};
        queue_lookup.reserve(result.queue_names.size());
        for (std::size_t index = 0; index < result.queue_names.size(); ++index)
        {
            queue_lookup.emplace(result.queue_names[index], index);
        }

        std::unordered_map<std::string, std::size_t> override_indices{};
        override_indices.reserve(options.queue_overrides.size());
        for (const auto& [category_lowered, queue_name] : options.queue_overrides)
        {
            const auto lookup = queue_lookup.find(queue_name);
            if (lookup == queue_lookup.end())
            {
                std::ostringstream stream;
                stream << "Queue override references unknown queue: " << queue_name;
                throw std::invalid_argument(stream.str());
            }
            override_indices.emplace(category_lowered, lookup->second);
        }

        std::map<std::string, std::vector<double>> durations_by_dispatch{};
        std::map<std::string, std::vector<double>> durations_by_category{};
        std::map<std::string, std::vector<double>> durations_by_queue{};
        std::unordered_map<std::string, std::string> queue_by_kernel{};
        compute::DependencyGraph last_graph{};
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
            last_graph = report.dependency_graph;

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
                const std::size_t queue_index = assign_queue(category, queue_count, override_indices);
                const std::string& queue_name = result.queue_names[queue_index];

                DispatchSample dispatch_sample{
                    .name = name,
                    .category = category,
                    .duration_ms = duration_ms,
                    .queue = queue_name,
                };
                frame.dispatches.push_back(dispatch_sample);
                queue_by_kernel.insert_or_assign(name, queue_name);

                frame.category_totals[category] += duration_ms;
                frame.queue_totals[queue_name] += duration_ms;
                frame.total_ms += duration_ms;
                durations_by_dispatch[name].push_back(duration_ms);
                durations_by_category[category].push_back(duration_ms);
                durations_by_queue[queue_name].push_back(duration_ms);
                result.queue_assignments[category] = queue_name;
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
        for (auto& [queue_name, values] : durations_by_queue)
        {
            result.summary.queues[queue_name] = compute_summary_stats(values);
        }

        result.dependency_graph = std::move(last_graph);
        const auto transitions = collect_queue_transitions(result.dependency_graph, queue_by_kernel);
        result.summary.queue_transitions = transitions;
        result.summary.queue_dependencies = summarize_queue_dependencies(transitions);

        result.summary.memory = compute_memory_usage(host);
        result.memory = result.summary.memory;

        return result;
    }

    [[nodiscard]] ExecutionOutcome execute_run(const CommandLineOptions& options)
    {
        runtime::RuntimeHost host{};
        samples::configure_runtime_host(
            host,
            options.workload,
            make_dispatcher_factory(options.dispatcher_backend));
        host.initialize();

        RunResult result = run_sample(options, host);
        const runtime::RuntimeDiagnostics diagnostics = host.diagnostics();
        host.shutdown();

        ExecutionOutcome outcome{};
        outcome.result = std::move(result);
        outcome.diagnostics = diagnostics;
        return outcome;
    }

    void print_text_summary(
        const RunResult& result,
        const RunResult* baseline,
        double speedup,
        std::size_t run_index,
        std::size_t run_count)
    {
        std::cout << "=== Compute Dispatcher Runtime Sample";
        if (run_count > 1U)
        {
            std::cout << " (run " << (run_index + 1U) << " of " << run_count << ")";
        }
        std::cout << " ===\n";
        std::cout << "Frames: " << result.summary.frame_totals.samples << '\n';
        std::cout << "Requested frames: " << result.requested_frames << '\n';
        std::cout << "Average frame dispatch time: " << std::fixed << std::setprecision(3)
            << result.summary.frame_totals.mean_ms << " ms\n";
        std::cout << "Clock: " << result.clock_name << " ("
            << timing_domain_to_string(result.clock_domain) << ")\n";
        std::cout << "Workload: " << samples::workload_to_string(result.workload) << '\n';
        std::cout << "Dispatcher backend: " << dispatcher_backend_to_string(result.dispatcher_backend) << '\n';
        std::cout << "Queues: " << result.queue_names.size();
        if (!result.queue_names.empty())
        {
            std::cout << " (";
            for (std::size_t index = 0; index < result.queue_names.size(); ++index)
            {
                if (index > 0U)
                {
                    std::cout << ", ";
                }
                std::cout << result.queue_names[index];
            }
            std::cout << ")";
        }
        std::cout << '\n';

        if (!result.queue_assignments.empty())
        {
            std::cout << "Queue assignments:" << '\n';
            for (const auto& [category, queue_name] : result.queue_assignments)
            {
                std::cout << "  - " << category << " -> " << queue_name << '\n';
            }
        }

        const MemoryUsageSummary& memory = result.summary.memory;
        std::cout << '\n' << "GPU staging estimate:" << '\n';
        std::cout << "  - Total: " << std::fixed << std::setprecision(3)
            << memory.total_mebibytes() << " MiB (budget "
            << std::setprecision(3) << memory.budget_mebibytes() << " MiB)";
        if (memory.exceeds_budget())
        {
            std::cout << " ⚠";
        }
        std::cout << '\n';
        std::cout << "  - Positions: " << std::setprecision(3)
            << static_cast<double>(memory.position_bytes) / kBytesPerMebibyte
            << " MiB across " << memory.vertex_count << " vertices\n";
        std::cout << "  - Normals: " << std::setprecision(3)
            << static_cast<double>(memory.normal_bytes) / kBytesPerMebibyte
            << " MiB\n";
        std::cout << "  - Skinning transforms: " << std::setprecision(3)
            << static_cast<double>(memory.transform_bytes) / kBytesPerMebibyte
            << " MiB across " << memory.joint_count << " joints\n";
        if (memory.exceeds_budget())
        {
            std::cout << "  WARNING: Estimated GPU staging exceeds the 256 MiB budget." << '\n';
        }

        std::vector<std::pair<std::string, SummaryStats>> top_dispatches{};
        top_dispatches.reserve(result.summary.dispatches.size());
        for (const auto& [name, stats] : result.summary.dispatches)
        {
            top_dispatches.emplace_back(name, stats);
        }
        std::sort(top_dispatches.begin(), top_dispatches.end(), [](const auto& lhs, const auto& rhs)
        {
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

        if (!result.summary.queues.empty())
        {
            std::cout << '\n' << "Queue totals:" << '\n';
            for (const auto& [queue_name, stats] : result.summary.queues)
            {
                std::cout << "  - " << queue_name << ": total " << std::fixed << std::setprecision(3)
                    << stats.total_ms << " ms across " << stats.samples << " samples\n";
            }
        }

        const double frame_jitter_ms = result.summary.frame_totals.stddev_ms;
        std::cout << '\n' << "Frame dispatch jitter σ: " << std::fixed << std::setprecision(3) << frame_jitter_ms
            << " ms (budget " << std::setprecision(3) << result.jitter_budget_ms << " ms)";
        const bool frame_jitter_exceeded = frame_jitter_ms > result.jitter_budget_ms && result.jitter_budget_ms >= 0.0;
        if (frame_jitter_exceeded)
        {
            std::cout << " ⚠";
        }
        std::cout << '\n';
        if (frame_jitter_exceeded)
        {
            std::cout << "  WARNING: Frame dispatch jitter exceeds the configured budget." << '\n';
        }

        if (!result.summary.queue_dependencies.empty())
        {
            std::cout << '\n' << "Cross-queue synchronization (fences):" << '\n';
            for (const auto& entry : result.summary.queue_dependencies)
            {
                std::cout << "  - " << entry.from_queue << " -> " << entry.to_queue << ": " << entry.edge_count
                    << " dependencies";
                if (!entry.consumer_kernels.empty())
                {
                    std::cout << " (consumers: ";
                    for (std::size_t index = 0; index < entry.consumer_kernels.size(); ++index)
                    {
                        if (index > 0U)
                        {
                            std::cout << ", ";
                        }
                        std::cout << entry.consumer_kernels[index];
                    }
                    std::cout << ')';
                }
                std::cout << '\n';
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

        if (baseline != nullptr)
        {
            const SummaryStats& stats = baseline->summary.frame_totals;
            std::cout << '\n' << "Baseline (" << baseline->queue_names.size() << " queue";
            if (baseline->queue_names.size() != 1U)
            {
                std::cout << 's';
            }
            std::cout << ") average frame dispatch: " << std::fixed << std::setprecision(3)
                << stats.mean_ms << " ms";
            const double jitter = stats.jitter_percent();
            std::cout << " (min " << std::setprecision(3) << stats.min_ms << ", max " << stats.max_ms
                << ", σ " << stats.stddev_ms << ", jitter " << std::setprecision(2) << jitter << "%)\n";
            std::cout << "Speed-up vs baseline: " << std::fixed << std::setprecision(3) << speedup << 'x';
            if (speedup < kBaselineSpeedupTarget)
            {
                std::cout << " (below " << std::setprecision(2) << kBaselineSpeedupTarget << "x target)";
            }
            std::cout << '\n';
            const bool baseline_jitter_exceeded = result.jitter_budget_ms >= 0.0 && stats.stddev_ms > result.
                jitter_budget_ms;
            if (frame_jitter_exceeded || baseline_jitter_exceeded)
            {
                std::cout << "Baseline jitter σ: " << std::fixed << std::setprecision(3) << stats.stddev_ms
                    << " ms";
                if (result.jitter_budget_ms >= 0.0)
                {
                    std::cout << " (budget " << std::setprecision(3) << result.jitter_budget_ms << " ms)";
                    if (baseline_jitter_exceeded)
                    {
                        std::cout << " ⚠";
                    }
                }
                std::cout << '\n';
                if (baseline_jitter_exceeded)
                {
                    std::cout << "  WARNING: Baseline frame dispatch jitter exceeds the configured budget." << '\n';
                }
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

        auto emit_field = [&](std::string_view key, double value, bool last)
        {
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
                           const RunResult* baseline,
                           double baseline_speedup,
                           std::ostream& stream,
                           std::size_t run_index,
                           std::size_t run_count)
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
        stream << '"' << "run_index" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << (run_index + 1U) << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "run_count" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << run_count << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "requested_frames" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << result.requested_frames << ',';
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
        stream << '"' << "workload" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << '"' << samples::workload_to_string(result.workload) << '"' << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "dispatcher_backend" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << '"' << dispatcher_backend_to_string(result.dispatcher_backend) << '"' << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "queue_count" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << result.queue_names.size() << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "queues" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        for (std::size_t queue_index = 0; queue_index < result.queue_names.size(); ++queue_index)
        {
            write_indent(stream, 3U, pretty);
            stream << '"' << escape_json(result.queue_names[queue_index]) << '"';
            if (queue_index + 1U < result.queue_names.size())
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
        write_indent(stream, 2U, pretty);
        stream << '"' << "queue_assignments" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        std::size_t assignment_index = 0U;
        for (const auto& [category, queue_name] : result.queue_assignments)
        {
            write_indent(stream, 3U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
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
            write_indent(stream, 4U, pretty);
            stream << '"' << "queue" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(queue_name) << '"';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << "}";
            if (++assignment_index < result.queue_assignments.size())
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
        const double frame_jitter_ms = result.summary.frame_totals.stddev_ms;
        const bool frame_jitter_exceeded = frame_jitter_ms > result.jitter_budget_ms && result.jitter_budget_ms >= 0.0;
        write_indent(stream, 2U, pretty);
        stream << '"' << "frame_jitter_ms" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << frame_jitter_ms << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "frame_jitter_budget_ms" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << result.jitter_budget_ms << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << '"' << "frame_jitter_exceeds_budget" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << (frame_jitter_exceeded ? "true" : "false") << ',';
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

        if (baseline != nullptr)
        {
            const SummaryStats& stats = baseline->summary.frame_totals;
            write_indent(stream, 1U, pretty);
            stream << '"' << "baseline" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }

            auto emit_number = [&](std::string_view key, double value, bool last)
            {
                write_indent(stream, 2U, pretty);
                stream << '"' << key << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << value;
                if (!last)
                {
                    stream << ',';
                }
                if (pretty)
                {
                    stream << '\n';
                }
            };
            auto emit_size = [&](std::string_view key, std::size_t value, bool last)
            {
                write_indent(stream, 2U, pretty);
                stream << '"' << key << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << value;
                if (!last)
                {
                    stream << ',';
                }
                if (pretty)
                {
                    stream << '\n';
                }
            };
            auto emit_bool = [&](std::string_view key, bool value, bool last)
            {
                write_indent(stream, 2U, pretty);
                stream << '"' << key << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << (value ? "true" : "false");
                if (!last)
                {
                    stream << ',';
                }
                if (pretty)
                {
                    stream << '\n';
                }
            };

            emit_size("frames", stats.samples, false);
            emit_size("queue_count", baseline->queue_names.size(), false);

            write_indent(stream, 2U, pretty);
            stream << '"' << "queue_names" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << "[";
            if (pretty)
            {
                stream << '\n';
            }
            for (std::size_t index = 0; index < baseline->queue_names.size(); ++index)
            {
                write_indent(stream, 3U, pretty);
                stream << '"' << escape_json(baseline->queue_names[index]) << '"';
                if (index + 1U < baseline->queue_names.size())
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

            write_indent(stream, 2U, pretty);
            stream << '"' << "dispatcher_backend" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << dispatcher_backend_to_string(baseline->dispatcher_backend) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }

            const bool baseline_jitter_exceeded = stats.stddev_ms > result.jitter_budget_ms && result.jitter_budget_ms
                >= 0.0;
            emit_number("average_frame_ms", stats.mean_ms, false);
            emit_number("min_frame_ms", stats.min_ms, false);
            emit_number("max_frame_ms", stats.max_ms, false);
            emit_number("stddev_frame_ms", stats.stddev_ms, false);
            emit_number("jitter_percent", stats.jitter_percent(), false);
            emit_number("jitter_budget_ms", result.jitter_budget_ms, false);
            emit_bool("jitter_exceeds_budget", baseline_jitter_exceeded, false);
            emit_number("speedup", baseline_speedup, false);
            emit_number("target_speedup", kBaselineSpeedupTarget, false);
            emit_size("memory_total_bytes", baseline->summary.memory.total_bytes, false);
            emit_number("memory_total_mebibytes", baseline->summary.memory.total_mebibytes(), false);
            emit_number("memory_budget_mebibytes", baseline->summary.memory.budget_mebibytes(), false);
            emit_bool("memory_exceeds_budget", baseline->summary.memory.exceeds_budget(), true);

            write_indent(stream, 1U, pretty);
            stream << "},";
            if (pretty)
            {
                stream << '\n';
            }
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
                stream << dispatch.duration_ms << ',';
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "queue" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << '"' << escape_json(dispatch.queue) << '"' << '\n';
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
            stream << "],";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << '"' << "queue_totals_ms" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << "[";
            if (pretty)
            {
                stream << '\n';
            }
            std::size_t queue_total_index = 0;
            for (const auto& [queue_name, total] : frame.queue_totals)
            {
                write_indent(stream, 4U, pretty);
                stream << "{";
                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, 5U, pretty);
                stream << '"' << "queue" << '"' << ':';
                if (pretty)
                {
                    stream << ' ';
                }
                stream << '"' << escape_json(queue_name) << '"' << ',';
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
                if (++queue_total_index < frame.queue_totals.size())
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

        // Queue summary
        write_indent(stream, 2U, pretty);
        stream << '"' << "queues" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        std::size_t summary_queue_index = 0;
        for (const auto& [queue_name, stats] : result.summary.queues)
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
            stream << '"' << escape_json(queue_name) << '"' << ',';
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
            if (++summary_queue_index < result.summary.queues.size())
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

        write_indent(stream, 2U, pretty);
        stream << '"' << "queue_dependencies" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        for (std::size_t index = 0; index < result.summary.queue_dependencies.size(); ++index)
        {
            const auto& dependency = result.summary.queue_dependencies[index];
            write_indent(stream, 3U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "from_queue" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(dependency.from_queue) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "to_queue" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(dependency.to_queue) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "edge_count" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << dependency.edge_count << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "consumer_kernels" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << "[";
            if (pretty)
            {
                stream << '\n';
            }
            for (std::size_t kernel_index = 0; kernel_index < dependency.consumer_kernels.size(); ++kernel_index)
            {
                write_indent(stream, 5U, pretty);
                stream << '"' << escape_json(dependency.consumer_kernels[kernel_index]) << '"';
                if (kernel_index + 1U < dependency.consumer_kernels.size())
                {
                    stream << ',';
                }
                if (pretty)
                {
                    stream << '\n';
                }
            }
            write_indent(stream, 4U, pretty);
            stream << "]";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << "}";
            if (index + 1U < result.summary.queue_dependencies.size())
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

        write_indent(stream, 2U, pretty);
        stream << '"' << "queue_transitions" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << "[";
        if (pretty)
        {
            stream << '\n';
        }
        for (std::size_t index = 0; index < result.summary.queue_transitions.size(); ++index)
        {
            const auto& transition = result.summary.queue_transitions[index];
            write_indent(stream, 3U, pretty);
            stream << "{";
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "producer" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(transition.producer) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "consumer" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(transition.consumer) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "from_queue" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(transition.from_queue) << '"' << ',';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 4U, pretty);
            stream << '"' << "to_queue" << '"' << ':';
            if (pretty)
            {
                stream << ' ';
            }
            stream << '"' << escape_json(transition.to_queue) << '"';
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, 3U, pretty);
            stream << "}";
            if (index + 1U < result.summary.queue_transitions.size())
            {
                stream << ',';
            }
            if (pretty)
            {
                stream << '\n';
            }
        }
        write_indent(stream, 2U, pretty);
        stream << "]";
        stream << ',';
        if (pretty)
        {
            stream << '\n';
        }

        const MemoryUsageSummary& memory = result.summary.memory;
        write_indent(stream, 2U, pretty);
        stream << '"' << "memory" << '"' << ':';
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
        stream << '"' << "vertex_count" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.vertex_count << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "joint_count" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.joint_count << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "position_bytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.position_bytes << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "normal_bytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.normal_bytes << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "transform_bytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.transform_bytes << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "total_bytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.total_bytes << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "total_mebibytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.total_mebibytes() << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "budget_bytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << kMemoryBudgetBytes << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "budget_mebibytes" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << memory.budget_mebibytes() << ',';
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 3U, pretty);
        stream << '"' << "exceeds_budget" << '"' << ':';
        if (pretty)
        {
            stream << ' ';
        }
        stream << (memory.exceeds_budget() ? "true" : "false");
        if (pretty)
        {
            stream << '\n';
        }
        write_indent(stream, 2U, pretty);
        stream << "}";
        stream << ',';
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
} // namespace

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

        for (std::size_t run_index = 0; run_index < options.repeat_count; ++run_index)
        {
            const ExecutionOutcome primary = execute_run(options);

            std::optional<ExecutionOutcome> baseline{};
            if (options.include_baseline)
            {
                CommandLineOptions baseline_options = options;
                baseline_options.queue_count = 1U;
                baseline_options.queue_names.clear();
                baseline_options.queue_overrides.clear();
                baseline_options.include_baseline = false;
                baseline = execute_run(baseline_options);
            }

            const RunResult* baseline_result = baseline ? &baseline->result : nullptr;
            const double speedup = baseline_result != nullptr
                                       ? compute_speedup(
                                           baseline_result->summary.frame_totals.mean_ms,
                                           primary.result.summary.frame_totals.mean_ms)
                                       : 0.0;

            if (run_index > 0U)
            {
                std::cout << '\n';
            }
            print_text_summary(primary.result, baseline_result, speedup, run_index, options.repeat_count);

            const auto output_path = make_output_path(options, run_index);
            if (output_path)
            {
                const std::filesystem::path parent = output_path->parent_path();
                if (!parent.empty())
                {
                    std::filesystem::create_directories(parent);
                }
                std::ofstream file{*output_path, std::ios::binary};
                if (!file)
                {
                    std::cerr << "Failed to open output file: " << output_path->string() << std::endl;
                    return 1;
                }
                write_json_report(
                    options,
                    primary.result,
                    primary.diagnostics,
                    baseline_result,
                    speedup,
                    file,
                    run_index,
                    options.repeat_count);
            }
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