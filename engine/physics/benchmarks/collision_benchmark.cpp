#include <algorithm>
#include <chrono>
#include <charconv>
#include <cmath>
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

#include "engine/physics/api.hpp"

namespace
{
    struct BenchmarkConfig
    {
        std::size_t bodies{128U};
        std::size_t steps{512U};
        float dt{1.0F / 60.0F};
        std::uint32_t seed{1337U};
        std::string output_path;
    };

    struct ParseResult
    {
        BenchmarkConfig config{};
        bool ok{true};
        bool help{false};
        std::string error;
    };

    [[nodiscard]] std::string_view usage() noexcept
    {
        return "physics_collision_benchmark [--bodies <count>] [--steps <count>] [--dt <seconds>] "
            "[--seed <value>] [--output <path>]";
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

    [[nodiscard]] bool parse_unsigned32(std::string_view text, std::uint32_t& value) noexcept
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

    [[nodiscard]] bool parse_float(std::string_view text, float& value) noexcept
    {
        float parsed = 0.0F;
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

        for (int i = 1; i < argc; ++i)
        {
            const std::string_view argument{argv[i]};
            if (argument == "--help")
            {
                result.help = true;
                return result;
            }
            if (argument == "--bodies")
            {
                if (i + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--bodies requires a value";
                    return result;
                }
                ++i;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[i], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--bodies must be a positive integer";
                    return result;
                }
                result.config.bodies = value;
                continue;
            }
            if (argument == "--steps")
            {
                if (i + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--steps requires a value";
                    return result;
                }
                ++i;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[i], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--steps must be a positive integer";
                    return result;
                }
                result.config.steps = value;
                continue;
            }
            if (argument == "--dt")
            {
                if (i + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--dt requires a value";
                    return result;
                }
                ++i;
                float value = 0.0F;
                if (!parse_float(argv[i], value) || !(value > 0.0F))
                {
                    result.ok = false;
                    result.error = "--dt must be a positive floating-point value";
                    return result;
                }
                result.config.dt = value;
                continue;
            }
            if (argument == "--seed")
            {
                if (i + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--seed requires a value";
                    return result;
                }
                ++i;
                std::uint32_t value = 0U;
                if (!parse_unsigned32(argv[i], value))
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
                if (i + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--output requires a path";
                    return result;
                }
                ++i;
                result.config.output_path = argv[i];
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

    [[nodiscard]] engine::physics::PhysicsWorld make_world(const BenchmarkConfig& config)
    {
        engine::physics::PhysicsWorld world{};
        world.gravity = engine::math::vec3{0.0F, 0.0F, 0.0F};
        world.linear_damping = 0.01F;

        const float radius = 0.5F;
        const float spacing = 1.5F;
        const float max_velocity = 4.0F;

        const auto grid_size = static_cast<std::size_t>(
            std::ceil(std::sqrt(static_cast<double>(config.bodies))));
        const float half_extent = spacing * static_cast<float>(grid_size) * 0.5F;

        std::mt19937 generator{config.seed};
        std::uniform_real_distribution<float> velocity_distribution{-max_velocity, max_velocity};

        for (std::size_t index = 0; index < config.bodies; ++index)
        {
            const std::size_t row = index / grid_size;
            const std::size_t column = index % grid_size;
            engine::physics::RigidBody body{};
            body.mass = 1.0F;
            body.position = engine::math::vec3{
                static_cast<float>(column) * spacing - half_extent,
                0.0F,
                static_cast<float>(row) * spacing - half_extent
            };
            body.velocity = engine::math::vec3{
                velocity_distribution(generator), 0.0F,
                velocity_distribution(generator)
            };
            body.collider = engine::physics::Collider::make_sphere(radius);
            (void)engine::physics::add_body(world, body);
        }

        const float wall_half_extent = half_extent + radius;
        engine::physics::RigidBody west_wall{};
        west_wall.mass = 0.0F;
        west_wall.position = engine::math::vec3{-wall_half_extent, 0.0F, 0.0F};
        west_wall.collider = engine::physics::Collider::make_aabb(
            engine::geometry::MakeAabbFromCenterExtent(engine::math::vec3{0.0F, 0.0F, 0.0F},
                                                       engine::math::vec3{radius, radius, wall_half_extent + radius}));
        (void)engine::physics::add_body(world, west_wall);

        engine::physics::RigidBody east_wall = west_wall;
        east_wall.position[0] = wall_half_extent;
        (void)engine::physics::add_body(world, east_wall);

        engine::physics::RigidBody north_wall{};
        north_wall.mass = 0.0F;
        north_wall.position = engine::math::vec3{0.0F, 0.0F, wall_half_extent};
        north_wall.collider = engine::physics::Collider::make_aabb(
            engine::geometry::MakeAabbFromCenterExtent(engine::math::vec3{0.0F, 0.0F, 0.0F},
                                                       engine::math::vec3{wall_half_extent + radius, radius, radius}));
        (void)engine::physics::add_body(world, north_wall);

        engine::physics::RigidBody south_wall = north_wall;
        south_wall.position[2] = -wall_half_extent;
        (void)engine::physics::add_body(world, south_wall);

        return world;
    }

    void clamp_to_bounds(engine::physics::PhysicsWorld& world, float half_extent) noexcept
    {
        const std::size_t body_total = engine::physics::body_count(world);
        for (std::size_t index = 0; index < body_total; ++index)
        {
            auto& body = engine::physics::body_at(world, index);
            if (body.inverse_mass == 0.0F)
            {
                continue;
            }

            if (body.position[0] < -half_extent)
            {
                body.position[0] = -half_extent;
                body.velocity[0] = std::fabs(body.velocity[0]);
            }
            else if (body.position[0] > half_extent)
            {
                body.position[0] = half_extent;
                body.velocity[0] = -std::fabs(body.velocity[0]);
            }

            if (body.position[2] < -half_extent)
            {
                body.position[2] = -half_extent;
                body.velocity[2] = std::fabs(body.velocity[2]);
            }
            else if (body.position[2] > half_extent)
            {
                body.position[2] = half_extent;
                body.velocity[2] = -std::fabs(body.velocity[2]);
            }
        }
    }

    struct BenchmarkMetrics
    {
        double duration_seconds{0.0};
        double average_manifolds{0.0};
        double average_contacts{0.0};
        double average_solver_iterations{0.0};
        std::size_t peak_manifolds{0U};
        std::size_t peak_contacts{0U};
        std::uint32_t peak_solver_iterations{0U};
        float max_penetration{0.0F};
    };

    [[nodiscard]] BenchmarkMetrics run_benchmark(const BenchmarkConfig& config)
    {
        auto world = make_world(config);

        const float spacing = 1.5F;
        const auto grid_size = static_cast<std::size_t>(
            std::ceil(std::sqrt(static_cast<double>(config.bodies))));
        const float half_extent = spacing * static_cast<float>(grid_size) * 0.5F + 0.5F;

        double accumulated_manifolds = 0.0;
        double accumulated_contacts = 0.0;
        double accumulated_solver_iterations = 0.0;
        std::size_t peak_manifolds = 0U;
        std::size_t peak_contacts = 0U;
        std::uint32_t peak_solver_iterations = 0U;
        float max_penetration = 0.0F;

        const auto start = std::chrono::steady_clock::now();
        for (std::size_t step = 0; step < config.steps; ++step)
        {
            engine::physics::integrate(world, static_cast<double>(config.dt));
            clamp_to_bounds(world, half_extent);
            engine::physics::update_contact_manifolds(world);

            const auto& telemetry = engine::physics::collision_telemetry(world);
            accumulated_manifolds += static_cast<double>(telemetry.manifold_count);
            accumulated_contacts += static_cast<double>(telemetry.contact_count);
            accumulated_solver_iterations += static_cast<double>(telemetry.solver_iterations);
            peak_manifolds = std::max(peak_manifolds, telemetry.manifold_count);
            peak_contacts = std::max(peak_contacts, telemetry.contact_count);
            peak_solver_iterations = std::max(peak_solver_iterations, telemetry.solver_iterations);
            max_penetration = std::max(max_penetration, telemetry.max_penetration);
        }
        const auto end = std::chrono::steady_clock::now();

        BenchmarkMetrics metrics{};
        metrics.duration_seconds = std::chrono::duration<double>(end - start).count();
        if (config.steps > 0U)
        {
            metrics.average_manifolds = accumulated_manifolds / static_cast<double>(config.steps);
            metrics.average_contacts = accumulated_contacts / static_cast<double>(config.steps);
            metrics.average_solver_iterations = accumulated_solver_iterations / static_cast<double>(config.steps);
        }
        metrics.peak_manifolds = peak_manifolds;
        metrics.peak_contacts = peak_contacts;
        metrics.peak_solver_iterations = peak_solver_iterations;
        metrics.max_penetration = max_penetration;
        return metrics;
    }

    [[nodiscard]] std::string serialize_metrics(const BenchmarkConfig& config, const BenchmarkMetrics& metrics)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(6);
        const double steps_per_second = (metrics.duration_seconds > 0.0)
                                            ? static_cast<double>(config.steps) / metrics.duration_seconds
                                            : 0.0;

        stream << "{\n";
        stream << "  \"benchmark\":\"physics_collision_throughput\",\n";
        stream << "  \"config\":{\n";
        stream << "    \"bodies\":" << static_cast<unsigned long long>(config.bodies) << ",\n";
        stream << "    \"steps\":" << static_cast<unsigned long long>(config.steps) << ",\n";
        stream << "    \"dt\":" << static_cast<double>(config.dt) << ",\n";
        stream << "    \"seed\":" << static_cast<unsigned long long>(config.seed) << "\n";
        stream << "  },\n";
        stream << "  \"metrics\":{\n";
        stream << "    \"duration_seconds\":" << metrics.duration_seconds << ",\n";
        stream << "    \"steps_per_second\":" << steps_per_second << ",\n";
        stream << "    \"average_manifolds\":" << metrics.average_manifolds << ",\n";
        stream << "    \"average_contacts\":" << metrics.average_contacts << ",\n";
        stream << "    \"average_solver_iterations\":" << metrics.average_solver_iterations << ",\n";
        stream << "    \"peak_manifolds\":" << static_cast<unsigned long long>(metrics.peak_manifolds) << ",\n";
        stream << "    \"peak_contacts\":" << static_cast<unsigned long long>(metrics.peak_contacts) << ",\n";
        stream << "    \"peak_solver_iterations\":" << static_cast<unsigned long long>(metrics.peak_solver_iterations)
            << ",\n";
        stream << "    \"max_penetration\":" << static_cast<double>(metrics.max_penetration) << "\n";
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
        std::cerr << "physics_collision_benchmark: " << parse.error << '\n';
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