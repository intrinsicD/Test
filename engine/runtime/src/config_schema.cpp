#include "engine/runtime/config_schema.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace engine::runtime::config
{
    namespace
    {
        using engine::runtime::RuntimeError;
        using engine::runtime::RuntimeErrorCode;

        class SchemaValidationError : public std::runtime_error
        {
        public:
            explicit SchemaValidationError(std::string message)
                : std::runtime_error{std::move(message)}
            {
            }
        };

        [[nodiscard]] RuntimeErrorCode make_validation_error(std::string message)
        {
            return make_runtime_error(RuntimeError::configuration_validation_error, std::move(message));
        }

        [[nodiscard]] RuntimeErrorCode make_parse_error(const std::filesystem::path& path, std::string message)
        {
            std::ostringstream builder;
            builder << "Failed to parse configuration '" << path.string() << "': " << message;
            return make_runtime_error(RuntimeError::configuration_parse_error, builder.str());
        }

        [[nodiscard]] RuntimeErrorCode make_io_error(const std::filesystem::path& path, std::string message)
        {
            std::ostringstream builder;
            builder << "Failed to read configuration '" << path.string() << "': " << message;
            return make_runtime_error(RuntimeError::configuration_io_error, builder.str());
        }

        [[nodiscard]] std::string join_context(std::string_view base, std::string_view suffix)
        {
            if (base.empty())
            {
                return std::string{suffix};
            }
            if (!suffix.empty() && suffix.front() == '[')
            {
                std::string result;
                result.reserve(base.size() + suffix.size());
                result.append(base);
                result.append(suffix);
                return result;
            }
            std::string result;
            result.reserve(base.size() + suffix.size() + 1U);
            result.append(base);
            result.push_back('.');
            result.append(suffix);
            return result;
        }

        [[nodiscard]] std::string index_context(std::string_view base, std::size_t index)
        {
            std::string result;
            const auto index_text = std::to_string(index);
            result.reserve(base.size() + index_text.size() + 2U);
            result.append(base);
            result.push_back('[');
            result.append(index_text);
            result.push_back(']');
            return result;
        }

        [[noreturn]] void throw_validation(std::string_view context, std::string_view message)
        {
            std::ostringstream builder;
            if (!context.empty())
            {
                builder << context << ": ";
            }
            builder << message;
            throw SchemaValidationError{builder.str()};
        }

        [[nodiscard]] bool is_truthy(std::string_view value)
        {
            std::string lowered;
            lowered.reserve(value.size());
            std::transform(value.begin(), value.end(), std::back_inserter(lowered), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            static const std::unordered_set<std::string, std::hash<std::string>, std::equal_to<>> kTruthy{
                "1", "true", "on", "yes", "enable", "enabled"};
            return kTruthy.count(lowered) > 0U;
        }

        [[nodiscard]] bool is_schema_enforced(std::optional<bool> override)
        {
            if (override.has_value())
            {
                return override.value();
            }
            const char* value = std::getenv("ENGINE_AI004_SCHEMA_V1");
            if (value == nullptr)
            {
                return false;
            }
            return is_truthy(value);
        }

        [[nodiscard]] const YAML::Node require_mapping(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsMap())
            {
                throw_validation(context, "must be a mapping");
            }
            return node;
        }

        [[nodiscard]] const YAML::Node require_sequence(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsSequence())
            {
                throw_validation(context, "must be a sequence");
            }
            return node;
        }

        [[nodiscard]] std::string require_string(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsScalar())
            {
                throw_validation(context, "must be a string");
            }
            const auto value = node.as<std::string>();
            if (value.empty())
            {
                throw_validation(context, "must not be empty");
            }
            return value;
        }

        [[nodiscard]] std::string require_slug(const YAML::Node& node, std::string_view context)
        {
            const auto value = require_string(node, context);
            if (value.empty())
            {
                throw_validation(context, "must contain lowercase alphanumeric characters separated by hyphens");
            }
            if (value.front() == '-' || value.back() == '-')
            {
                throw_validation(context, "must not start or end with a hyphen");
            }
            bool has_alphanumeric = false;
            for (char ch : value)
            {
                if (std::isalnum(static_cast<unsigned char>(ch)))
                {
                    if (std::isalpha(static_cast<unsigned char>(ch)) &&
                        !std::islower(static_cast<unsigned char>(ch)))
                    {
                        throw_validation(context, "must contain lowercase alphanumeric characters separated by hyphens");
                    }
                    has_alphanumeric = true;
                    continue;
                }
                if (ch != '-')
                {
                    throw_validation(context, "must contain lowercase alphanumeric characters separated by hyphens");
                }
            }
            if (!has_alphanumeric)
            {
                throw_validation(context, "must include at least one alphanumeric character");
            }
            return value;
        }

        [[nodiscard]] bool require_bool(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsScalar())
            {
                throw_validation(context, "must be a boolean");
            }
            try
            {
                return node.as<bool>();
            }
            catch (const YAML::BadConversion&)
            {
                throw_validation(context, "must be a boolean");
            }
        }

        [[nodiscard]] int require_int(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsScalar())
            {
                throw_validation(context, "must be an integer");
            }
            try
            {
                return node.as<int>();
            }
            catch (const YAML::BadConversion&)
            {
                throw_validation(context, "must be an integer");
            }
        }

        [[nodiscard]] std::uint64_t require_non_negative_uint64(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsScalar())
            {
                throw_validation(context, "must be a non-negative integer");
            }
            try
            {
                const auto value = node.as<long long>();
                if (value < 0)
                {
                    throw_validation(context, "must be non-negative");
                }
                return static_cast<std::uint64_t>(value);
            }
            catch (const YAML::BadConversion&)
            {
                throw_validation(context, "must be a non-negative integer");
            }
        }

        [[nodiscard]] int require_positive_int(const YAML::Node& node, std::string_view context)
        {
            const auto value = require_int(node, context);
            if (value <= 0)
            {
                throw_validation(context, "must be greater than zero");
            }
            return value;
        }

        [[nodiscard]] double require_float(const YAML::Node& node, std::string_view context)
        {
            if (!node || !node.IsScalar())
            {
                throw_validation(context, "must be a number");
            }
            try
            {
                const auto value = node.as<double>();
                if (!std::isfinite(value))
                {
                    throw_validation(context, "must be a finite number");
                }
                return value;
            }
            catch (const YAML::BadConversion&)
            {
                throw_validation(context, "must be a finite number");
            }
        }

        [[nodiscard]] double require_positive_float(const YAML::Node& node, std::string_view context)
        {
            const auto value = require_float(node, context);
            if (value <= 0.0)
            {
                throw_validation(context, "must be greater than zero");
            }
            return value;
        }

        [[nodiscard]] double require_non_negative_float(const YAML::Node& node, std::string_view context)
        {
            const auto value = require_float(node, context);
            if (value < 0.0)
            {
                throw_validation(context, "must be non-negative");
            }
            return value;
        }

        [[nodiscard]] std::array<double, 2> require_vec2(const YAML::Node& node, std::string_view context)
        {
            const auto sequence = require_sequence(node, context);
            if (sequence.size() != 2U)
            {
                throw_validation(context, "must contain exactly two elements");
            }
            return {require_float(sequence[0], join_context(context, "[0]")),
                    require_float(sequence[1], join_context(context, "[1]"))};
        }

        [[nodiscard]] std::array<double, 3> require_vec3(const YAML::Node& node, std::string_view context)
        {
            const auto sequence = require_sequence(node, context);
            if (sequence.size() != 3U)
            {
                throw_validation(context, "must contain exactly three elements");
            }
            return {require_float(sequence[0], join_context(context, "[0]")),
                    require_float(sequence[1], join_context(context, "[1]")),
                    require_float(sequence[2], join_context(context, "[2]"))};
        }

        [[nodiscard]] std::string require_sha256(const YAML::Node& node, std::string_view context)
        {
            const auto value = require_string(node, context);
            if (value.size() != 64U)
            {
                throw_validation(context, "must contain a 64-character SHA-256 digest");
            }
            if (!std::all_of(value.begin(), value.end(), [](unsigned char ch) {
                    return std::isxdigit(ch) != 0;
                }))
            {
                throw_validation(context, "must contain only hexadecimal characters");
            }
            std::string lowered;
            lowered.reserve(value.size());
            std::transform(value.begin(), value.end(), std::back_inserter(lowered), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return lowered;
        }

        struct SchemaHeader
        {
            std::string id;
            int version = 1;
        };

        [[nodiscard]] SchemaHeader parse_schema_header(const YAML::Node& node,
                                                       std::string_view context,
                                                       std::string_view expected_id,
                                                       bool enforce_schema)
        {
            const YAML::Node schema_node = node["schema"];
            if (!schema_node)
            {
                if (enforce_schema)
                {
                    throw_validation(join_context(context, "schema"), "is required when schema enforcement is enabled");
                }
                return SchemaHeader{std::string{expected_id}, 1};
            }

            const auto schema = require_mapping(schema_node, join_context(context, "schema"));
            std::string schema_id = std::string{expected_id};
            if (const auto id_node = schema["id"]; id_node)
            {
                schema_id = require_string(id_node, join_context(context, "schema.id"));
            }
            else if (enforce_schema)
            {
                throw_validation(join_context(context, "schema.id"), "is required when schema enforcement is enabled");
            }

            if (schema_id != expected_id)
            {
                std::ostringstream builder;
                builder << "must be '" << expected_id << "'; received '" << schema_id << "'";
                throw_validation(join_context(context, "schema.id"), builder.str());
            }

            int version = 1;
            if (const auto version_node = schema["version"]; version_node)
            {
                version = require_int(version_node, join_context(context, "schema.version"));
            }
            else if (enforce_schema)
            {
                throw_validation(join_context(context, "schema.version"), "is required when schema enforcement is enabled");
            }

            if (version < 1)
            {
                throw_validation(join_context(context, "schema.version"), "must be >= 1");
            }

            return SchemaHeader{std::string{expected_id}, version};
        }

        [[nodiscard]] RemeshingTargets parse_remeshing_targets(const YAML::Node& node, std::string_view context)
        {
            RemeshingTargets result;
            if (const auto value = node["target_edge_length"]; value)
            {
                result.target_edge_length = require_float(value, join_context(context, "target_edge_length"));
            }
            if (const auto value = node["relative_edge_scale"]; value)
            {
                result.relative_edge_scale = require_float(value, join_context(context, "relative_edge_scale"));
            }
            if (const auto value = node["max_normal_deviation_degrees"]; value)
            {
                result.max_normal_deviation_degrees =
                    require_float(value, join_context(context, "max_normal_deviation_degrees"));
            }
            if (const auto value = node["max_surface_deviation"]; value)
            {
                result.max_surface_deviation = require_float(value, join_context(context, "max_surface_deviation"));
            }
            return result;
        }

        [[nodiscard]] FeaturePreservation parse_feature_preservation(const YAML::Node& node, std::string_view context)
        {
            FeaturePreservation result;
            result.lock_boundary_edges = require_bool(node["lock_boundary_edges"],
                                                      join_context(context, "lock_boundary_edges"));
            result.lock_feature_edges =
                require_bool(node["lock_feature_edges"], join_context(context, "lock_feature_edges"));
            result.minimum_feature_angle_degrees = require_float(node["minimum_feature_angle_degrees"],
                                                                 join_context(context, "minimum_feature_angle_degrees"));
            return result;
        }

        [[nodiscard]] EdgeLengthMetrics parse_edge_length_metrics(const YAML::Node& node, std::string_view context)
        {
            EdgeLengthMetrics result;
            result.minimum = require_float(node["min"], join_context(context, "min"));
            result.maximum = require_float(node["max"], join_context(context, "max"));
            result.mean = require_float(node["mean"], join_context(context, "mean"));
            return result;
        }

        [[nodiscard]] MeshMetrics parse_mesh_metrics(const YAML::Node& node, std::string_view context)
        {
            MeshMetrics metrics;
            metrics.vertices = require_int(node["vertices"], join_context(context, "vertices"));
            metrics.faces = require_int(node["faces"], join_context(context, "faces"));
            metrics.edge_length = parse_edge_length_metrics(require_mapping(node["edge_length"],
                                                                           join_context(context, "edge_length")),
                                                            join_context(context, "edge_length"));
            return metrics;
        }

        [[nodiscard]] ParameterizationSummary parse_parameterization(const YAML::Node& node,
                                                                      std::string_view context)
        {
            ParameterizationSummary summary;
            summary.mode = require_string(node["mode"], join_context(context, "mode"));
            std::string lowered_mode;
            lowered_mode.reserve(summary.mode.size());
            std::transform(summary.mode.begin(), summary.mode.end(), std::back_inserter(lowered_mode),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            static const std::unordered_set<std::string, std::hash<std::string>, std::equal_to<>> kSupportedModes{
                "none", "reuse_existing", "generate_lscm", "generate_abfpp"};
            if (kSupportedModes.count(lowered_mode) == 0U)
            {
                std::ostringstream builder;
                builder << "contains unsupported value '" << summary.mode << "'";
                throw_validation(join_context(context, "mode"), builder.str());
            }
            summary.mode = lowered_mode;

            if (const auto value = node["target_texel_density"]; value)
            {
                summary.target_texel_density = require_float(value, join_context(context, "target_texel_density"));
            }

            summary.texel_density = require_float(node["texel_density"], join_context(context, "texel_density"));
            summary.chart_count = require_int(node["chart_count"], join_context(context, "chart_count"));
            summary.average_stretch = require_float(node["average_stretch"], join_context(context, "average_stretch"));
            summary.max_stretch = require_float(node["max_stretch"], join_context(context, "max_stretch"));
            summary.fill_ratio = require_float(node["fill_ratio"], join_context(context, "fill_ratio"));
            summary.total_seam_length = require_float(node["total_seam_length"], join_context(context, "total_seam_length"));
            if (const auto value = node["atlas_area"]; value)
            {
                summary.atlas_area = require_float(value, join_context(context, "atlas_area"));
            }
            if (const auto value = node["total_chart_area"]; value)
            {
                summary.total_chart_area = require_float(value, join_context(context, "total_chart_area"));
            }

            if (const auto charts_node = node["charts"]; charts_node)
            {
                const auto charts = require_sequence(charts_node, join_context(context, "charts"));
                summary.charts.reserve(charts.size());
                for (std::size_t index = 0; index < charts.size(); ++index)
                {
                    const auto chart_context = index_context(join_context(context, "charts"), index);
                    const auto chart_node = require_mapping(charts[index], chart_context);
                    ParameterizationChart chart;
                    chart.index = require_int(chart_node["index"], join_context(chart_context, "index"));
                    chart.min_uv = require_vec2(chart_node["min_uv"], join_context(chart_context, "min_uv"));
                    chart.max_uv = require_vec2(chart_node["max_uv"], join_context(chart_context, "max_uv"));
                    chart.translation = require_vec2(chart_node["translation"], join_context(chart_context, "translation"));
                    chart.scale = require_float(chart_node["scale"], join_context(chart_context, "scale"));
                    chart.area = require_float(chart_node["area"], join_context(chart_context, "area"));
                    chart.boundary_length =
                        require_float(chart_node["boundary_length"], join_context(chart_context, "boundary_length"));
                    summary.charts.push_back(std::move(chart));
                }
            }

            return summary;
        }

        [[nodiscard]] DatasetStatistics parse_dataset_statistics(const YAML::Node& node, std::string_view context)
        {
            DatasetStatistics stats;
            stats.iteration_count = require_int(node["iterations"], join_context(context, "iterations"));
            stats.max_error = require_float(node["max_error"], join_context(context, "max_error"));
            stats.min_edge_length = require_float(node["min_edge_length"], join_context(context, "min_edge_length"));
            stats.max_edge_length = require_float(node["max_edge_length"], join_context(context, "max_edge_length"));
            stats.max_surface_deviation =
                require_float(node["max_surface_deviation"], join_context(context, "max_surface_deviation"));
            stats.mean_surface_deviation =
                require_float(node["mean_surface_deviation"], join_context(context, "mean_surface_deviation"));
            stats.rms_surface_deviation =
                require_float(node["rms_surface_deviation"], join_context(context, "rms_surface_deviation"));
            return stats;
        }

        [[nodiscard]] DatasetEntry parse_dataset_entry(const YAML::Node& node,
                                                       std::string_view context,
                                                       bool enforce_schema)
        {
            const auto header = parse_schema_header(node, context, "ai-004.dataset", enforce_schema);
            DatasetEntry entry;
            entry.schema_id = header.id;
            entry.schema_version = header.version;
            entry.identifier = require_slug(node["id"], join_context(context, "id"));
            entry.kind = require_string(node["kind"], join_context(context, "kind"));

            const auto tags_node = require_sequence(node["tags"], join_context(context, "tags"));
            entry.tags.reserve(tags_node.size());
            for (std::size_t index = 0; index < tags_node.size(); ++index)
            {
                entry.tags.push_back(require_string(tags_node[index], index_context(join_context(context, "tags"), index)));
            }

            if (const auto job_label = node["job_label"]; job_label)
            {
                entry.job_label = require_string(job_label, join_context(context, "job_label"));
            }

            const auto source = require_mapping(node["source"], join_context(context, "source"));
            entry.source_generator = require_string(source["generator"], join_context(context, "source.generator"));
            entry.source_mesh = require_string(source["mesh"], join_context(context, "source.mesh"));
            if (const auto value = source["mesh_sha256"]; value)
            {
                entry.source_mesh_sha256 = require_sha256(value, join_context(context, "source.mesh_sha256"));
            }
            if (const auto value = source["mesh_size_bytes"]; value)
            {
                entry.source_mesh_size_bytes = require_non_negative_uint64(value, join_context(context, "source.mesh_size_bytes"));
            }

            const auto outputs = require_mapping(node["outputs"], join_context(context, "outputs"));
            entry.output_mesh = require_string(outputs["mesh"], join_context(context, "outputs.mesh"));
            if (const auto value = outputs["mesh_sha256"]; value)
            {
                entry.output_mesh_sha256 = require_sha256(value, join_context(context, "outputs.mesh_sha256"));
            }
            if (const auto value = outputs["mesh_size_bytes"]; value)
            {
                entry.output_mesh_size_bytes =
                    require_non_negative_uint64(value, join_context(context, "outputs.mesh_size_bytes"));
            }

            const auto remeshing = require_mapping(node["remeshing"], join_context(context, "remeshing"));
            entry.remeshing_mode = require_string(remeshing["mode"], join_context(context, "remeshing.mode"));
            if (const auto value = remeshing["targets"]; value)
            {
                entry.remeshing_targets =
                    parse_remeshing_targets(require_mapping(value, join_context(context, "remeshing.targets")),
                                            join_context(context, "remeshing.targets"));
            }

            entry.feature_preservation = parse_feature_preservation(
                require_mapping(node["feature_preservation"], join_context(context, "feature_preservation")),
                join_context(context, "feature_preservation"));

            const auto metrics = require_mapping(node["metrics"], join_context(context, "metrics"));
            entry.input_metrics =
                parse_mesh_metrics(require_mapping(metrics["input"], join_context(context, "metrics.input")),
                                   join_context(context, "metrics.input"));
            entry.output_metrics =
                parse_mesh_metrics(require_mapping(metrics["output"], join_context(context, "metrics.output")),
                                   join_context(context, "metrics.output"));

            if (const auto parameterization = node["parameterization"]; parameterization)
            {
                entry.parameterization = parse_parameterization(
                    require_mapping(parameterization, join_context(context, "parameterization")),
                    join_context(context, "parameterization"));
            }

            entry.statistics = parse_dataset_statistics(
                require_mapping(node["statistics"], join_context(context, "statistics")),
                join_context(context, "statistics"));

            if (entry.schema_version >= 2)
            {
                if (!entry.source_mesh_sha256.has_value())
                {
                    throw_validation(join_context(context, "source.mesh_sha256"),
                                     "is required for dataset schema version >= 2");
                }
                if (!entry.source_mesh_size_bytes.has_value())
                {
                    throw_validation(join_context(context, "source.mesh_size_bytes"),
                                     "is required for dataset schema version >= 2");
                }
                if (!entry.output_mesh_sha256.has_value())
                {
                    throw_validation(join_context(context, "outputs.mesh_sha256"),
                                     "is required for dataset schema version >= 2");
                }
                if (!entry.output_mesh_size_bytes.has_value())
                {
                    throw_validation(join_context(context, "outputs.mesh_size_bytes"),
                                     "is required for dataset schema version >= 2");
                }
            }

            return entry;
        }

        [[nodiscard]] DatasetManifest parse_dataset_manifest_root(const YAML::Node& root, bool enforce_schema)
        {
            const auto manifest_mapping = require_mapping(root, "manifest");
            const auto datasets_node = require_sequence(manifest_mapping["datasets"], "datasets");
            DatasetManifest manifest;
            manifest.datasets.reserve(datasets_node.size());
            std::unordered_set<std::string> seen;
            for (std::size_t index = 0; index < datasets_node.size(); ++index)
            {
                const auto context = index_context("datasets", index);
                const auto dataset_node = require_mapping(datasets_node[index], context);
                auto entry = parse_dataset_entry(dataset_node, context, enforce_schema);
                if (!seen.insert(entry.identifier).second)
                {
                    std::ostringstream builder;
                    builder << "duplicates dataset identifier '" << entry.identifier << "'";
                    throw_validation(join_context(context, "id"), builder.str());
                }
                manifest.datasets.push_back(std::move(entry));
            }
            return manifest;
        }

        [[nodiscard]] RenderingConfig parse_rendering_config(const YAML::Node& node,
                                                              std::string_view context,
                                                              bool enforce_schema)
        {
            const auto header = parse_schema_header(node, context, "ai-004.rendering", enforce_schema);
            RenderingConfig config;
            config.schema_version = header.version;
            config.preset = require_string(node["preset"], join_context(context, "preset"));
            config.shading_mode = "deferred";
            config.width = 1920;
            config.height = 1080;
            config.overlay_normals = false;
            config.overlay_uv = false;
            config.overlay_material = false;
            config.overlay_light_volume = false;

            if (const auto options_node = node["options"]; options_node)
            {
                const auto options = require_mapping(options_node, join_context(context, "options"));
                if (const auto shading = options["shading_mode"]; shading)
                {
                    std::string shading_mode = require_string(shading, join_context(context, "options.shading_mode"));
                    std::string lowered;
                    lowered.reserve(shading_mode.size());
                    std::transform(shading_mode.begin(), shading_mode.end(), std::back_inserter(lowered),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (lowered != "forward" && lowered != "deferred")
                    {
                        std::ostringstream builder;
                        builder << "must be 'forward' or 'deferred'; received '" << shading_mode << "'";
                        throw_validation(join_context(context, "options.shading_mode"), builder.str());
                    }
                    config.shading_mode = lowered;
                }
                if (const auto resolution_node = options["resolution"]; resolution_node)
                {
                    const auto resolution = require_mapping(resolution_node, join_context(context, "options.resolution"));
                    config.width = require_positive_int(resolution["width"],
                                                        join_context(context, "options.resolution.width"));
                    config.height = require_positive_int(resolution["height"],
                                                         join_context(context, "options.resolution.height"));
                }
                if (const auto overlays_node = options["overlays"]; overlays_node)
                {
                    const auto overlays = require_mapping(overlays_node, join_context(context, "options.overlays"));
                    if (const auto value = overlays["normals"]; value)
                    {
                        config.overlay_normals = require_bool(value, join_context(context, "options.overlays.normals"));
                    }
                    if (const auto value = overlays["uv"]; value)
                    {
                        config.overlay_uv = require_bool(value, join_context(context, "options.overlays.uv"));
                    }
                    if (const auto value = overlays["material"]; value)
                    {
                        config.overlay_material = require_bool(value, join_context(context, "options.overlays.material"));
                    }
                    if (const auto value = overlays["light_volume"]; value)
                    {
                        config.overlay_light_volume =
                            require_bool(value, join_context(context, "options.overlays.light_volume"));
                    }
                }
            }

            return config;
        }

        [[nodiscard]] RuntimeCameraConfig parse_camera_config(const YAML::Node& node, std::string_view context)
        {
            RuntimeCameraConfig config;
            config.mode = require_string(node["mode"], join_context(context, "mode"));
            std::string lowered;
            lowered.reserve(config.mode.size());
            std::transform(config.mode.begin(), config.mode.end(), std::back_inserter(lowered),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            static const std::unordered_set<std::string, std::hash<std::string>, std::equal_to<>> kSupportedModes{
                "orbit", "fly", "fixed"};
            if (kSupportedModes.count(lowered) == 0U)
            {
                std::ostringstream builder;
                builder << "must be one of 'orbit', 'fly', or 'fixed'; received '" << config.mode << "'";
                throw_validation(join_context(context, "mode"), builder.str());
            }
            config.mode = lowered;
            if (const auto value = node["position"]; value)
            {
                config.position = require_vec3(value, join_context(context, "position"));
            }
            if (const auto value = node["target"]; value)
            {
                config.target = require_vec3(value, join_context(context, "target"));
            }
            return config;
        }

        [[nodiscard]] RuntimeSimulationConfig parse_simulation_config(const YAML::Node& node, std::string_view context)
        {
            RuntimeSimulationConfig config;
            config.timestep_seconds = require_positive_float(node["timestep_seconds"],
                                                             join_context(context, "timestep_seconds"));
            config.max_substeps = require_positive_int(node["max_substeps"], join_context(context, "max_substeps"));
            return config;
        }

        [[nodiscard]] RuntimeHotReloadConfig parse_hot_reload_config(const YAML::Node& node, std::string_view context)
        {
            RuntimeHotReloadConfig config;
            config.enabled = require_bool(node["enabled"], join_context(context, "enabled"));
            if (const auto value = node["watch_interval_seconds"]; value)
            {
                config.watch_interval_seconds =
                    require_positive_float(value, join_context(context, "watch_interval_seconds"));
            }
            return config;
        }

        [[nodiscard]] RuntimeConfig parse_runtime_config(const YAML::Node& node,
                                                         std::string_view context,
                                                         bool enforce_schema)
        {
            const auto header = parse_schema_header(node, context, "ai-004.runtime", enforce_schema);
            RuntimeConfig config;
            config.schema_version = header.version;
            if (const auto dataset_node = node["dataset"]; dataset_node)
            {
                config.dataset = require_slug(dataset_node, join_context(context, "dataset"));
            }
            if (const auto scene_node = node["scene"]; scene_node)
            {
                const auto scene = require_mapping(scene_node, join_context(context, "scene"));
                config.scene_manifest = require_string(scene["manifest"], join_context(context, "scene.manifest"));
                if (const auto value = scene["entry_point"]; value)
                {
                    config.scene_entry_point = require_string(value, join_context(context, "scene.entry_point"));
                }
            }
            if (const auto camera_node = node["camera"]; camera_node)
            {
                config.camera =
                    parse_camera_config(require_mapping(camera_node, join_context(context, "camera")),
                                        join_context(context, "camera"));
            }
            if (const auto sim_node = node["simulation"]; sim_node)
            {
                config.simulation =
                    parse_simulation_config(require_mapping(sim_node, join_context(context, "simulation")),
                                            join_context(context, "simulation"));
            }
            config.hot_reload = RuntimeHotReloadConfig{false, std::nullopt};
            if (const auto hot_reload_node = node["hot_reload"]; hot_reload_node)
            {
                config.hot_reload =
                    parse_hot_reload_config(require_mapping(hot_reload_node, join_context(context, "hot_reload")),
                                             join_context(context, "hot_reload"));
            }
            return config;
        }

        [[nodiscard]] BenchmarkThreshold parse_benchmark_threshold(const YAML::Node& node, std::string_view context)
        {
            BenchmarkThreshold threshold;
            threshold.mode = require_string(node["type"], join_context(context, "type"));
            std::string lowered;
            lowered.reserve(threshold.mode.size());
            std::transform(threshold.mode.begin(), threshold.mode.end(), std::back_inserter(lowered),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (lowered == "relative")
            {
                threshold.limit = require_non_negative_float(node["max_regression"],
                                                             join_context(context, "max_regression"));
            }
            else if (lowered == "absolute")
            {
                threshold.limit = require_non_negative_float(node["max_delta"], join_context(context, "max_delta"));
            }
            else
            {
                std::ostringstream builder;
                builder << "must be 'relative' or 'absolute'; received '" << threshold.mode << "'";
                throw_validation(join_context(context, "type"), builder.str());
            }
            threshold.mode = lowered;
            return threshold;
        }

        [[nodiscard]] BenchmarkMetricConfig parse_benchmark_metric(const YAML::Node& node, std::string_view context)
        {
            BenchmarkMetricConfig metric;
            metric.name = require_string(node["name"], join_context(context, "name"));
            metric.higher_is_better = require_bool(node["higher_is_better"], join_context(context, "higher_is_better"));
            metric.threshold = parse_benchmark_threshold(require_mapping(node["threshold"],
                                                                        join_context(context, "threshold")),
                                                         join_context(context, "threshold"));
            return metric;
        }

        [[nodiscard]] BenchmarkCommandConfig parse_benchmark_command(const YAML::Node& node, std::string_view context)
        {
            BenchmarkCommandConfig command;
            command.output = require_string(node["output"], join_context(context, "output"));
            if (const auto command_node = node["command"]; command_node)
            {
                const auto sequence = require_sequence(command_node, join_context(context, "command"));
                if (sequence.size() == 0U)
                {
                    throw_validation(join_context(context, "command"), "must not be empty");
                }
                command.command = std::vector<std::string>{};
                command.command->reserve(sequence.size());
                for (std::size_t index = 0; index < sequence.size(); ++index)
                {
                    command.command->push_back(
                        require_string(sequence[index],
                                       index_context(join_context(context, "command"), index)));
                }
            }
            return command;
        }

        [[nodiscard]] BenchmarkScenarioConfig parse_benchmark_scenario(const YAML::Node& node,
                                                                       std::string_view context)
        {
            BenchmarkScenarioConfig scenario;
            scenario.name = require_string(node["name"], join_context(context, "name"));
            if (const auto id_node = node["id"]; id_node)
            {
                scenario.identifier = require_slug(id_node, join_context(context, "id"));
            }
            else
            {
                scenario.identifier = require_slug(node["name"], join_context(context, "name"));
            }
            if (const auto dataset_node = node["dataset"]; dataset_node)
            {
                scenario.dataset = require_slug(dataset_node, join_context(context, "dataset"));
            }
            if (const auto preset_node = node["rendering_preset"]; preset_node)
            {
                scenario.rendering_preset = require_string(preset_node, join_context(context, "rendering_preset"));
            }
            if (const auto runtime_profile_node = node["runtime_profile"]; runtime_profile_node)
            {
                scenario.runtime_profile = require_string(runtime_profile_node, join_context(context, "runtime_profile"));
            }
            scenario.engine =
                parse_benchmark_command(require_mapping(node["engine"], join_context(context, "engine")),
                                        join_context(context, "engine"));
            scenario.reference =
                parse_benchmark_command(require_mapping(node["reference"], join_context(context, "reference")),
                                        join_context(context, "reference"));

            const auto metrics_node = require_sequence(node["metrics"], join_context(context, "metrics"));
            if (metrics_node.size() == 0U)
            {
                throw_validation(join_context(context, "metrics"), "must contain at least one metric");
            }
            scenario.metrics.reserve(metrics_node.size());
            for (std::size_t index = 0; index < metrics_node.size(); ++index)
            {
                const auto metric_context = index_context(join_context(context, "metrics"), index);
                scenario.metrics.push_back(
                    parse_benchmark_metric(require_mapping(metrics_node[index], metric_context), metric_context));
            }
            return scenario;
        }

        [[nodiscard]] BenchmarkConfig parse_benchmark_config(const YAML::Node& node,
                                                              std::string_view context,
                                                              bool enforce_schema)
        {
            const auto header = parse_schema_header(node, context, "ai-004.benchmarks", enforce_schema);
            BenchmarkConfig config;
            config.schema_version = header.version;
            const auto scenarios_node = require_sequence(node["scenarios"], join_context(context, "scenarios"));
            if (scenarios_node.size() == 0U)
            {
                throw_validation(join_context(context, "scenarios"), "must contain at least one scenario");
            }
            config.scenarios.reserve(scenarios_node.size());
            for (std::size_t index = 0; index < scenarios_node.size(); ++index)
            {
                const auto scenario_context = index_context(join_context(context, "scenarios"), index);
                config.scenarios.push_back(
                    parse_benchmark_scenario(require_mapping(scenarios_node[index], scenario_context), scenario_context));
            }
            return config;
        }

        [[nodiscard]] TelemetryOutputConfig parse_telemetry_output(const YAML::Node& node, std::string_view context)
        {
            TelemetryOutputConfig output;
            output.kind = require_string(node["type"], join_context(context, "type"));
            std::string lowered;
            lowered.reserve(output.kind.size());
            std::transform(output.kind.begin(), output.kind.end(), std::back_inserter(lowered),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (lowered == "file")
            {
                output.path = require_string(node["path"], join_context(context, "path"));
            }
            else if (lowered == "stdout")
            {
                if (const auto value = node["path"]; value)
                {
                    throw_validation(join_context(context, "path"), "is not valid for stdout outputs");
                }
            }
            else
            {
                std::ostringstream builder;
                builder << "must be 'file' or 'stdout'; received '" << output.kind << "'";
                throw_validation(join_context(context, "type"), builder.str());
            }
            output.kind = lowered;
            return output;
        }

        [[nodiscard]] TelemetryMetricConfig parse_telemetry_metric(const YAML::Node& node, std::string_view context)
        {
            TelemetryMetricConfig metric;
            metric.name = require_string(node["name"], join_context(context, "name"));
            metric.statistic = require_string(node["statistic"], join_context(context, "statistic"));
            std::string lowered;
            lowered.reserve(metric.statistic.size());
            std::transform(metric.statistic.begin(), metric.statistic.end(), std::back_inserter(lowered),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            static const std::unordered_set<std::string, std::hash<std::string>, std::equal_to<>> kStatistics{
                "mean", "median", "min", "max", "p95", "p99"};
            if (kStatistics.count(lowered) == 0U)
            {
                std::ostringstream builder;
                builder << "must be one of mean, median, min, max, p95, or p99; received '" << metric.statistic << "'";
                throw_validation(join_context(context, "statistic"), builder.str());
            }
            metric.statistic = lowered;
            return metric;
        }

        [[nodiscard]] TelemetrySamplingConfig parse_telemetry_sampling(const YAML::Node& node, std::string_view context)
        {
            TelemetrySamplingConfig sampling;
            sampling.frame_interval = require_positive_int(node["frame_interval"], join_context(context, "frame_interval"));
            if (const auto value = node["include_debug_overlays"]; value)
            {
                sampling.include_debug_overlays = require_bool(value, join_context(context, "include_debug_overlays"));
            }
            else
            {
                sampling.include_debug_overlays = false;
            }
            return sampling;
        }

        [[nodiscard]] TelemetryConfig parse_telemetry_config(const YAML::Node& node,
                                                              std::string_view context,
                                                              bool enforce_schema)
        {
            const auto header = parse_schema_header(node, context, "ai-004.telemetry", enforce_schema);
            TelemetryConfig config;
            config.schema_version = header.version;

            if (const auto outputs_node = node["outputs"]; outputs_node)
            {
                const auto outputs = require_sequence(outputs_node, join_context(context, "outputs"));
                config.outputs.reserve(outputs.size());
                for (std::size_t index = 0; index < outputs.size(); ++index)
                {
                    const auto output_context = index_context(join_context(context, "outputs"), index);
                    config.outputs.push_back(
                        parse_telemetry_output(require_mapping(outputs[index], output_context), output_context));
                }
            }

            if (const auto metrics_node = node["metrics"]; metrics_node)
            {
                const auto metrics = require_sequence(metrics_node, join_context(context, "metrics"));
                config.metrics.reserve(metrics.size());
                for (std::size_t index = 0; index < metrics.size(); ++index)
                {
                    const auto metric_context = index_context(join_context(context, "metrics"), index);
                    config.metrics.push_back(
                        parse_telemetry_metric(require_mapping(metrics[index], metric_context), metric_context));
                }
            }

            if (const auto sampling_node = node["sampling"]; sampling_node)
            {
                config.sampling = parse_telemetry_sampling(
                    require_mapping(sampling_node, join_context(context, "sampling")),
                    join_context(context, "sampling"));
            }

            return config;
        }

        [[nodiscard]] Ai004Configuration parse_configuration_root(const YAML::Node& root, bool enforce_schema)
        {
            const auto mapping = require_mapping(root, "configuration");
            Ai004Configuration configuration;
            if (const auto datasets_node = mapping["datasets"]; datasets_node)
            {
                configuration.datasets = parse_dataset_manifest_root(mapping, enforce_schema);
            }
            else
            {
                configuration.datasets = DatasetManifest{};
            }

            std::unordered_set<std::string> dataset_slugs;
            for (const auto& dataset : configuration.datasets.datasets)
            {
                dataset_slugs.insert(dataset.identifier);
            }

            const auto validate_dataset_reference = [&](const std::string& slug, std::string_view context) {
                if (dataset_slugs.count(slug) > 0U)
                {
                    return;
                }
                if (dataset_slugs.empty())
                {
                    std::ostringstream builder;
                    builder << "references dataset '" << slug << "' but no datasets are declared";
                    throw_validation(context, builder.str());
                }
                std::ostringstream builder;
                builder << "references unknown dataset '" << slug << "'. Available datasets: ";
                bool first = true;
                for (const auto& known : dataset_slugs)
                {
                    if (!first)
                    {
                        builder << ", ";
                    }
                    builder << known;
                    first = false;
                }
                throw_validation(context, builder.str());
            };

            if (const auto rendering_node = mapping["rendering"]; rendering_node)
            {
                configuration.rendering = parse_rendering_config(
                    require_mapping(rendering_node, "rendering"),
                    "rendering",
                    enforce_schema);
            }

            if (const auto runtime_node = mapping["runtime"]; runtime_node)
            {
                configuration.runtime = parse_runtime_config(require_mapping(runtime_node, "runtime"), "runtime",
                                                              enforce_schema);
                if (configuration.runtime->dataset.has_value())
                {
                    validate_dataset_reference(*configuration.runtime->dataset, "runtime.dataset");
                }
            }

            if (const auto benchmarks_node = mapping["benchmarks"]; benchmarks_node)
            {
                configuration.benchmarks = parse_benchmark_config(
                    require_mapping(benchmarks_node, "benchmarks"),
                    "benchmarks",
                    enforce_schema);
                for (std::size_t index = 0; index < configuration.benchmarks->scenarios.size(); ++index)
                {
                    const auto& scenario = configuration.benchmarks->scenarios[index];
                    if (scenario.dataset.has_value())
                    {
                        validate_dataset_reference(
                            *scenario.dataset,
                            index_context("benchmarks.scenarios", index) + ".dataset");
                    }
                }
            }

            if (const auto telemetry_node = mapping["telemetry"]; telemetry_node)
            {
                configuration.telemetry = parse_telemetry_config(
                    require_mapping(telemetry_node, "telemetry"),
                    "telemetry",
                    enforce_schema);
            }

            return configuration;
        }

        [[nodiscard]] bool is_supported_extension(const std::filesystem::path& path)
        {
            const auto extension = path.extension().string();
            if (extension.empty())
            {
                return true; // Treat as JSON/YAML
            }
            std::string lowered;
            lowered.reserve(extension.size());
            std::transform(extension.begin(), extension.end(), std::back_inserter(lowered),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return lowered == ".json" || lowered == ".yaml" || lowered == ".yml";
        }

        [[nodiscard]] RuntimeResult<std::string> read_text_file(const std::filesystem::path& path)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
            {
                return make_io_error(path, "file could not be opened");
            }
            std::ostringstream buffer;
            buffer << stream.rdbuf();
            if (stream.bad())
            {
                return make_io_error(path, "error while reading file");
            }
            return buffer.str();
        }
    } // namespace

    RuntimeResult<DatasetManifest> load_dataset_manifest(const std::filesystem::path& path,
                                                         std::optional<bool> require_schema) noexcept
    {
        if (!is_supported_extension(path))
        {
            return make_parse_error(path, "unsupported manifest file extension");
        }

        const bool enforce_schema = is_schema_enforced(require_schema);
        auto text_result = read_text_file(path);
        if (!text_result)
        {
            return text_result.error();
        }

        YAML::Node root;
        try
        {
            root = YAML::Load(text_result.value());
        }
        catch (const YAML::ParserException& exception)
        {
            return make_parse_error(path, exception.what());
        }
        catch (const YAML::BadFile& exception)
        {
            return make_io_error(path, exception.what());
        }

        try
        {
            return parse_dataset_manifest_root(root, enforce_schema);
        }
        catch (const SchemaValidationError& error)
        {
            return make_validation_error(error.what());
        }
    }

    RuntimeResult<Ai004Configuration> load_configuration(const std::filesystem::path& path,
                                                         std::optional<bool> require_schema) noexcept
    {
        if (!is_supported_extension(path))
        {
            return make_parse_error(path, "unsupported manifest file extension");
        }

        const bool enforce_schema = is_schema_enforced(require_schema);
        auto text_result = read_text_file(path);
        if (!text_result)
        {
            return text_result.error();
        }

        YAML::Node root;
        try
        {
            root = YAML::Load(text_result.value());
        }
        catch (const YAML::ParserException& exception)
        {
            return make_parse_error(path, exception.what());
        }
        catch (const YAML::BadFile& exception)
        {
            return make_io_error(path, exception.what());
        }

        try
        {
            return parse_configuration_root(root, enforce_schema);
        }
        catch (const SchemaValidationError& error)
        {
            return make_validation_error(error.what());
        }
    }
} // namespace engine::runtime::config

