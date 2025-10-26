#include "remesh_cli.hpp"

#include "engine/geometry/api.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

namespace engine::geometry::tools
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string_view value) noexcept
        {
            std::string lowered{value};
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return lowered;
        }

        [[nodiscard]] bool parse_float(std::string_view text, float& out_value) noexcept
        {
            const auto* begin = text.data();
            const auto* end = begin + text.size();
            const auto result = std::from_chars(begin, end, out_value);
            return result.ec == std::errc{} && result.ptr == end;
        }

        [[nodiscard]] bool parse_uint(std::string_view text, std::uint32_t& out_value) noexcept
        {
            const auto* begin = text.data();
            const auto* end = begin + text.size();
            const auto result = std::from_chars(begin, end, out_value);
            return result.ec == std::errc{} && result.ptr == end;
        }

        [[nodiscard]] RemeshingMode parse_mode(std::string_view value, bool& recognised) noexcept
        {
            const std::string lowered = to_lower(value);
            if (lowered == "uniform")
            {
                recognised = true;
                return RemeshingMode::kUniform;
            }
            if (lowered == "feature" || lowered == "feature-preserving" || lowered == "feature_preserving")
            {
                recognised = true;
                return RemeshingMode::kFeaturePreserving;
            }
            if (lowered == "adaptive")
            {
                recognised = true;
                return RemeshingMode::kAdaptive;
            }

            recognised = false;
            return RemeshingMode::kUniform;
        }

        [[nodiscard]] ParameterizationMode parse_parameterization_mode(std::string_view value,
                                                                       bool& recognised) noexcept
        {
            const std::string lowered = to_lower(value);
            if (lowered == "none")
            {
                recognised = true;
                return ParameterizationMode::kNone;
            }
            if (lowered == "reuse" || lowered == "reuse-existing" || lowered == "reuse_existing")
            {
                recognised = true;
                return ParameterizationMode::kReuseExisting;
            }
            if (lowered == "lscm")
            {
                recognised = true;
                return ParameterizationMode::kGenerateLscm;
            }
            if (lowered == "abfpp" || lowered == "abf++")
            {
                recognised = true;
                return ParameterizationMode::kGenerateAbfpp;
            }

            recognised = false;
            return ParameterizationMode::kNone;
        }

        [[nodiscard]] std::filesystem::path default_output_path(const std::filesystem::path& input)
        {
            const auto directory = input.parent_path();
            std::filesystem::path stem = input.stem();
            stem += "_remeshed";
            auto extension = input.extension();
            if (extension.empty())
            {
                extension = ".obj";
            }
            return directory / (stem.string() + extension.string());
        }

        [[nodiscard]] std::string missing_value_error(std::string_view option)
        {
            std::ostringstream stream;
            stream << "Option '" << option << "' requires a value";
            return stream.str();
        }

        [[nodiscard]] std::string invalid_value_error(std::string_view option, std::string_view value)
        {
            std::ostringstream stream;
            stream << "Option '" << option << "' received invalid value '" << value << "'";
            return stream.str();
        }

        [[nodiscard]] std::string unsupported_mode_error(std::string_view option, std::string_view value)
        {
            std::ostringstream stream;
            stream << "Unsupported value '" << value << "' for option '" << option << "'";
            return stream.str();
        }
    } // namespace

    RemeshCliOptionsResult ParseArguments(std::span<const char* const> arguments) noexcept
    {
        RemeshCliOptions options{};

        if (arguments.empty())
        {
            return options;
        }

        for (std::size_t index = 1; index < arguments.size(); ++index)
        {
            const std::string_view argument{arguments[index]};

            if (argument == "--help" || argument == "-h")
            {
                options.show_help = true;
                return options;
            }

            if (argument == "--input")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                options.input_path = std::filesystem::path{arguments[index]};
                continue;
            }

            if (argument == "--output")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                options.output_path = std::filesystem::path{arguments[index]};
                continue;
            }

            if (argument == "--mode")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                bool recognised = false;
                const RemeshingMode mode = parse_mode(arguments[index], recognised);
                if (!recognised)
                {
                    return RemeshCliOptionsResult{unsupported_mode_error(argument, arguments[index])};
                }
                options.mode = mode;
                continue;
            }

            if (argument == "--target-edge-length")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.targets.target_edge_length = value;
                continue;
            }

            if (argument == "--relative-edge-scale")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.targets.relative_edge_scale = value;
                continue;
            }

            if (argument == "--max-normal-deviation")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.targets.maximum_normal_deviation_degrees = value;
                continue;
            }

            if (argument == "--max-surface-deviation")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.targets.maximum_surface_deviation = value;
                continue;
            }

            if (argument == "--max-iterations")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                std::uint32_t value = 0U;
                if (!parse_uint(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.max_iterations = value;
                continue;
            }

            if (argument == "--relaxation-factor")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.relaxation_factor = value;
                continue;
            }

            if (argument == "--tangential-smoothing-weight")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.tangential_smoothing_weight = value;
                continue;
            }

            if (argument == "--no-tangential-smoothing")
            {
                options.tangential_smoothing_weight = 0.0F;
                continue;
            }

            if (argument == "--lock-boundary-edges")
            {
                options.feature_preservation.lock_boundary_edges = true;
                continue;
            }

            if (argument == "--unlock-boundary-edges")
            {
                options.feature_preservation.lock_boundary_edges = false;
                continue;
            }

            if (argument == "--lock-feature-edges")
            {
                options.feature_preservation.lock_feature_edges = true;
                continue;
            }

            if (argument == "--unlock-feature-edges")
            {
                options.feature_preservation.lock_feature_edges = false;
                continue;
            }

            if (argument == "--feature-angle")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.feature_preservation.minimum_feature_angle_degrees = value;
                continue;
            }

            if (argument == "--parameterization")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                bool recognised = false;
                const ParameterizationMode mode = parse_parameterization_mode(arguments[index], recognised);
                if (!recognised)
                {
                    return RemeshCliOptionsResult{unsupported_mode_error(argument, arguments[index])};
                }
                options.parameterization.mode = mode;
                continue;
            }

            if (argument == "--target-texel-density")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.parameterization.target_texel_density = value;
                continue;
            }

            if (argument == "--gutter-width")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                float value = 0.0F;
                if (!parse_float(arguments[index], value))
                {
                    return RemeshCliOptionsResult{invalid_value_error(argument, arguments[index])};
                }
                options.parameterization.gutter_width = value;
                continue;
            }

            if (argument == "--no-repack-islands")
            {
                options.parameterization.repack_islands = false;
                continue;
            }

            if (argument == "--repack-islands")
            {
                options.parameterization.repack_islands = true;
                continue;
            }

            if (argument == "--no-chart-reuse")
            {
                options.parameterization.allow_chart_reuse = false;
                continue;
            }

            if (argument == "--chart-reuse")
            {
                options.parameterization.allow_chart_reuse = true;
                continue;
            }

            if (argument == "--job-label")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                options.job_label = std::string{arguments[index]};
                continue;
            }

            if (argument == "--no-diagnostics")
            {
                options.record_diagnostics = false;
                continue;
            }

            if (argument == "--diagnostics")
            {
                options.record_diagnostics = true;
                continue;
            }

            if (argument == "--verbose")
            {
                options.verbose = true;
                continue;
            }

            return RemeshCliOptionsResult{std::string{"Unknown option '"} + std::string{argument} + "'"};
        }

        if (options.show_help)
        {
            return options;
        }

        if (options.input_path.empty())
        {
            return RemeshCliOptionsResult{"Missing required --input path"};
        }

        if (options.output_path.empty())
        {
            options.output_path = default_output_path(options.input_path);
        }

        if ((options.mode == RemeshingMode::kUniform || options.mode == RemeshingMode::kFeaturePreserving) &&
            !options.targets.target_edge_length.has_value() && !options.targets.relative_edge_scale.has_value())
        {
            return RemeshCliOptionsResult{
                "Uniform and feature-preserving remeshing require --target-edge-length or --relative-edge-scale"};
        }

        if (options.mode == RemeshingMode::kAdaptive)
        {
            const bool has_primary = options.targets.target_edge_length.has_value() ||
                                     options.targets.relative_edge_scale.has_value();
            const bool has_budget = options.targets.maximum_normal_deviation_degrees.has_value() ||
                                    options.targets.maximum_surface_deviation.has_value();
            if (!has_budget)
            {
                return RemeshCliOptionsResult{
                    "Adaptive remeshing requires an error budget (--max-normal-deviation or --max-surface-deviation)"};
            }
            if (!has_primary)
            {
                options.targets.relative_edge_scale = 1.0F;
            }
        }

        return options;
    }

    RemeshCliExecution ExecuteRemesh(const RemeshCliOptions& options) noexcept
    {
        RemeshCliExecutionResult summary{};

        SurfaceMesh input_mesh{};
        try
        {
            input_mesh = load_surface_mesh(options.input_path);
        }
        catch (const std::exception& exception)
        {
            return RemeshCliExecution{std::string{"Failed to load input mesh: "}.append(exception.what())};
        }

        summary.input_vertex_count = input_mesh.positions.size();
        summary.input_face_count = input_mesh.indices.size() / 3U;
        summary.input_edge_statistics = ComputeMeshEdgeStatistics(input_mesh);

        RemeshRequest request{};
        request.input_mesh = &input_mesh;
        request.mode = options.mode;
        request.targets = options.targets;
        request.feature_preservation = options.feature_preservation;
        request.parameterization = options.parameterization;
        request.max_iterations = options.max_iterations;
        request.relaxation_factor = options.relaxation_factor;
        request.tangential_smoothing_weight = options.tangential_smoothing_weight;
        request.record_diagnostics = options.record_diagnostics;
        request.job_label = options.job_label;

        const RemeshResult<RemeshOutput> remesh_result = Remesh(request);
        if (!remesh_result.has_value())
        {
            const RemeshErrorCode error = remesh_result.error();
            std::string message{"Remeshing failed: "};
            message.append(error.message());
            return RemeshCliExecution{std::move(message)};
        }

        summary.output = remesh_result.value();

        try
        {
            save_surface_mesh(summary.output.mesh, options.output_path);
        }
        catch (const std::exception& exception)
        {
            return RemeshCliExecution{std::string{"Failed to save remeshed output: "}.append(exception.what())};
        }

        return RemeshCliExecution{summary};
    }

    void PrintSummary(const RemeshCliOptions& options,
                      const RemeshCliExecutionResult& result,
                      std::ostream& stream) noexcept
    {
        const auto output_vertices = result.output.mesh.positions.size();
        const auto output_faces = result.output.mesh.indices.size() / 3U;
        const MeshEdgeStatistics output_edge_statistics = ComputeMeshEdgeStatistics(result.output.mesh);

        stream << "Remeshed '" << options.input_path.string() << "' -> '" << options.output_path.string() << "'\n";
        stream << "  Input:  vertices=" << result.input_vertex_count << ", faces=" << result.input_face_count << "\n";
        stream << "  Output: vertices=" << output_vertices << ", faces=" << output_faces << "\n";

        stream << std::fixed << std::setprecision(4);
        stream << "  Edge length (input):  min=" << result.input_edge_statistics.min_edge_length
               << " max=" << result.input_edge_statistics.max_edge_length
               << " mean=" << result.input_edge_statistics.mean_edge_length() << "\n";
        stream << "  Edge length (output): min=" << output_edge_statistics.min_edge_length
               << " max=" << output_edge_statistics.max_edge_length
               << " mean=" << output_edge_statistics.mean_edge_length() << "\n";

        const RemeshStatistics& statistics = result.output.statistics;
        stream << "  Iterations=" << statistics.iteration_count
               << " splits/collapses recorded in telemetry" << "\n";
        stream << "  Metrics: min_edge=" << statistics.min_edge_length
               << " max_edge=" << statistics.max_edge_length
               << " max_error=" << statistics.max_error << "\n";

        if (options.job_label.has_value())
        {
            stream << "  Job label: " << options.job_label.value() << "\n";
        }

        if (options.verbose)
        {
            if (options.targets.target_edge_length.has_value())
            {
                stream << "  Target edge length: " << options.targets.target_edge_length.value() << "\n";
            }
            if (options.targets.relative_edge_scale.has_value())
            {
                stream << "  Relative edge scale: " << options.targets.relative_edge_scale.value() << "\n";
            }
            if (options.targets.maximum_normal_deviation_degrees.has_value())
            {
                stream << "  Max normal deviation: "
                       << options.targets.maximum_normal_deviation_degrees.value() << " deg\n";
            }
            if (options.targets.maximum_surface_deviation.has_value())
            {
                stream << "  Max surface deviation: " << options.targets.maximum_surface_deviation.value() << "\n";
            }
        }

        if (options.parameterization.mode != ParameterizationMode::kNone)
        {
            const ParameterizationSummary& summary = result.output.parameterization;
            stream << "  Parameterization: charts=" << summary.chart_count
                   << " texel_density=" << summary.texel_density
                   << " avg_stretch=" << summary.average_stretch
                   << " max_stretch=" << summary.max_stretch << "\n";

            if (options.verbose)
            {
                for (std::size_t chart_index = 0; chart_index < summary.charts.size(); ++chart_index)
                {
                    const ParameterizationChart& chart = summary.charts[chart_index];
                    stream << "    Chart " << chart_index
                           << ": min_uv=(" << chart.min_uv[0] << ", " << chart.min_uv[1] << ")"
                           << " max_uv=(" << chart.max_uv[0] << ", " << chart.max_uv[1] << ")"
                           << " translation=(" << chart.translation[0] << ", " << chart.translation[1] << ")"
                           << " scale=" << chart.scale
                           << " area=" << chart.area << "\n";
                }
            }
        }
    }

    void PrintHelp(std::ostream& stream) noexcept
    {
        stream << "geometry_remesh — offline remeshing pipeline for SurfaceMesh assets\n\n";
        stream << "Usage:\n";
        stream << "  geometry_remesh --input <mesh.obj> [options]\n\n";
        stream << "Options:\n";
        stream << "  --input <path>                  Path to the source mesh (OBJ).\n";
        stream << "  --output <path>                 Output mesh path (defaults to <input>_remeshed.obj).\n";
        stream << "  --mode <uniform|feature|adaptive>\n";
        stream << "                                   Remeshing mode (default: uniform).\n";
        stream << "  --target-edge-length <value>    Absolute target edge length in world units.\n";
        stream << "  --relative-edge-scale <value>   Scale relative to the input mean edge length.\n";
        stream << "  --max-normal-deviation <deg>    Maximum allowed normal deviation (adaptive mode).\n";
        stream << "  --max-surface-deviation <value> Maximum allowed surface deviation (adaptive mode).\n";
        stream << "  --max-iterations <count>        Maximum remeshing iterations (default: 64).\n";
        stream << "  --relaxation-factor <value>     Relaxation factor in (0, 1] (default: 0.6).\n";
        stream << "  --tangential-smoothing-weight <value>  Tangential smoothing weight in [0, 1].\n";
        stream << "  --no-tangential-smoothing       Disable tangential smoothing.\n";
        stream << "  --lock-boundary-edges           Lock boundary edges (default).\n";
        stream << "  --unlock-boundary-edges         Allow boundary vertices to move.\n";
        stream << "  --lock-feature-edges            Lock feature edges (default).\n";
        stream << "  --unlock-feature-edges          Allow feature edges to relax.\n";
        stream << "  --feature-angle <deg>           Minimum dihedral angle treated as a feature (default: 45).\n";
        stream << "  --parameterization <mode>       none|reuse|lscm|abfpp.\n";
        stream << "  --target-texel-density <value>  Target texel density when generating parameterisation.\n";
        stream << "  --gutter-width <value>          UV gutter width when repacking charts.\n";
        stream << "  --no-repack-islands             Preserve original island layout.\n";
        stream << "  --no-chart-reuse                Force repacking existing UV charts.\n";
        stream << "  --job-label <label>             Label recorded in remeshing telemetry.\n";
        stream << "  --no-diagnostics                Skip telemetry recording for this run.\n";
        stream << "  --verbose                       Print additional statistics.\n";
        stream << "  --help                          Display this message.\n";
    }
}

