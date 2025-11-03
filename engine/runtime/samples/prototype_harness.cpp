#include "engine/runtime/api.hpp"
#include "engine/runtime/config_schema.hpp"
#include "engine/runtime/errors.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <system_error>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using engine::runtime::RuntimeHost;
    using engine::runtime::RuntimeHostDependencies;
    namespace config = engine::runtime::config;

    class HarnessError final : public std::runtime_error
    {
    public:
        explicit HarnessError(std::string message)
            : std::runtime_error{std::move(message)}
        {
        }
    };

    struct CommandLineOptions
    {
        bool show_help = false;
        bool dry_run = false;
        std::optional<bool> require_schema{};
        std::optional<std::filesystem::path> config_path{};
        std::optional<std::filesystem::path> summary_path{};
        std::size_t frames = 600U;
        double timestep = 1.0 / 60.0;
        std::optional<std::string> scenario{};
        std::optional<std::string> runtime_profile{};
        std::optional<int> resolution_width{};
        std::optional<int> resolution_height{};
        std::optional<int> run_index{};
        std::optional<int> run_count{};
        std::vector<std::string> overlays{};
    };

    struct HarnessSummary
    {
        std::optional<std::string> dataset_identifier;
        std::optional<std::string> rendering_preset;
        std::optional<std::string> shading_mode;
        std::size_t frames_executed = 0U;
        double timestep_seconds = 0.0;
        double average_tick_ms = 0.0;
        std::vector<std::string> dispatch_order{};
        std::vector<double> dispatch_durations_ms{};
        std::optional<std::string> scenario{};
        std::optional<std::string> runtime_profile{};
        std::optional<int> resolution_width{};
        std::optional<int> resolution_height{};
        std::optional<int> run_index{};
        std::optional<int> run_count{};
        std::vector<std::string> overlays{};
    };

    [[nodiscard]] std::string format_error(const engine::runtime::RuntimeErrorCode& error)
    {
        std::ostringstream stream;
        stream << "[" << error.identifier() << "] ";
        if (error.has_message())
        {
            stream << error.message();
        }
        else
        {
            stream << error.identifier();
        }
        return stream.str();
    }

    [[nodiscard]] double parse_double(std::string_view text)
    {
        std::string buffer{text};
        char* end = nullptr;
        const double value = std::strtod(buffer.c_str(), &end); // NOLINT
        if (end == buffer.c_str() || !std::isfinite(value))
        {
            throw std::invalid_argument{"expected floating-point value"};
        }
        return value;
    }

    [[nodiscard]] std::size_t parse_size(std::string_view text)
    {
        std::string buffer{text};
        char* end = nullptr;
        const unsigned long long parsed = std::strtoull(buffer.c_str(), &end, 10); // NOLINT
        if (end == buffer.c_str())
        {
            throw std::invalid_argument{"expected positive integer"};
        }
        if (parsed == 0ULL)
        {
            throw std::invalid_argument{"value must be greater than zero"};
        }
        if (parsed > std::numeric_limits<std::size_t>::max())
        {
            throw std::out_of_range{"value exceeds size_t range"};
        }
        return static_cast<std::size_t>(parsed);
    }

    void print_usage()
    {
        std::cout << "Usage: runtime_prototype_harness --config <path> [options]\n"
                  << "Options:\n"
                  << "  --config <path>         Path to AI-004 configuration manifest (YAML/JSON)\n"
                  << "  --frames <count>        Number of frames to execute (default: 600)\n"
                  << "  --dt <seconds>          Fixed timestep for execution (default: 0.0166667)\n"
                  << "  --dry-run               Validate configuration without executing runtime\n"
                  << "  --require-schema        Require configuration to declare ai-004.* headers\n"
                  << "  --summary-json <path>   Write execution summary to JSON file\n"
                  << "  --runtime-profile <id>  Runtime profile/variant identifier\n"
                  << "  --resolution-width <px> Resolution width in pixels\n"
                  << "  --resolution-height <px> Resolution height in pixels\n"
                  << "  --overlay <name=value>  Enable overlay (can be specified multiple times)\n"
                  << "  --help                  Display this message\n";
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
            if (argument == "--dry-run")
            {
                options.dry_run = true;
                continue;
            }
            if (argument == "--require-schema")
            {
                options.require_schema = true;
                continue;
            }
            if (argument == "--config")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--config requires a path"};
                }
                options.config_path = std::filesystem::path{argv[++index]};
                continue;
            }
            if (argument == "--summary-json")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--summary-json requires a path"};
                }
                options.summary_path = std::filesystem::path{argv[++index]};
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
                if (options.timestep <= 0.0)
                {
                    throw std::invalid_argument{"--dt must be positive"};
                }
                continue;
            }
            if (argument == "--runtime-profile")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--runtime-profile requires a value"};
                }
                options.runtime_profile = argv[++index];
                continue;
            }
            if (argument == "--resolution-width")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--resolution-width requires a value"};
                }
                options.resolution_width = static_cast<int>(parse_size(argv[++index]));
                continue;
            }
            if (argument == "--resolution-height")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--resolution-height requires a value"};
                }
                options.resolution_height = static_cast<int>(parse_size(argv[++index]));
                continue;
            }
            if (argument == "--overlay")
            {
                if (index + 1 >= argc)
                {
                    throw std::invalid_argument{"--overlay requires a value"};
                }
                options.overlays.emplace_back(argv[++index]);
                continue;
            }

            std::ostringstream stream;
            stream << "unknown argument: " << argument;
            throw std::invalid_argument(stream.str());
        }

        if (!options.config_path)
        {
            throw std::invalid_argument{"--config is required"};
        }

        return options;
    }

    [[nodiscard]] const config::DatasetEntry*
    resolve_dataset(const config::DatasetManifest& manifest, const config::RuntimeConfig& runtime_config)
    {
        if (!runtime_config.dataset)
        {
            return nullptr;
        }

        const std::string& slug = runtime_config.dataset.value();
        for (const auto& entry : manifest.datasets)
        {
            if (entry.identifier == slug)
            {
                return &entry;
            }
        }

        std::ostringstream stream;
        stream << "runtime.dataset references unknown dataset '" << slug
               << "'. Provide a matching entry in datasets[].";
        throw HarnessError{stream.str()};
    }

    [[nodiscard]] RuntimeHostDependencies build_dependencies(const config::RuntimeConfig& runtime_config,
                                                             const config::DatasetEntry* dataset)
    {
        RuntimeHostDependencies dependencies{};
        if (dataset != nullptr)
        {
            dependencies.scene_name = dataset->identifier;
        }
        if (runtime_config.scene_manifest)
        {
            dependencies.scene_name = runtime_config.scene_manifest.value();
        }
        dependencies.streaming_config.enable = runtime_config.hot_reload.enabled;
        return dependencies;
    }

    HarnessSummary execute_runtime(const CommandLineOptions& options,
                                   const config::Ai004Configuration& configuration,
                                   const config::RuntimeConfig& runtime_config,
                                   const config::DatasetEntry* dataset)
    {
        HarnessSummary summary{};
        summary.dataset_identifier = dataset ? std::optional<std::string>{dataset->identifier} : std::nullopt;
        summary.scenario = summary.dataset_identifier;  // Use dataset as scenario identifier
        if (configuration.rendering)
        {
            summary.rendering_preset = configuration.rendering->preset;
            summary.shading_mode = configuration.rendering->shading_mode;
        }
        summary.timestep_seconds = options.timestep;
        summary.runtime_profile = options.runtime_profile;
        summary.resolution_width = options.resolution_width;
        summary.resolution_height = options.resolution_height;
        summary.run_index = options.run_index;
        summary.run_count = options.run_count;
        summary.overlays = options.overlays;

        if (options.dry_run)
        {
            return summary;
        }

        RuntimeHostDependencies dependencies = build_dependencies(runtime_config, dataset);
        RuntimeHost host{std::move(dependencies)};
        bool initialized = false;
        try
        {
            host.initialize();
            initialized = true;
            for (std::size_t frame = 0; frame < options.frames; ++frame)
            {
                (void)host.tick(options.timestep);
                ++summary.frames_executed;
            }
            summary.average_tick_ms = host.diagnostics().average_tick_ms;
            const auto& report = host.last_dispatch_report();
            const auto dispatch_count = std::min(report.execution_order.size(), report.kernel_durations.size());
            summary.dispatch_order.assign(report.execution_order.begin(),
                                          report.execution_order.begin() + static_cast<std::ptrdiff_t>(dispatch_count));
            summary.dispatch_durations_ms.clear();
            summary.dispatch_durations_ms.reserve(dispatch_count);
            for (std::size_t index = 0; index < dispatch_count; ++index)
            {
                summary.dispatch_durations_ms.push_back(report.kernel_durations[index] * 1000.0);
            }
            host.shutdown();
        }
        catch (...)
        {
            if (initialized)
            {
                host.shutdown();
            }
            throw;
        }

        return summary;
    }

    void print_summary(const HarnessSummary& summary, bool dry_run)
    {
        std::cout << (dry_run ? "Dry run summary" : "Execution summary") << '\n';
        std::cout << "  dataset: ";
        if (summary.dataset_identifier)
        {
            std::cout << *summary.dataset_identifier << '\n';
        }
        else
        {
            std::cout << "(none)\n";
        }
        std::cout << "  rendering preset: ";
        if (summary.rendering_preset)
        {
            std::cout << *summary.rendering_preset << '\n';
        }
        else
        {
            std::cout << "(none)\n";
        }
        std::cout << "  shading mode: ";
        if (summary.shading_mode)
        {
            std::cout << *summary.shading_mode << '\n';
        }
        else
        {
            std::cout << "(none)\n";
        }
        {
            const auto previous_flags = std::cout.flags();
            const auto previous_precision = std::cout.precision();
            std::cout << "  timestep: " << std::fixed << std::setprecision(6) << summary.timestep_seconds << '\n';
            std::cout.flags(previous_flags);
            std::cout.precision(previous_precision);
        }
        std::cout << "  dispatch count: " << summary.dispatch_order.size() << '\n';
        if (!summary.dispatch_order.empty())
        {
            std::cout << "  dispatch order: ";
            for (std::size_t index = 0; index < summary.dispatch_order.size(); ++index)
            {
                if (index != 0U)
                {
                    std::cout << ", ";
                }
                std::cout << summary.dispatch_order[index];
            }
            std::cout << '\n';
        }
        if (!dry_run)
        {
            const auto previous_flags = std::cout.flags();
            const auto previous_precision = std::cout.precision();
            std::cout << "  frames executed: " << summary.frames_executed << '\n';
            std::cout << "  average tick (ms): " << std::setprecision(3) << summary.average_tick_ms << '\n';
            if (!summary.dispatch_durations_ms.empty())
            {
                std::cout << "  dispatch durations (ms): " << std::fixed << std::setprecision(3);
                for (std::size_t index = 0; index < summary.dispatch_durations_ms.size(); ++index)
                {
                    if (index != 0U)
                    {
                        std::cout << ", ";
                    }
                    std::cout << summary.dispatch_durations_ms[index];
                }
                std::cout << '\n';
            }
            std::cout.flags(previous_flags);
            std::cout.precision(previous_precision);
        }
    }

    [[nodiscard]] std::string summary_to_json(const HarnessSummary& summary, bool dry_run)
    {
        std::ostringstream stream;
        stream << "{\n";
        stream << "  \"dry_run\": " << (dry_run ? "true" : "false") << ",\n";

        // scenario field (optional)
        stream << "  \"scenario\": ";
        if (summary.scenario)
        {
            stream << '\"' << *summary.scenario << '\"';
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";

        stream << "  \"dataset\": ";
        if (summary.dataset_identifier)
        {
            stream << '\"' << *summary.dataset_identifier << '\"';
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";
        stream << "  \"rendering_preset\": ";
        if (summary.rendering_preset)
        {
            stream << '\"' << *summary.rendering_preset << '\"';
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";
        stream << "  \"shading_mode\": ";
        if (summary.shading_mode)
        {
            stream << '\"' << *summary.shading_mode << '\"';
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";

        // runtime_profile field (optional)
        stream << "  \"runtime_profile\": ";
        if (summary.runtime_profile)
        {
            stream << '\"' << *summary.runtime_profile << '\"';
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";

        // resolution fields (optional)
        stream << "  \"resolution_width\": ";
        if (summary.resolution_width)
        {
            stream << *summary.resolution_width;
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";
        stream << "  \"resolution_height\": ";
        if (summary.resolution_height)
        {
            stream << *summary.resolution_height;
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";

        // overlays array
        stream << "  \"overlays\": [";
        for (std::size_t index = 0; index < summary.overlays.size(); ++index)
        {
            if (index != 0U)
            {
                stream << ", ";
            }
            stream << '\"' << summary.overlays[index] << '\"';
        }
        stream << "],\n";

        // frames (using frames_executed)
        stream << "  \"frames\": " << summary.frames_executed << ",\n";
        stream << "  \"frames_executed\": " << summary.frames_executed << ",\n";
        {
            const auto previous_flags = stream.flags();
            const auto previous_precision = stream.precision();
            stream << "  \"timestep_seconds\": " << std::setprecision(7) << summary.timestep_seconds << ",\n";
            stream.flags(previous_flags);
            stream.precision(previous_precision);
        }
        if (!dry_run)
        {
            const auto previous_flags = stream.flags();
            const auto previous_precision = stream.precision();
            stream << "  \"average_tick_ms\": " << std::setprecision(6) << summary.average_tick_ms << ",\n";
            stream.flags(previous_flags);
            stream.precision(previous_precision);
        }

        // run_index and run_count (optional)
        stream << "  \"run_index\": ";
        if (summary.run_index)
        {
            stream << *summary.run_index;
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";
        stream << "  \"run_count\": ";
        if (summary.run_count)
        {
            stream << *summary.run_count;
        }
        else
        {
            stream << "null";
        }
        stream << ",\n";

        stream << "  \"dispatch_order\": [";
        for (std::size_t index = 0; index < summary.dispatch_order.size(); ++index)
        {
            if (index != 0U)
            {
                stream << ", ";
            }
            stream << '\"' << summary.dispatch_order[index] << '\"';
        }
        stream << "],\n";
        stream << "  \"dispatch_durations_ms\": [";
        if (!summary.dispatch_durations_ms.empty())
        {
            const auto previous_flags = stream.flags();
            const auto previous_precision = stream.precision();
            stream << std::fixed << std::setprecision(6);
            for (std::size_t index = 0; index < summary.dispatch_durations_ms.size(); ++index)
            {
                if (index != 0U)
                {
                    stream << ", ";
                }
                stream << summary.dispatch_durations_ms[index];
            }
            stream.flags(previous_flags);
            stream.precision(previous_precision);
        }
        stream << "],\n";

        // telemetry_outputs placeholder (for compatibility)
        stream << "  \"telemetry_outputs\": []\n";
        stream << "}\n";
        return stream.str();
    }

    void write_summary_json(const HarnessSummary& summary, bool dry_run, const std::filesystem::path& path)
    {
        if (const auto parent = path.parent_path(); !parent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec)
            {
                std::ostringstream stream;
                stream << "failed to create summary directory: " << parent << " (" << ec.message() << ")";
                throw HarnessError{stream.str()};
            }
        }

        std::ofstream output{path, std::ios::binary};
        if (!output)
        {
            std::ostringstream stream;
            stream << "failed to open summary output file: " << path;
            throw HarnessError{stream.str()};
        }

        output << summary_to_json(summary, dry_run);
        output.flush();
        if (!output)
        {
            std::ostringstream stream;
            stream << "failed to write summary output file: " << path;
            throw HarnessError{stream.str()};
        }
    }
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const auto options = parse_command_line(argc, argv);
        if (options.show_help)
        {
            print_usage();
            return EXIT_SUCCESS;
        }

        const std::filesystem::path config_path = *options.config_path;
        auto config_result = config::load_configuration(config_path, options.require_schema);
        if (!config_result)
        {
            std::cerr << "error: " << format_error(config_result.error()) << '\n';
            return EXIT_FAILURE;
        }
        config::Ai004Configuration configuration = std::move(config_result.value());
        if (!configuration.runtime)
        {
            throw HarnessError{"configuration.runtime section is required for harness execution"};
        }

        const config::RuntimeConfig& runtime_config = configuration.runtime.value();
        const config::DatasetEntry* dataset = resolve_dataset(configuration.datasets, runtime_config);

        HarnessSummary summary = execute_runtime(options, configuration, runtime_config, dataset);
        print_summary(summary, options.dry_run);

        if (options.summary_path)
        {
            write_summary_json(summary, options.dry_run, *options.summary_path);
        }

        return EXIT_SUCCESS;
    }
    catch (const HarnessError& error)
    {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "error: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
