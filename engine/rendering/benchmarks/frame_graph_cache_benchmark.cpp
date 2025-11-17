#include <algorithm>
#include <array>
#include <chrono>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/rendering/frame_graph.hpp"

namespace
{
    using engine::rendering::CallbackRenderPass;
    using engine::rendering::FrameGraph;
    using engine::rendering::FrameGraphPassBuilder;
    using engine::rendering::FrameGraphPassExecutionContext;
    using engine::rendering::FrameGraphResourceDescriptor;
    using engine::rendering::FrameGraphResourceHandle;
    using engine::rendering::PassPhase;
    using engine::rendering::QueueType;
    using engine::rendering::ResourceDimension;
    using engine::rendering::ResourceFormat;
    using engine::rendering::ResourceLifetime;
    using engine::rendering::ResourceSampleCount;
    using engine::rendering::ResourceState;
    using engine::rendering::ResourceUsage;
    using engine::rendering::ValidationSeverity;

    struct BenchmarkConfig
    {
        std::size_t passes{12U};
        std::size_t iterations{64U};
        std::string output_path{};
    };

    struct ParseResult
    {
        BenchmarkConfig config{};
        bool ok{true};
        bool help{false};
        std::string error{};
    };

    [[nodiscard]] std::string_view usage() noexcept
    {
        return "rendering_frame_graph_cache_benchmark [--passes <count>] [--iterations <count>] [--output <path>]";
    }

    [[nodiscard]] bool parse_unsigned(std::string_view text, std::size_t& value) noexcept
    {
        std::size_t parsed = 0U;
        const auto* begin = text.data();
        const auto* end = begin + text.size();
        const auto result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc{} || result.ptr != end)
        {
            return false;
        }
        value = parsed;
        return true;
    }

    ParseResult parse_arguments(int argc, char** argv)
    {
        ParseResult result{};
        for (int index = 1; index < argc; ++index)
        {
            const std::string_view argument{argv[index]};
            if (argument == "--help")
            {
                result.help = true;
                return result;
            }
            if (argument == "--passes")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--passes requires a value";
                    return result;
                }
                ++index;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[index], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--passes must be a positive integer";
                    return result;
                }
                result.config.passes = value;
                continue;
            }
            if (argument == "--iterations")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--iterations requires a value";
                    return result;
                }
                ++index;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[index], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--iterations must be a positive integer";
                    return result;
                }
                result.config.iterations = value;
                continue;
            }
            if (argument == "--output")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--output requires a path";
                    return result;
                }
                ++index;
                result.config.output_path = argv[index];
                continue;
            }

            result.ok = false;
            std::ostringstream stream;
            stream << "Unknown argument: " << argument;
            result.error = stream.str();
            return result;
        }

        return result;
    }

    [[nodiscard]] FrameGraphResourceDescriptor make_color_resource(std::string name)
    {
        FrameGraphResourceDescriptor descriptor{};
        descriptor.name = std::move(name);
        descriptor.lifetime = ResourceLifetime::Transient;
        descriptor.format = ResourceFormat::Rgba16f;
        descriptor.dimension = ResourceDimension::Texture2D;
        descriptor.usage = ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead | ResourceUsage::ShaderWrite;
        descriptor.initial_state = ResourceState::ColorAttachment;
        descriptor.final_state = ResourceState::ShaderRead;
        descriptor.width = 1920;
        descriptor.height = 1080;
        descriptor.depth = 1;
        descriptor.array_layers = 1;
        descriptor.mip_levels = 1;
        descriptor.sample_count = ResourceSampleCount::Count1;
        descriptor.size_bytes = static_cast<std::uint64_t>(descriptor.width) * descriptor.height * 8ULL;
        return descriptor;
    }

    [[nodiscard]] FrameGraphResourceDescriptor make_depth_resource(std::string name)
    {
        FrameGraphResourceDescriptor descriptor{};
        descriptor.name = std::move(name);
        descriptor.lifetime = ResourceLifetime::Transient;
        descriptor.format = ResourceFormat::Depth24Stencil8;
        descriptor.dimension = ResourceDimension::Texture2D;
        descriptor.usage = ResourceUsage::DepthStencilAttachment | ResourceUsage::ShaderRead;
        descriptor.initial_state = ResourceState::DepthStencilAttachment;
        descriptor.final_state = ResourceState::DepthStencilAttachment;
        descriptor.width = 1920;
        descriptor.height = 1080;
        descriptor.depth = 1;
        descriptor.array_layers = 1;
        descriptor.mip_levels = 1;
        descriptor.sample_count = ResourceSampleCount::Count1;
        descriptor.size_bytes = static_cast<std::uint64_t>(descriptor.width) * descriptor.height * 4ULL;
        return descriptor;
    }

    [[nodiscard]] FrameGraphResourceDescriptor make_storage_buffer(std::string name)
    {
        FrameGraphResourceDescriptor descriptor{};
        descriptor.name = std::move(name);
        descriptor.lifetime = ResourceLifetime::Transient;
        descriptor.dimension = ResourceDimension::Buffer;
        descriptor.usage = ResourceUsage::ShaderRead | ResourceUsage::ShaderWrite;
        descriptor.initial_state = ResourceState::ShaderRead;
        descriptor.final_state = ResourceState::ShaderWrite;
        descriptor.size_bytes = 64ULL * 1024ULL;
        return descriptor;
    }

    void build_linear_graph(FrameGraph& graph, std::size_t pass_count)
    {
        const auto depth = graph.create_resource(make_depth_resource("Depth"));
        FrameGraphResourceHandle previous_color{};
        const std::array phases{
            PassPhase::Setup,
            PassPhase::Geometry,
            PassPhase::Lighting,
            PassPhase::PostProcess,
            PassPhase::Compute,
            PassPhase::Transfer,
        };

        for (std::size_t pass_index = 0; pass_index < pass_count; ++pass_index)
        {
            auto color = graph.create_resource(make_color_resource("Color_" + std::to_string(pass_index)));
            auto storage = graph.create_resource(make_storage_buffer("Storage_" + std::to_string(pass_index)));
            const auto previous = previous_color;
            const bool uses_depth = (pass_index % 3U) == 0U;
            const auto queue = (pass_index % 2U) == 0U ? QueueType::Graphics : QueueType::Compute;
            const auto phase = phases[pass_index % phases.size()];

            graph.add_pass(std::make_unique<CallbackRenderPass>(
                "Pass_" + std::to_string(pass_index),
                [previous, color, storage, depth, uses_depth](FrameGraphPassBuilder& builder)
                {
                    if (uses_depth)
                    {
                        builder.read(depth);
                    }
                    if (previous.valid())
                    {
                        builder.read(previous);
                    }
                    builder.write(color);
                    builder.write(storage);
                },
                [](FrameGraphPassExecutionContext&)
                {
                },
                queue,
                phase,
                ValidationSeverity::Warning));

            previous_color = color;
        }
    }

    double compile_graph(FrameGraph& graph, std::size_t pass_count)
    {
        graph.reset();
        build_linear_graph(graph, pass_count);
        const auto start = std::chrono::steady_clock::now();
        graph.compile();
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::micro>(end - start).count();
    }

    struct SeriesMetrics
    {
        double min_us{0.0};
        double max_us{0.0};
        double average_us{0.0};
    };

    [[nodiscard]] SeriesMetrics compute_series_metrics(const std::vector<double>& samples)
    {
        SeriesMetrics metrics{};
        if (samples.empty())
        {
            return metrics;
        }

        double total = 0.0;
        double min_value = samples.front();
        double max_value = samples.front();
        for (double sample : samples)
        {
            total += sample;
            min_value = std::min(min_value, sample);
            max_value = std::max(max_value, sample);
        }

        metrics.min_us = min_value;
        metrics.max_us = max_value;
        metrics.average_us = total / static_cast<double>(samples.size());
        return metrics;
    }

    struct BenchmarkMetrics
    {
        SeriesMetrics miss;
        SeriesMetrics hit;
        double speedup{0.0};
    };

    [[nodiscard]] BenchmarkMetrics run_benchmark(const BenchmarkConfig& config)
    {
        FrameGraph graph;
        std::vector<double> miss_samples;
        std::vector<double> hit_samples;
        miss_samples.reserve(config.iterations);
        hit_samples.reserve(config.iterations);

        for (std::size_t iteration = 0; iteration < config.iterations; ++iteration)
        {
            graph.clear_cache();
            miss_samples.push_back(compile_graph(graph, config.passes));
            hit_samples.push_back(compile_graph(graph, config.passes));
        }

        BenchmarkMetrics metrics{};
        metrics.miss = compute_series_metrics(miss_samples);
        metrics.hit = compute_series_metrics(hit_samples);
        if (metrics.hit.average_us > 0.0)
        {
            metrics.speedup = metrics.miss.average_us / metrics.hit.average_us;
        }
        return metrics;
    }

    [[nodiscard]] std::string serialize_metrics(const BenchmarkConfig& config, const BenchmarkMetrics& metrics)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(3);

        stream << "{\n";
        stream << "  \"benchmark\":\"rendering_frame_graph_compile_cache\",\n";
        stream << "  \"config\":{\n";
        stream << "    \"passes\":" << static_cast<unsigned long long>(config.passes) << ",\n";
        stream << "    \"iterations\":" << static_cast<unsigned long long>(config.iterations) << "\n";
        stream << "  },\n";
        stream << "  \"metrics\":{\n";
        stream << "    \"cache_miss_avg_us\":" << metrics.miss.average_us << ",\n";
        stream << "    \"cache_miss_min_us\":" << metrics.miss.min_us << ",\n";
        stream << "    \"cache_miss_max_us\":" << metrics.miss.max_us << ",\n";
        stream << "    \"cache_hit_avg_us\":" << metrics.hit.average_us << ",\n";
        stream << "    \"cache_hit_min_us\":" << metrics.hit.min_us << ",\n";
        stream << "    \"cache_hit_max_us\":" << metrics.hit.max_us << ",\n";
        stream << "    \"speedup\":" << metrics.speedup << "\n";
        stream << "  }\n";
        stream << "}\n";

        return stream.str();
    }

    bool write_output_file(const std::string& path, const std::string& contents)
    {
        std::ofstream file{path, std::ios::binary | std::ios::trunc};
        if (!file)
        {
            return false;
        }
        file << contents;
        return static_cast<bool>(file);
    }
}

int main(int argc, char** argv)
{
    const auto parsed = parse_arguments(argc, argv);
    if (parsed.help)
    {
        std::cout << usage() << '\n';
        return 0;
    }
    if (!parsed.ok)
    {
        std::cerr << "rendering_frame_graph_cache_benchmark: " << parsed.error << '\n';
        std::cerr << usage() << '\n';
        return 1;
    }

    const auto metrics = run_benchmark(parsed.config);
    const auto json = serialize_metrics(parsed.config, metrics);

    std::cout << json;
    if (!parsed.config.output_path.empty())
    {
        if (!write_output_file(parsed.config.output_path, json))
        {
            std::cerr << "Failed to write benchmark output to '" << parsed.config.output_path << "'\n";
            return 1;
        }
    }

    return 0;
}
