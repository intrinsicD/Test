#include <chrono>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/geometry/api.hpp"

namespace
{
    struct BenchmarkConfig
    {
        std::size_t resolution{256U};
        std::size_t iterations{128U};
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
        return "geometry_normals_benchmark [--resolution <quads-per-axis>] [--iterations <count>] [--output <path>]";
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
            if (argument == "--resolution")
            {
                if (index + 1 >= argc)
                {
                    result.ok = false;
                    result.error = "--resolution requires a value";
                    return result;
                }
                ++index;
                std::size_t value = 0U;
                if (!parse_unsigned(argv[index], value) || value == 0U)
                {
                    result.ok = false;
                    result.error = "--resolution must be a positive integer";
                    return result;
                }
                result.config.resolution = value;
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

    [[nodiscard]] engine::geometry::SurfaceMesh make_grid(std::size_t resolution)
    {
        const std::size_t vertices_per_axis = resolution + 1U;
        const std::size_t vertex_count = vertices_per_axis * vertices_per_axis;
        const std::size_t quad_count = resolution * resolution;
        const std::size_t triangle_count = quad_count * 2U;

        engine::geometry::SurfaceMesh mesh{};
        mesh.rest_positions.resize(vertex_count);
        mesh.positions.resize(vertex_count);
        mesh.normals.assign(vertex_count, engine::math::vec3{0.0F, 1.0F, 0.0F});
        mesh.indices.resize(triangle_count * 3U);

        const float step = 1.0F / static_cast<float>(resolution);
        std::size_t vertex_index = 0U;
        for (std::size_t row = 0; row < vertices_per_axis; ++row)
        {
            for (std::size_t column = 0; column < vertices_per_axis; ++column)
            {
                const float x = (static_cast<float>(column) * step) - 0.5F;
                const float z = (static_cast<float>(row) * step) - 0.5F;
                engine::math::vec3 position{x, 0.0F, z};
                mesh.rest_positions[vertex_index] = position;
                mesh.positions[vertex_index] = position;
                ++vertex_index;
            }
        }

        std::size_t index_cursor = 0U;
        for (std::size_t row = 0; row < resolution; ++row)
        {
            for (std::size_t column = 0; column < resolution; ++column)
            {
                const std::size_t top_left = row * vertices_per_axis + column;
                const std::size_t top_right = top_left + 1U;
                const std::size_t bottom_left = (row + 1U) * vertices_per_axis + column;
                const std::size_t bottom_right = bottom_left + 1U;

                mesh.indices[index_cursor++] = static_cast<std::uint32_t>(top_left);
                mesh.indices[index_cursor++] = static_cast<std::uint32_t>(bottom_left);
                mesh.indices[index_cursor++] = static_cast<std::uint32_t>(top_right);

                mesh.indices[index_cursor++] = static_cast<std::uint32_t>(top_right);
                mesh.indices[index_cursor++] = static_cast<std::uint32_t>(bottom_left);
                mesh.indices[index_cursor++] = static_cast<std::uint32_t>(bottom_right);
            }
        }

        engine::geometry::update_bounds(mesh);
        return mesh;
    }

    struct BenchmarkMetrics
    {
        double duration_seconds{0.0};
        double iterations_per_second{0.0};
        double vertices_per_second{0.0};
        double triangles_per_second{0.0};
        double normal_checksum{0.0};
        std::size_t vertex_count{0U};
        std::size_t triangle_count{0U};
    };

    [[nodiscard]] BenchmarkMetrics run_benchmark(const BenchmarkConfig& config)
    {
        auto mesh = make_grid(config.resolution);
        const std::size_t vertex_count = mesh.positions.size();
        const std::size_t triangle_count = mesh.indices.size() / 3U;

        const auto start = std::chrono::steady_clock::now();
        for (std::size_t iteration = 0; iteration < config.iterations; ++iteration)
        {
            engine::geometry::recompute_vertex_normals(mesh);
        }
        const auto end = std::chrono::steady_clock::now();

        double checksum = 0.0;
        for (const auto& normal : mesh.normals)
        {
            checksum += static_cast<double>(normal[0] + normal[1] + normal[2]);
        }

        const double duration_seconds = std::chrono::duration<double>(end - start).count();
        const double iterations_per_second = duration_seconds > 0.0
                                                 ? static_cast<double>(config.iterations) / duration_seconds
                                                 : 0.0;
        const double vertices_per_second = duration_seconds > 0.0
                                               ? static_cast<double>(vertex_count * config.iterations) /
                                               duration_seconds
                                               : 0.0;
        const double triangles_per_second = duration_seconds > 0.0
                                                ? static_cast<double>(triangle_count * config.iterations) /
                                                duration_seconds
                                                : 0.0;

        BenchmarkMetrics metrics{};
        metrics.duration_seconds = duration_seconds;
        metrics.iterations_per_second = iterations_per_second;
        metrics.vertices_per_second = vertices_per_second;
        metrics.triangles_per_second = triangles_per_second;
        metrics.normal_checksum = checksum;
        metrics.vertex_count = vertex_count;
        metrics.triangle_count = triangle_count;
        return metrics;
    }

    [[nodiscard]] std::string serialize_metrics(const BenchmarkConfig& config, const BenchmarkMetrics& metrics)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(6);

        stream << "{\n";
        stream << "  \"benchmark\":\"geometry_normal_recompute\",\n";
        stream << "  \"config\":{\n";
        stream << "    \"resolution\":" << static_cast<unsigned long long>(config.resolution) << ",\n";
        stream << "    \"iterations\":" << static_cast<unsigned long long>(config.iterations) << ",\n";
        stream << "    \"vertex_count\":" << static_cast<unsigned long long>(metrics.vertex_count) << ",\n";
        stream << "    \"triangle_count\":" << static_cast<unsigned long long>(metrics.triangle_count) << "\n";
        stream << "  },\n";
        stream << "  \"metrics\":{\n";
        stream << "    \"duration_seconds\":" << metrics.duration_seconds << ",\n";
        stream << "    \"iterations_per_second\":" << metrics.iterations_per_second << ",\n";
        stream << "    \"vertices_per_second\":" << metrics.vertices_per_second << ",\n";
        stream << "    \"triangles_per_second\":" << metrics.triangles_per_second << ",\n";
        stream << "    \"normal_checksum\":" << metrics.normal_checksum << "\n";
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
        std::cerr << "geometry_normals_benchmark: " << parse.error << '\n';
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