#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/frustum.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/math/math.hpp"
#include "engine/math/utils/utils_camera.hpp"

namespace
{
    struct BenchmarkConfig
    {
        std::size_t box_count{200'000U};
        std::size_t iterations{512U};
        std::uint32_t seed{0xDEADBEEF};
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
        return "geometry_frustum_benchmark [--boxes <count>] [--iterations <count>] [--seed <value>] [--output <path>]";
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

    [[nodiscard]] bool parse_unsigned(std::string_view text, std::uint32_t& value) noexcept
    {
        std::uint32_t parsed = 0U;
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
            if (argument == "--boxes")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--boxes requires a value";
                    return result;
                }
                ++index;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[index], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--boxes must be a positive integer";
                    return result;
                }
                result.config.box_count = value;
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
            if (argument == "--seed")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--seed requires a value";
                    return result;
                }
                ++index;
                std::uint32_t value = 0U;
                if (!parse_unsigned(argv[index], value))
                {
                    result.ok = false;
                    result.error = "--seed must be an unsigned integer";
                    return result;
                }
                result.config.seed = value;
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

    [[nodiscard]] engine::geometry::Frustum make_reference_frustum() noexcept
    {
        const engine::math::vec3 eye{0.0F, 1.5F, -4.0F};
        const engine::math::vec3 target{0.0F, 0.5F, 0.0F};
        const engine::math::vec3 up{0.0F, 1.0F, 0.0F};
        const engine::math::mat4 view = engine::math::utils::look_at(eye, target, up);
        const float aspect = 16.0F / 9.0F;
        const engine::math::mat4 projection =
            engine::math::utils::perspective(engine::math::utils::radians(60.0F), aspect, 0.1F, 200.0F);
        return engine::geometry::ExtractFrustum(projection * view);
    }

    [[nodiscard]] std::vector<engine::geometry::Aabb> make_boxes(const BenchmarkConfig& config)
    {
        std::vector<engine::geometry::Aabb> boxes(config.box_count);
        engine::geometry::RandomEngine rng{config.seed};
        for (auto& box : boxes)
        {
            engine::geometry::Random(box, rng);
        }
        return boxes;
    }

    struct BenchmarkMetrics
    {
        double duration_seconds{0.0};
        double tests_per_second{0.0};
        double nanoseconds_per_test{0.0};
        double hit_ratio{0.0};
        std::size_t box_count{0U};
        std::size_t iterations{0U};
        std::size_t total_tests{0U};
        std::size_t hits{0U};
    };

    [[nodiscard]] BenchmarkMetrics run_benchmark(const BenchmarkConfig& config)
    {
        const auto boxes = make_boxes(config);
        const auto frustum = make_reference_frustum();
        const std::size_t total_tests = config.box_count * config.iterations;

        std::size_t hit_count = 0U;
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t iteration = 0; iteration < config.iterations; ++iteration)
        {
            for (const auto& box : boxes)
            {
                if (engine::geometry::Intersects(frustum, box))
                {
                    ++hit_count;
                }
            }
        }
        const auto end = std::chrono::steady_clock::now();

        const double duration_seconds = std::chrono::duration<double>(end - start).count();
        const double tests_per_second = duration_seconds > 0.0
                                            ? static_cast<double>(total_tests) / duration_seconds
                                            : 0.0;
        const double nanoseconds_per_test = tests_per_second > 0.0
                                                ? 1'000'000'000.0 / tests_per_second
                                                : 0.0;
        const double hit_ratio = total_tests > 0U
                                     ? static_cast<double>(hit_count) / static_cast<double>(total_tests)
                                     : 0.0;

        BenchmarkMetrics metrics{};
        metrics.duration_seconds = duration_seconds;
        metrics.tests_per_second = tests_per_second;
        metrics.nanoseconds_per_test = nanoseconds_per_test;
        metrics.hit_ratio = hit_ratio;
        metrics.box_count = config.box_count;
        metrics.iterations = config.iterations;
        metrics.total_tests = total_tests;
        metrics.hits = hit_count;
        return metrics;
    }

    [[nodiscard]] std::string serialize_metrics(const BenchmarkConfig& config, const BenchmarkMetrics& metrics)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(6);

        stream << "{\n";
        stream << "  \"benchmark\":\"geometry_frustum_culling\",\n";
        stream << "  \"config\":{\n";
        stream << "    \"boxes\":" << static_cast<unsigned long long>(config.box_count) << ",\n";
        stream << "    \"iterations\":" << static_cast<unsigned long long>(config.iterations) << ",\n";
        stream << "    \"seed\":" << static_cast<unsigned long long>(config.seed) << "\n";
        stream << "  },\n";
        stream << "  \"metrics\":{\n";
        stream << "    \"duration_seconds\":" << metrics.duration_seconds << ",\n";
        stream << "    \"tests_per_second\":" << metrics.tests_per_second << ",\n";
        stream << "    \"nanoseconds_per_test\":" << metrics.nanoseconds_per_test << ",\n";
        stream << "    \"hit_ratio\":" << metrics.hit_ratio << ",\n";
        stream << "    \"total_tests\":" << static_cast<unsigned long long>(metrics.total_tests) << ",\n";
        stream << "    \"hits\":" << static_cast<unsigned long long>(metrics.hits) << "\n";
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
} // namespace

int main(int argc, char** argv)
{
    const auto parse = parse_arguments(argc, argv);
    if (parse.help)
    {
        std::cout << usage() << '\n';
        return 0;
    }
    if (!parse.ok)
    {
        std::cerr << "geometry_frustum_benchmark: " << parse.error << '\n';
        std::cerr << usage() << '\n';
        return 1;
    }

    const auto metrics = run_benchmark(parse.config);
    const auto json = serialize_metrics(parse.config, metrics);

    std::cout << json;
    if (!parse.config.output_path.empty())
    {
        if (!write_output_file(parse.config.output_path, json))
        {
            std::cerr << "Failed to write benchmark output to '" << parse.config.output_path << "'\n";
            return 1;
        }
    }

    return 0;
}
