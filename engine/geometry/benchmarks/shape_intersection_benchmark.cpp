#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/geometry/random.hpp"
#include "engine/geometry/shapes.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/math/math.hpp"
#include "engine/math/utils/utils.hpp"

namespace
{
    using engine::geometry::RandomEngine;

    struct BenchmarkConfig
    {
        std::size_t pair_count{100'000U};
        std::size_t iterations{64U};
        std::uint32_t seed{0x91C0FFEEU};
        std::string output_path{};
    };

    struct ParseResult
    {
        BenchmarkConfig config{};
        bool ok{true};
        bool help{false};
        std::string error{};
    };

    struct ScenarioMetrics
    {
        std::string name{};
        double duration_seconds{0.0};
        double tests_per_second{0.0};
        double nanoseconds_per_test{0.0};
        double hit_ratio{0.0};
        std::size_t total_tests{0U};
        std::size_t hits{0U};
        std::size_t pair_count{0U};
        std::size_t iterations{0U};
    };

    [[nodiscard]] std::string_view usage() noexcept
    {
        return "geometry_shape_intersection_benchmark [--pairs <count>] [--iterations <count>] [--seed <value>] [--output <path>]";
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
            if (argument == "--pairs")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--pairs requires a value";
                    return result;
                }
                ++index;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[index], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--pairs must be a positive integer";
                    return result;
                }
                result.config.pair_count = value;
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

    [[nodiscard]] float uniform(RandomEngine& rng, float min_value, float max_value) noexcept
    {
        std::uniform_real_distribution<float> distribution(min_value, max_value);
        return distribution(rng);
    }

    [[nodiscard]] engine::math::vec3 random_unit_vector(RandomEngine& rng) noexcept
    {
        std::normal_distribution<float> normal(0.0f, 1.0f);
        engine::math::vec3 candidate{0.0F};
        do
        {
            candidate = engine::math::vec3{normal(rng), normal(rng), normal(rng)};
        }
        while (engine::math::length_squared(candidate) <= std::numeric_limits<float>::epsilon());
        return engine::math::normalize(candidate);
    }

    ScenarioMetrics run_aabb_sphere(const BenchmarkConfig& config)
    {
        struct Pair
        {
            engine::geometry::Aabb box{};
            engine::geometry::Sphere sphere{};
        };

        RandomEngine rng{config.seed ^ 0xA11CE5EEDU};
        std::vector<Pair> pairs(config.pair_count);
        for (std::size_t index = 0; index < pairs.size(); ++index)
        {
            auto& pair = pairs[index];
            engine::geometry::Random(pair.box, rng);
            const engine::math::vec3 center = engine::geometry::Center(pair.box);
            const engine::math::vec3 extent = engine::geometry::Extent(pair.box);
            const float max_extent = engine::math::utils::max(extent[0], engine::math::utils::max(extent[1], extent[2]));
            const float extent_length = engine::math::length(extent);
            const bool expect_hit = (index % 2U) == 0U;

            if (expect_hit)
            {
                pair.sphere.radius = uniform(rng, 0.5F * max_extent, 1.25F * max_extent);
                const engine::math::vec3 offset{
                    uniform(rng, -extent[0], extent[0]),
                    uniform(rng, -extent[1], extent[1]),
                    uniform(rng, -extent[2], extent[2])
                };
                pair.sphere.center = center + 0.5F * offset;
            }
            else
            {
                pair.sphere.radius = uniform(rng, 0.25F * max_extent, 0.75F * max_extent);
                const engine::math::vec3 direction = random_unit_vector(rng);
                const float distance = extent_length + pair.sphere.radius + uniform(rng, 0.5F, 2.0F);
                pair.sphere.center = center + direction * distance;
            }
        }

        const std::size_t total_tests = config.pair_count * config.iterations;
        std::size_t hit_count = 0U;
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t iteration = 0; iteration < config.iterations; ++iteration)
        {
            for (const auto& pair : pairs)
            {
                if (engine::geometry::Intersects(pair.box, pair.sphere))
                {
                    ++hit_count;
                }
            }
        }
        const auto end = std::chrono::steady_clock::now();

        ScenarioMetrics metrics{};
        metrics.name = "aabb_sphere";
        metrics.duration_seconds = std::chrono::duration<double>(end - start).count();
        metrics.tests_per_second = metrics.duration_seconds > 0.0
                                       ? static_cast<double>(total_tests) / metrics.duration_seconds
                                       : 0.0;
        metrics.nanoseconds_per_test = metrics.tests_per_second > 0.0
                                           ? 1'000'000'000.0 / metrics.tests_per_second
                                           : 0.0;
        metrics.hit_ratio = total_tests > 0U
                                 ? static_cast<double>(hit_count) / static_cast<double>(total_tests)
                                 : 0.0;
        metrics.total_tests = total_tests;
        metrics.hits = hit_count;
        metrics.pair_count = config.pair_count;
        metrics.iterations = config.iterations;
        return metrics;
    }

    ScenarioMetrics run_ray_triangle(const BenchmarkConfig& config)
    {
        struct Pair
        {
            engine::geometry::Ray ray{};
            engine::geometry::Triangle triangle{};
        };

        RandomEngine rng{config.seed ^ 0xC0FFEE12U};
        std::vector<Pair> pairs(config.pair_count);
        for (std::size_t index = 0; index < pairs.size(); ++index)
        {
            auto& pair = pairs[index];
            engine::geometry::Random(pair.triangle, rng);

            engine::math::vec3 normal = engine::math::cross(pair.triangle.b - pair.triangle.a,
                                                             pair.triangle.c - pair.triangle.a);
            const float normal_length = engine::math::length(normal);
            if (normal_length <= std::numeric_limits<float>::epsilon())
            {
                normal = engine::math::vec3{0.0F, 1.0F, 0.0F};
            }
            else
            {
                normal /= normal_length;
            }

            const engine::math::vec3 centroid = (pair.triangle.a + pair.triangle.b + pair.triangle.c) / 3.0F;
            const float edge_ab = engine::math::length(pair.triangle.b - pair.triangle.a);
            const float edge_bc = engine::math::length(pair.triangle.c - pair.triangle.b);
            const float edge_ca = engine::math::length(pair.triangle.a - pair.triangle.c);
            const float average_edge = engine::math::utils::max((edge_ab + edge_bc + edge_ca) / 3.0F, 0.5F);
            const bool expect_hit = (index % 2U) == 0U;

            if (expect_hit)
            {
                const float offset = uniform(rng, 0.05F, 0.25F) * average_edge;
                pair.ray.origin = centroid + normal * offset;
                pair.ray.direction = -normal;
            }
            else
            {
                const float offset = average_edge + uniform(rng, 1.0F, 3.0F);
                pair.ray.origin = centroid + normal * offset;
                pair.ray.direction = normal;
            }
        }

        const std::size_t total_tests = config.pair_count * config.iterations;
        std::size_t hit_count = 0U;
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t iteration = 0; iteration < config.iterations; ++iteration)
        {
            for (const auto& pair : pairs)
            {
                if (engine::geometry::Intersects(pair.ray, pair.triangle, nullptr))
                {
                    ++hit_count;
                }
            }
        }
        const auto end = std::chrono::steady_clock::now();

        ScenarioMetrics metrics{};
        metrics.name = "ray_triangle";
        metrics.duration_seconds = std::chrono::duration<double>(end - start).count();
        metrics.tests_per_second = metrics.duration_seconds > 0.0
                                       ? static_cast<double>(total_tests) / metrics.duration_seconds
                                       : 0.0;
        metrics.nanoseconds_per_test = metrics.tests_per_second > 0.0
                                           ? 1'000'000'000.0 / metrics.tests_per_second
                                           : 0.0;
        metrics.hit_ratio = total_tests > 0U
                                 ? static_cast<double>(hit_count) / static_cast<double>(total_tests)
                                 : 0.0;
        metrics.total_tests = total_tests;
        metrics.hits = hit_count;
        metrics.pair_count = config.pair_count;
        metrics.iterations = config.iterations;
        return metrics;
    }

    ScenarioMetrics run_cylinder_sphere(const BenchmarkConfig& config)
    {
        struct Pair
        {
            engine::geometry::Cylinder cylinder{};
            engine::geometry::Sphere sphere{};
        };

        RandomEngine rng{config.seed ^ 0x9E3779B9U};
        std::vector<Pair> pairs(config.pair_count);
        for (std::size_t index = 0; index < pairs.size(); ++index)
        {
            auto& pair = pairs[index];
            engine::geometry::Random(pair.cylinder, rng);
            const engine::math::vec3 axis_dir = engine::geometry::AxisDirection(pair.cylinder);
            const bool expect_hit = (index % 2U) == 0U;

            if (expect_hit)
            {
                pair.sphere.radius = uniform(rng, pair.cylinder.radius * 0.3F, pair.cylinder.radius * 0.9F);
                const float axial = uniform(rng, -pair.cylinder.half_height * 0.9F, pair.cylinder.half_height * 0.9F);
                engine::math::vec3 radial = random_unit_vector(rng);
                radial -= axis_dir * engine::math::dot(radial, axis_dir);
                if (engine::math::length_squared(radial) <= std::numeric_limits<float>::epsilon())
                {
                    radial = engine::math::cross(axis_dir, engine::math::vec3{1.0F, 0.0F, 0.0F});
                    if (engine::math::length_squared(radial) <= std::numeric_limits<float>::epsilon())
                    {
                        radial = engine::math::cross(axis_dir, engine::math::vec3{0.0F, 1.0F, 0.0F});
                    }
                }
                radial = engine::math::normalize(radial);
                const float radial_distance = uniform(rng, 0.0F, pair.cylinder.radius * 0.75F);
                pair.sphere.center = pair.cylinder.center + axis_dir * axial + radial * radial_distance;
            }
            else
            {
                pair.sphere.radius = uniform(rng, pair.cylinder.radius * 0.3F, pair.cylinder.radius * 0.9F);
                const float separation = pair.cylinder.half_height + pair.sphere.radius + uniform(rng, 0.5F, 2.0F);
                const float sign = (index % 4U < 2U) ? 1.0F : -1.0F;
                pair.sphere.center = pair.cylinder.center + axis_dir * separation * sign;
            }
        }

        const std::size_t total_tests = config.pair_count * config.iterations;
        std::size_t hit_count = 0U;
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t iteration = 0; iteration < config.iterations; ++iteration)
        {
            for (const auto& pair : pairs)
            {
                if (engine::geometry::Intersects(pair.cylinder, pair.sphere))
                {
                    ++hit_count;
                }
            }
        }
        const auto end = std::chrono::steady_clock::now();

        ScenarioMetrics metrics{};
        metrics.name = "cylinder_sphere";
        metrics.duration_seconds = std::chrono::duration<double>(end - start).count();
        metrics.tests_per_second = metrics.duration_seconds > 0.0
                                       ? static_cast<double>(total_tests) / metrics.duration_seconds
                                       : 0.0;
        metrics.nanoseconds_per_test = metrics.tests_per_second > 0.0
                                           ? 1'000'000'000.0 / metrics.tests_per_second
                                           : 0.0;
        metrics.hit_ratio = total_tests > 0U
                                 ? static_cast<double>(hit_count) / static_cast<double>(total_tests)
                                 : 0.0;
        metrics.total_tests = total_tests;
        metrics.hits = hit_count;
        metrics.pair_count = config.pair_count;
        metrics.iterations = config.iterations;
        return metrics;
    }

    [[nodiscard]] std::vector<ScenarioMetrics> run_benchmarks(const BenchmarkConfig& config)
    {
        std::vector<ScenarioMetrics> scenarios{};
        scenarios.reserve(3);
        scenarios.push_back(run_aabb_sphere(config));
        scenarios.push_back(run_ray_triangle(config));
        scenarios.push_back(run_cylinder_sphere(config));
        return scenarios;
    }

    [[nodiscard]] std::string serialize_metrics(const BenchmarkConfig& config,
                                                const std::vector<ScenarioMetrics>& scenarios)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(6);

        stream << "{\n";
        stream << "  \"benchmark\":\"geometry_shape_intersections\",\n";
        stream << "  \"config\":{\n";
        stream << "    \"pairs\":" << static_cast<unsigned long long>(config.pair_count) << ",\n";
        stream << "    \"iterations\":" << static_cast<unsigned long long>(config.iterations) << ",\n";
        stream << "    \"seed\":" << static_cast<unsigned long long>(config.seed) << "\n";
        stream << "  },\n";
        stream << "  \"scenarios\":[\n";
        for (std::size_t index = 0; index < scenarios.size(); ++index)
        {
            const auto& metrics = scenarios[index];
            stream << "    {\n";
            stream << "      \"name\":\"" << metrics.name << "\",\n";
            stream << "      \"duration_seconds\":" << metrics.duration_seconds << ",\n";
            stream << "      \"tests_per_second\":" << metrics.tests_per_second << ",\n";
            stream << "      \"nanoseconds_per_test\":" << metrics.nanoseconds_per_test << ",\n";
            stream << "      \"hit_ratio\":" << metrics.hit_ratio << ",\n";
            stream << "      \"pairs\":" << static_cast<unsigned long long>(metrics.pair_count) << ",\n";
            stream << "      \"iterations\":" << static_cast<unsigned long long>(metrics.iterations) << ",\n";
            stream << "      \"total_tests\":" << static_cast<unsigned long long>(metrics.total_tests) << ",\n";
            stream << "      \"hits\":" << static_cast<unsigned long long>(metrics.hits) << "\n";
            stream << "    }";
            if (index + 1 < scenarios.size())
            {
                stream << ",";
            }
            stream << "\n";
        }
        stream << "  ]\n";
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
        std::cerr << "geometry_shape_intersection_benchmark: " << parse.error << '\n';
        std::cerr << usage() << '\n';
        return 1;
    }

    const auto scenarios = run_benchmarks(parse.config);
    const auto json = serialize_metrics(parse.config, scenarios);

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
