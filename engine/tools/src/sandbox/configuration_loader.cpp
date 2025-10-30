#include "engine/tools/sandbox/configuration_loader.hpp"

#include <charconv>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace engine::tools::sandbox
{
    namespace
    {
        struct JsonValue
        {
            using object_type = std::map<std::string, JsonValue>;
            using array_type = std::vector<JsonValue>;
            using variant_type = std::variant<std::nullptr_t, bool, double, std::string, array_type, object_type>;

            JsonValue() : data(nullptr) {}
            explicit JsonValue(std::nullptr_t) : data(nullptr) {}
            explicit JsonValue(bool value) : data(value) {}
            explicit JsonValue(double value) : data(value) {}
            explicit JsonValue(std::string value) : data(std::move(value)) {}
            explicit JsonValue(array_type value) : data(std::move(value)) {}
            explicit JsonValue(object_type value) : data(std::move(value)) {}

            [[nodiscard]] bool is_null() const noexcept { return std::holds_alternative<std::nullptr_t>(data); }
            [[nodiscard]] bool is_bool() const noexcept { return std::holds_alternative<bool>(data); }
            [[nodiscard]] bool is_number() const noexcept { return std::holds_alternative<double>(data); }
            [[nodiscard]] bool is_string() const noexcept { return std::holds_alternative<std::string>(data); }
            [[nodiscard]] bool is_array() const noexcept { return std::holds_alternative<array_type>(data); }
            [[nodiscard]] bool is_object() const noexcept { return std::holds_alternative<object_type>(data); }

            [[nodiscard]] bool as_bool(std::string_view context) const
            {
                if (!is_bool())
                {
                    throw std::runtime_error(std::string(context) + " must be a boolean");
                }
                return std::get<bool>(data);
            }

            [[nodiscard]] double as_number(std::string_view context) const
            {
                if (!is_number())
                {
                    throw std::runtime_error(std::string(context) + " must be a number");
                }
                return std::get<double>(data);
            }

            [[nodiscard]] const std::string& as_string(std::string_view context) const
            {
                if (!is_string())
                {
                    throw std::runtime_error(std::string(context) + " must be a string");
                }
                return std::get<std::string>(data);
            }

            [[nodiscard]] const array_type& as_array(std::string_view context) const
            {
                if (!is_array())
                {
                    throw std::runtime_error(std::string(context) + " must be an array");
                }
                return std::get<array_type>(data);
            }

            [[nodiscard]] const object_type& as_object(std::string_view context) const
            {
                if (!is_object())
                {
                    throw std::runtime_error(std::string(context) + " must be an object");
                }
                return std::get<object_type>(data);
            }

            variant_type data;
        };

        class JsonParser
        {
        public:
            explicit JsonParser(std::string_view data) : data_(data) {}

            JsonValue parse()
            {
                auto value = parse_value();
                ensure_end();
                return value;
            }

        private:
            [[nodiscard]] bool eof() const noexcept
            {
                return index_ >= data_.size();
            }

            [[nodiscard]] char peek() const
            {
                if (eof())
                {
                    throw std::runtime_error("Unexpected end of JSON input");
                }
                return data_[index_];
            }

            [[nodiscard]] char get()
            {
                if (eof())
                {
                    throw std::runtime_error("Unexpected end of JSON input");
                }
                return data_[index_++];
            }

            void skip_whitespace()
            {
                while (!eof() && std::isspace(static_cast<unsigned char>(data_[index_])) != 0)
                {
                    ++index_;
                }
            }

            void ensure_end()
            {
                skip_whitespace();
                if (!eof())
                {
                    throw std::runtime_error("Unexpected trailing data in JSON input");
                }
            }

            [[nodiscard]] bool starts_with(std::string_view literal) const noexcept
            {
                if (literal.size() > data_.size() - index_)
                {
                    return false;
                }
                return data_.substr(index_, literal.size()) == literal;
            }

            [[nodiscard]] bool match_literal(std::string_view literal)
            {
                if (!starts_with(literal))
                {
                    return false;
                }
                index_ += literal.size();
                return true;
            }

            JsonValue parse_value()
            {
                skip_whitespace();
                if (eof())
                {
                    throw std::runtime_error("Unexpected end of JSON input");
                }
                const char ch = peek();
                if (ch == '{')
                {
                    return JsonValue(parse_object());
                }
                if (ch == '[')
                {
                    return JsonValue(parse_array());
                }
                if (ch == '"')
                {
                    return JsonValue(parse_string());
                }
                if (match_literal("true"))
                {
                    return JsonValue(true);
                }
                if (match_literal("false"))
                {
                    return JsonValue(false);
                }
                if (match_literal("null"))
                {
                    return JsonValue(nullptr);
                }
                if (ch == '-' || ch == '+' || ch == '.' || std::isdigit(static_cast<unsigned char>(ch)) != 0)
                {
                    return JsonValue(parse_number());
                }
                throw std::runtime_error("Unexpected token in JSON input");
            }

            JsonValue::object_type parse_object()
            {
                expect_character('{');
                skip_whitespace();
                JsonValue::object_type result;
                if (consume_character('}'))
                {
                    return result;
                }
                while (true)
                {
                    skip_whitespace();
                    const std::string key = parse_string();
                    expect_character(':');
                    JsonValue value = parse_value();
                    result.insert_or_assign(key, std::move(value));
                    skip_whitespace();
                    if (consume_character('}'))
                    {
                        break;
                    }
                    expect_character(',');
                }
                return result;
            }

            JsonValue::array_type parse_array()
            {
                expect_character('[');
                skip_whitespace();
                JsonValue::array_type result;
                if (consume_character(']'))
                {
                    return result;
                }
                while (true)
                {
                    result.push_back(parse_value());
                    skip_whitespace();
                    if (consume_character(']'))
                    {
                        break;
                    }
                    expect_character(',');
                }
                return result;
            }

            std::string parse_string()
            {
                expect_character('"');
                std::string result;
                bool terminated = false;
                while (!eof())
                {
                    const char ch = get();
                    if (ch == '"')
                    {
                        terminated = true;
                        break;
                    }
                    if (ch == '\\')
                    {
                        if (eof())
                        {
                            throw std::runtime_error("Invalid escape sequence in JSON string");
                        }
                        const char escape = get();
                        switch (escape)
                        {
                        case '"': result.push_back('"'); break;
                        case '\\': result.push_back('\\'); break;
                        case '/': result.push_back('/'); break;
                        case 'b': result.push_back('\b'); break;
                        case 'f': result.push_back('\f'); break;
                        case 'n': result.push_back('\n'); break;
                        case 'r': result.push_back('\r'); break;
                        case 't': result.push_back('\t'); break;
                        case 'u':
                            {
                                unsigned int codepoint = 0;
                                for (int i = 0; i < 4; ++i)
                                {
                                    if (eof())
                                    {
                                        throw std::runtime_error("Invalid Unicode escape in JSON string");
                                    }
                                    const char digit = get();
                                    codepoint <<= 4;
                                    if (digit >= '0' && digit <= '9')
                                    {
                                        codepoint |= static_cast<unsigned int>(digit - '0');
                                    }
                                    else if (digit >= 'a' && digit <= 'f')
                                    {
                                        codepoint |= static_cast<unsigned int>(digit - 'a' + 10);
                                    }
                                    else if (digit >= 'A' && digit <= 'F')
                                    {
                                        codepoint |= static_cast<unsigned int>(digit - 'A' + 10);
                                    }
                                    else
                                    {
                                        throw std::runtime_error("Invalid Unicode escape in JSON string");
                                    }
                                }
                                if (codepoint <= 0x7F)
                                {
                                    result.push_back(static_cast<char>(codepoint));
                                }
                                else
                                {
                                    throw std::runtime_error("Only ASCII Unicode escapes are supported in JSON string");
                                }
                                break;
                            }
                        default:
                            throw std::runtime_error("Unsupported escape sequence in JSON string");
                        }
                        continue;
                    }
                    result.push_back(ch);
                }
                if (!terminated)
                {
                    throw std::runtime_error("Unterminated string literal in JSON input");
                }
                return result;
            }

            double parse_number()
            {
                const std::size_t start = index_;
                if (!eof() && (data_[index_] == '-' || data_[index_] == '+'))
                {
                    ++index_;
                }
                while (!eof() && std::isdigit(static_cast<unsigned char>(data_[index_])) != 0)
                {
                    ++index_;
                }
                if (!eof() && data_[index_] == '.')
                {
                    ++index_;
                    while (!eof() && std::isdigit(static_cast<unsigned char>(data_[index_])) != 0)
                    {
                        ++index_;
                    }
                }
                if (!eof() && (data_[index_] == 'e' || data_[index_] == 'E'))
                {
                    ++index_;
                    if (!eof() && (data_[index_] == '+' || data_[index_] == '-'))
                    {
                        ++index_;
                    }
                    while (!eof() && std::isdigit(static_cast<unsigned char>(data_[index_])) != 0)
                    {
                        ++index_;
                    }
                }
                const auto token = data_.substr(start, index_ - start);
                double value = 0.0;
                const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
                if (result.ec != std::errc{})
                {
                    throw std::runtime_error("Invalid numeric literal in JSON input");
                }
                return value;
            }

            void expect_character(char expected)
            {
                skip_whitespace();
                if (eof() || data_[index_] != expected)
                {
                    throw std::runtime_error("Unexpected character in JSON input");
                }
                ++index_;
            }

            bool consume_character(char ch)
            {
                skip_whitespace();
                if (eof() || data_[index_] != ch)
                {
                    return false;
                }
                ++index_;
                return true;
            }

            std::string_view data_{};
            std::size_t index_{0};
        };

        [[nodiscard]] const JsonValue* find_member(const JsonValue::object_type& object, std::string_view key)
        {
            if (auto it = object.find(std::string(key)); it != object.end())
            {
                return &it->second;
            }
            return nullptr;
        }

        [[nodiscard]] std::string get_string(const JsonValue::object_type& object, std::string_view key,
                                             std::string_view context, std::string default_value = {})
        {
            if (const auto* value = find_member(object, key))
            {
                if (value->is_null())
                {
                    return std::string{};
                }
                return value->as_string(context);
            }
            return std::string{default_value};
        }

        [[nodiscard]] bool get_bool(const JsonValue::object_type& object, std::string_view key,
                                    std::string_view context, bool default_value = false)
        {
            if (const auto* value = find_member(object, key))
            {
                if (value->is_null())
                {
                    return default_value;
                }
                return value->as_bool(context);
            }
            return default_value;
        }

        [[nodiscard]] double get_number(const JsonValue::object_type& object, std::string_view key,
                                        std::string_view context, double default_value = 0.0)
        {
            if (const auto* value = find_member(object, key))
            {
                if (value->is_null())
                {
                    return default_value;
                }
                return value->as_number(context);
            }
            return default_value;
        }

        [[nodiscard]] std::optional<std::string> get_optional_string(const JsonValue::object_type& object,
                                                                     std::string_view key,
                                                                     std::string_view context)
        {
            if (const auto* value = find_member(object, key))
            {
                if (value->is_null())
                {
                    return std::nullopt;
                }
                return value->as_string(context);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<double> get_optional_number(const JsonValue::object_type& object,
                                                                 std::string_view key,
                                                                 std::string_view context)
        {
            if (const auto* value = find_member(object, key))
            {
                if (value->is_null())
                {
                    return std::nullopt;
                }
                return value->as_number(context);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::uintmax_t> get_optional_size(const JsonValue::object_type& object,
                                                                      std::string_view key,
                                                                      std::string_view context)
        {
            if (const auto* value = find_member(object, key))
            {
                if (value->is_null())
                {
                    return std::nullopt;
                }
                const double numeric = value->as_number(context);
                if (numeric < 0.0)
                {
                    return std::nullopt;
                }
                return static_cast<std::uintmax_t>(numeric);
            }
            return std::nullopt;
        }

        void flatten_numeric_entries(const JsonValue& value, std::string prefix,
                                     std::map<std::string, double>& output)
        {
            if (value.is_number())
            {
                if (!prefix.empty())
                {
                    output.insert_or_assign(prefix, value.as_number(prefix));
                }
                return;
            }
            if (value.is_object())
            {
                for (const auto& [child_key, child_value] : value.as_object(prefix))
                {
                    std::string next_prefix = prefix.empty() ? child_key : prefix + '.' + child_key;
                    flatten_numeric_entries(child_value, std::move(next_prefix), output);
                }
                return;
            }
            if (value.is_array())
            {
                const auto& array = value.as_array(prefix);
                for (std::size_t index = 0; index < array.size(); ++index)
                {
                    const auto& element = array[index];
                    std::string next_prefix = prefix + '[' + std::to_string(index) + ']';
                    flatten_numeric_entries(element, std::move(next_prefix), output);
                }
            }
        }

        void populate_dataset(const JsonValue& dataset_value, DatasetDescriptor& descriptor)
        {
            const auto& dataset = dataset_value.as_object("dataset entry");
            descriptor.identifier = get_string(dataset, "id", "dataset.id");
            descriptor.label = get_string(dataset, "label", "dataset.label", descriptor.identifier);
            descriptor.kind = get_string(dataset, "kind", "dataset.kind");
            descriptor.schema_id = get_string(dataset, "schema_id", "dataset.schema_id");
            descriptor.schema_version = static_cast<int>(std::lround(
                get_number(dataset, "schema_version", "dataset.schema_version", 0.0)));
            
            if (const auto* tags_value = find_member(dataset, "tags"))
            {
                if (!tags_value->is_array())
                {
                    throw std::runtime_error("dataset.tags must be an array");
                }
                const auto& tags = tags_value->as_array("dataset.tags");
                descriptor.tags.clear();
                descriptor.tags.reserve(tags.size());
                for (const auto& tag : tags)
                {
                    descriptor.tags.push_back(tag.as_string("dataset.tags[]"));
                }
            }

            descriptor.source_generator = get_string(dataset, "source_generator", "dataset.source_generator");
            descriptor.source_asset = get_string(dataset, "source_mesh", "dataset.source_mesh");
            descriptor.source_asset_sha256 =
                get_optional_string(dataset, "source_mesh_sha256", "dataset.source_mesh_sha256");
            descriptor.source_asset_size_bytes =
                get_optional_size(dataset, "source_mesh_size_bytes", "dataset.source_mesh_size_bytes");
            descriptor.processed_asset = get_string(dataset, "output_mesh", "dataset.output_mesh");
            descriptor.processed_asset_sha256 =
                get_optional_string(dataset, "output_mesh_sha256", "dataset.output_mesh_sha256");
            descriptor.processed_asset_size_bytes =
                get_optional_size(dataset, "output_mesh_size_bytes", "dataset.output_mesh_size_bytes");
            descriptor.remeshing_mode = get_string(dataset, "remeshing_mode", "dataset.remeshing_mode");

            descriptor.statistics.clear();
            if (const auto* statistics = find_member(dataset, "statistics"))
            {
                if (!statistics->is_object())
                {
                    throw std::runtime_error("dataset.statistics must be an object");
                }
                for (const auto& [key, value] : statistics->as_object("dataset.statistics"))
                {
                    if (value.is_number())
                    {
                        descriptor.statistics.insert_or_assign(key, value.as_number("dataset.statistics value"));
                    }
                }
            }

            descriptor.metrics.clear();
            if (const auto* metrics = find_member(dataset, "metrics"))
            {
                flatten_numeric_entries(*metrics, std::string{}, descriptor.metrics);
            }
            descriptor.remeshing_targets.clear();
            if (const auto* targets = find_member(dataset, "remeshing_targets"))
            {
                if (targets->is_object())
                {
                    const auto& target_object = targets->as_object("dataset.remeshing_targets");
                    for (const auto& [key, value] : target_object)
                    {
                        if (value.is_number())
                        {
                            descriptor.remeshing_targets.emplace_back(
                                key, value.as_number("dataset.remeshing_targets value"));
                        }
                    }
                }
                flatten_numeric_entries(*targets, std::string{"remeshing"}, descriptor.metrics);
            }
            descriptor.parameterization_properties.clear();
            if (const auto* parameterization = find_member(dataset, "parameterization"))
            {
                if (parameterization->is_object())
                {
                    const auto& parameterization_object = parameterization->as_object("dataset.parameterization");
                    if (const auto* mode_value = find_member(parameterization_object, "mode"))
                    {
                        if (mode_value->is_string())
                        {
                            descriptor.parameterization_properties.emplace_back(
                                "mode", mode_value->as_string("dataset.parameterization.mode"));
                        }
                    }
                    if (const auto* charts_value = find_member(parameterization_object, "charts"))
                    {
                        if (charts_value->is_array())
                        {
                            const auto& charts = charts_value->as_array("dataset.parameterization.charts");
                            descriptor.parameterization_properties.emplace_back(
                                "charts", std::to_string(charts.size()));
                        }
                    }
                }
                flatten_numeric_entries(*parameterization, std::string{"parameterization"}, descriptor.metrics);
            }

            descriptor.feature_preservation.clear();
            if (const auto* preservation = find_member(dataset, "feature_preservation"))
            {
                if (preservation->is_object())
                {
                    const auto& preservation_object =
                        preservation->as_object("dataset.feature_preservation");
                    for (const auto& [key, value] : preservation_object)
                    {
                        if (value.is_bool())
                        {
                            descriptor.feature_preservation.emplace_back(
                                key, value.as_bool("dataset.feature_preservation") ? "true" : "false");
                        }
                        else if (value.is_number())
                        {
                            std::ostringstream stream;
                            stream.setf(std::ios::fixed, std::ios::floatfield);
                            stream << std::setprecision(3)
                                   << value.as_number("dataset.feature_preservation value");
                            descriptor.feature_preservation.emplace_back(key, stream.str());
                        }
                        else if (value.is_string())
                        {
                            descriptor.feature_preservation.emplace_back(
                                key, value.as_string("dataset.feature_preservation value"));
                        }
                    }
                }
            }

            descriptor.assets.clear();
            if (const auto* assets_value = find_member(dataset, "assets"))
            {
                const auto& assets = assets_value->as_array("dataset.assets");
                descriptor.assets.reserve(assets.size());
                for (const auto& asset_value : assets)
                {
                    const auto& asset = asset_value.as_object("dataset.assets[]");
                    DatasetAssetDescriptor asset_descriptor{};
                    asset_descriptor.role = get_string(asset, "role", "dataset.assets[].role");
                    asset_descriptor.path = get_string(asset, "path", "dataset.assets[].path");
                    asset_descriptor.resolved_path = get_string(asset, "resolved_path", "dataset.assets[].resolved_path");
                    asset_descriptor.exists = get_bool(asset, "exists", "dataset.assets[].exists", false);
                    asset_descriptor.verified = get_bool(asset, "verified", "dataset.assets[].verified", false);
                    asset_descriptor.expected_size_bytes =
                        get_optional_size(asset, "expected_size_bytes", "dataset.assets[].expected_size_bytes");
                    asset_descriptor.actual_size_bytes =
                        get_optional_size(asset, "actual_size_bytes", "dataset.assets[].actual_size_bytes");
                    asset_descriptor.expected_sha256 =
                        get_optional_string(asset, "expected_sha256", "dataset.assets[].expected_sha256");
                    asset_descriptor.actual_sha256 =
                        get_optional_string(asset, "actual_sha256", "dataset.assets[].actual_sha256");
                    asset_descriptor.message =
                        get_optional_string(asset, "message", "dataset.assets[].message");
                    descriptor.assets.push_back(std::move(asset_descriptor));
                }
            }
        }

        [[nodiscard]] OverlayDescriptor make_overlay_descriptor(std::string key, bool enabled,
                                                                std::optional<std::string> label = std::nullopt)
        {
            OverlayDescriptor descriptor{};
            descriptor.key = std::move(key);
            descriptor.label = label.value_or(descriptor.key);
            descriptor.default_enabled = enabled;
            return descriptor;
        }

        void populate_rendering(const JsonValue::object_type& root, ExperimentConfigurationSummary& summary)
        {
            summary.rendering_presets.clear();
            summary.selected_rendering_preset.reset();

            if (const auto* selected = find_member(root, "selected_rendering_preset"))
            {
                if (!selected->is_null())
                {
                    summary.selected_rendering_preset = selected->as_string("selected_rendering_preset");
                }
            }

            if (const auto* presets_value = find_member(root, "rendering_presets"))
            {
                const auto& presets = presets_value->as_array("rendering_presets");
                summary.rendering_presets.reserve(presets.size());
                for (const auto& preset_value : presets)
                {
                    const auto& preset_object = preset_value.as_object("rendering_presets[]");
                    RenderingPresetDescriptor preset{};
                    preset.identifier = get_string(preset_object, "id", "rendering_presets[].id", "research");
                    preset.label = get_string(preset_object, "label", "rendering_presets[].label", preset.identifier);

                    if (const auto* resolution = find_member(preset_object, "default_resolution"))
                    {
                        const auto& resolution_object = resolution->as_object("rendering_presets[].default_resolution");
                        preset.default_resolution.first = static_cast<int>(std::lround(
                            get_number(resolution_object, "width",
                                      "rendering_presets[].default_resolution.width", 1280.0)));
                        preset.default_resolution.second = static_cast<int>(std::lround(
                            get_number(resolution_object, "height",
                                      "rendering_presets[].default_resolution.height", 720.0)));
                    }
                    else
                    {
                        preset.default_resolution = {1280, 720};
                    }

                    preset.shading_modes.clear();
                    if (const auto* shading_modes = find_member(preset_object, "shading_modes"))
                    {
                        for (const auto& entry : shading_modes->as_array("rendering_presets[].shading_modes"))
                        {
                            preset.shading_modes.push_back(entry.as_string("rendering_presets[].shading_modes[]"));
                        }
                    }

                    preset.overlays.clear();
                    if (const auto* overlays = find_member(preset_object, "overlays"))
                    {
                        for (const auto& overlay_value : overlays->as_array("rendering_presets[].overlays"))
                        {
                            const auto& overlay_object = overlay_value.as_object("rendering_presets[].overlays[]");
                            const std::string key = get_string(overlay_object, "key",
                                                               "rendering_presets[].overlays[].key");
                            const std::string label = get_string(overlay_object, "label",
                                                                 "rendering_presets[].overlays[].label", key);
                            const bool enabled = get_bool(overlay_object, "default_enabled",
                                                          "rendering_presets[].overlays[].default_enabled", false);
                            preset.overlays.push_back(make_overlay_descriptor(key, enabled, label));
                        }
                    }

                    summary.rendering_presets.push_back(std::move(preset));
                }
                return;
            }

            const auto* rendering_value = find_member(root, "rendering");
            if (rendering_value == nullptr || rendering_value->is_null())
            {
                return;
            }

            const auto& rendering = rendering_value->as_object("rendering");
            RenderingPresetDescriptor preset{};
            preset.identifier = get_string(rendering, "preset", "rendering.preset", "research");
            preset.label = preset.identifier;
            preset.default_resolution = {1280, 720};

            if (const auto* resolution = find_member(rendering, "resolution"))
            {
                const auto& resolution_object = resolution->as_object("rendering.resolution");
                preset.default_resolution.first = static_cast<int>(std::lround(
                    get_number(resolution_object, "width", "rendering.resolution.width", 1280.0)));
                preset.default_resolution.second = static_cast<int>(std::lround(
                    get_number(resolution_object, "height", "rendering.resolution.height", 720.0)));
            }

            preset.shading_modes.clear();
            if (const auto* shading = find_member(rendering, "shading_modes"))
            {
                for (const auto& entry : shading->as_array("rendering.shading_modes"))
                {
                    preset.shading_modes.push_back(entry.as_string("rendering.shading_modes[]"));
                }
            }
            else if (const auto* shading_mode = find_member(rendering, "shading_mode"))
            {
                preset.shading_modes.push_back(shading_mode->as_string("rendering.shading_mode"));
            }

            preset.overlays.clear();
            if (const auto* overlays = find_member(rendering, "overlays"))
            {
                for (const auto& [key, value] : overlays->as_object("rendering.overlays"))
                {
                    preset.overlays.push_back(
                        make_overlay_descriptor(key, value.as_bool("rendering.overlays value")));
                }
            }

            summary.rendering_presets.push_back(std::move(preset));
        }

        void populate_algorithm_variants(const JsonValue::object_type& root, ExperimentConfigurationSummary& summary)
        {
            summary.algorithm_variants.clear();
            summary.selected_algorithm_variant.reset();

            if (const auto* selected = find_member(root, "selected_algorithm_variant"))
            {
                if (!selected->is_null())
                {
                    summary.selected_algorithm_variant = selected->as_string("selected_algorithm_variant");
                }
            }

            const auto* variants_value = find_member(root, "algorithm_variants");
            if (variants_value == nullptr || variants_value->is_null())
            {
                return;
            }

            const auto& variants = variants_value->as_array("algorithm_variants");
            summary.algorithm_variants.reserve(variants.size());
            for (const auto& variant_value : variants)
            {
                const auto& variant_object = variant_value.as_object("algorithm_variants[]");
                AlgorithmVariantDescriptor descriptor{};
                descriptor.identifier = get_string(variant_object, "id", "algorithm_variants[].id");
                descriptor.label = get_string(variant_object, "label", "algorithm_variants[].label", descriptor.identifier);
                descriptor.description =
                    get_optional_string(variant_object, "description", "algorithm_variants[].description");
                summary.algorithm_variants.push_back(std::move(descriptor));
            }
        }

        [[nodiscard]] std::string format_vec3(const JsonValue& value, std::string_view context)
        {
            const auto& array = value.as_array(context);
            if (array.empty())
            {
                return "[]";
            }
            std::ostringstream stream;
            stream.setf(std::ios::fixed, std::ios::floatfield);
            stream.precision(4);
            stream << '(';
            for (std::size_t index = 0; index < array.size(); ++index)
            {
                if (index != 0)
                {
                    stream << ", ";
                }
                stream << array[index].as_number(context);
            }
            stream << ')';
            return stream.str();
        }

        [[nodiscard]] std::string describe_camera(const JsonValue::object_type& runtime)
        {
            const auto* camera_value = find_member(runtime, "camera");
            if (camera_value == nullptr || camera_value->is_null())
            {
                return {};
            }
            const auto& camera = camera_value->as_object("runtime.camera");
            std::ostringstream stream;
            bool first_component = true;
            const auto mode = get_string(camera, "mode", "runtime.camera.mode");
            if (!mode.empty())
            {
                stream << "mode=" << mode;
                first_component = false;
            }
            if (const auto* position = find_member(camera, "position"))
            {
                if (!first_component)
                {
                    stream << ", ";
                }
                stream << "position=" << format_vec3(*position, "runtime.camera.position");
                first_component = false;
            }
            if (const auto* target = find_member(camera, "target"))
            {
                if (!first_component)
                {
                    stream << ", ";
                }
                stream << "target=" << format_vec3(*target, "runtime.camera.target");
                first_component = false;
            }
            return stream.str();
        }

        [[nodiscard]] std::string describe_simulation(const JsonValue::object_type& runtime)
        {
            const auto* simulation_value = find_member(runtime, "simulation");
            if (simulation_value == nullptr || simulation_value->is_null())
            {
                return {};
            }
            const auto& simulation = simulation_value->as_object("runtime.simulation");
            std::ostringstream stream;
            const double dt = get_number(simulation, "timestep_seconds", "runtime.simulation.timestep_seconds", 0.0);
            if (dt > 0.0)
            {
                stream.setf(std::ios::fixed, std::ios::floatfield);
                stream.precision(6);
                stream << "dt=" << dt;
            }
            const double substeps = get_number(simulation, "max_substeps", "runtime.simulation.max_substeps", 0.0);
            if (substeps > 0.0)
            {
                if (stream.tellp() > 0)
                {
                    stream << ", ";
                }
                stream << "max_substeps=" << substeps;
            }
            return stream.str();
        }

        void populate_runtime(const JsonValue::object_type& root, RuntimeSummary& runtime_summary)
        {
            const auto* runtime_value = find_member(root, "runtime");
            if (runtime_value == nullptr || runtime_value->is_null())
            {
                runtime_summary = RuntimeSummary{};
                return;
            }

            const auto& runtime = runtime_value->as_object("runtime");
            runtime_summary.dataset_identifier = get_string(runtime, "dataset", "runtime.dataset");
            runtime_summary.scene_manifest = get_string(runtime, "scene_manifest", "runtime.scene_manifest");
            runtime_summary.scene_manifest_path =
                get_optional_string(runtime, "scene_manifest_path", "runtime.scene_manifest_path");
            runtime_summary.scene_entry_point = get_string(runtime, "scene_entry_point", "runtime.scene_entry_point");
            runtime_summary.camera_description = describe_camera(runtime);
            runtime_summary.simulation_description = describe_simulation(runtime);
            runtime_summary.schema_version = static_cast<int>(std::lround(
                get_number(runtime, "schema_version", "runtime.schema_version", 0.0)));

            runtime_summary.hot_reload.enabled = false;
            runtime_summary.hot_reload.watch_interval_seconds.reset();
            if (const auto* hot_reload = find_member(runtime, "hot_reload"))
            {
                const auto& hot_reload_object = hot_reload->as_object("runtime.hot_reload");
                runtime_summary.hot_reload.enabled =
                    get_bool(hot_reload_object, "enabled", "runtime.hot_reload.enabled", false);
                runtime_summary.hot_reload.watch_interval_seconds =
                    get_optional_number(hot_reload_object, "watch_interval_seconds",
                                         "runtime.hot_reload.watch_interval_seconds");
            }
        }

        void populate_telemetry(const JsonValue::object_type& root, ExperimentConfigurationSummary& summary)
        {
            const auto* telemetry_value = find_member(root, "telemetry");
            if (telemetry_value == nullptr || telemetry_value->is_null())
            {
                summary.telemetry.reset();
                return;
            }

            const auto& telemetry = telemetry_value->as_object("telemetry");
            TelemetryConfigurationDescriptor descriptor{};
            descriptor.schema_version = static_cast<int>(std::lround(
                get_number(telemetry, "schema_version", "telemetry.schema_version", 0.0)));

            descriptor.outputs.clear();
            if (const auto* outputs_value = find_member(telemetry, "outputs"))
            {
                const auto& outputs = outputs_value->as_array("telemetry.outputs");
                descriptor.outputs.reserve(outputs.size());
                for (const auto& output_value : outputs)
                {
                    const auto& output = output_value.as_object("telemetry.outputs[]");
                    TelemetryOutputDescriptor entry{};
                    entry.kind = get_string(output, "kind", "telemetry.outputs[].kind");
                    entry.path = get_optional_string(output, "path", "telemetry.outputs[].path");
                    entry.template_path = get_optional_string(output, "template", "telemetry.outputs[].template");
                    descriptor.outputs.push_back(std::move(entry));
                }
            }

            descriptor.metrics.clear();
            if (const auto* metrics_value = find_member(telemetry, "metrics"))
            {
                const auto& metrics = metrics_value->as_array("telemetry.metrics");
                descriptor.metrics.reserve(metrics.size());
                for (const auto& metric_value : metrics)
                {
                    const auto& metric = metric_value.as_object("telemetry.metrics[]");
                    TelemetryMetricDescriptor entry{};
                    entry.name = get_string(metric, "name", "telemetry.metrics[].name");
                    entry.statistic = get_string(metric, "statistic", "telemetry.metrics[].statistic");
                    descriptor.metrics.push_back(std::move(entry));
                }
            }

            descriptor.sampling.reset();
            if (const auto* sampling_value = find_member(telemetry, "sampling"))
            {
                if (!sampling_value->is_null())
                {
                    const auto& sampling = sampling_value->as_object("telemetry.sampling");
                    TelemetrySamplingDescriptor sampling_descriptor{};
                    sampling_descriptor.frame_interval = static_cast<int>(std::lround(
                        get_number(sampling, "frame_interval", "telemetry.sampling.frame_interval", 0.0)));
                    sampling_descriptor.include_debug_overlays =
                        get_bool(sampling, "include_debug_overlays",
                                 "telemetry.sampling.include_debug_overlays", false);
                    descriptor.sampling = sampling_descriptor;
                }
            }

            summary.telemetry = std::move(descriptor);
        }

        [[nodiscard]] BenchmarkCommandDescriptor parse_benchmark_command(const JsonValue::object_type& object,
                                                                         std::string_view context)
        {
            BenchmarkCommandDescriptor descriptor{};
            const std::string output_context = std::string{context} + ".output";
            descriptor.output = get_string(object, "output", output_context);
            if (const auto* command_value = find_member(object, "command"))
            {
                if (!command_value->is_null())
                {
                    const std::string command_context = std::string{context} + ".command";
                    const auto& commands = command_value->as_array(command_context);
                    descriptor.command.reserve(commands.size());
                    for (const auto& token : commands)
                    {
                        const std::string element_context = command_context + "[]";
                        descriptor.command.push_back(token.as_string(element_context));
                    }
                }
            }
            return descriptor;
        }

        void populate_benchmarks(const JsonValue::object_type& root, ExperimentConfigurationSummary& summary)
        {
            const auto* benchmarks_value = find_member(root, "benchmarks");
            if (benchmarks_value == nullptr || benchmarks_value->is_null())
            {
                summary.benchmarks.reset();
                return;
            }

            const auto& benchmarks = benchmarks_value->as_object("benchmarks");
            BenchmarkConfigurationDescriptor descriptor{};
            descriptor.schema_version = static_cast<int>(std::lround(
                get_number(benchmarks, "schema_version", "benchmarks.schema_version", 0.0)));

            descriptor.scenarios.clear();
            if (const auto* scenarios_value = find_member(benchmarks, "scenarios"))
            {
                const auto& scenarios = scenarios_value->as_array("benchmarks.scenarios");
                descriptor.scenarios.reserve(scenarios.size());
                for (const auto& scenario_value : scenarios)
                {
                    const auto& scenario = scenario_value.as_object("benchmarks.scenarios[]");
                    BenchmarkScenarioDescriptor scenario_descriptor{};
                    scenario_descriptor.identifier = get_string(scenario, "id", "benchmarks.scenarios[].id");
                    scenario_descriptor.name = get_string(scenario, "name", "benchmarks.scenarios[].name",
                                                          scenario_descriptor.identifier);
                    scenario_descriptor.dataset = get_string(scenario, "dataset", "benchmarks.scenarios[].dataset");
                    scenario_descriptor.rendering_preset =
                        get_string(scenario, "rendering_preset", "benchmarks.scenarios[].rendering_preset");
                    scenario_descriptor.runtime_profile =
                        get_string(scenario, "runtime_profile", "benchmarks.scenarios[].runtime_profile");
                    if (const auto* engine_value = find_member(scenario, "engine"))
                    {
                        scenario_descriptor.engine =
                            parse_benchmark_command(engine_value->as_object("benchmarks.scenarios[].engine"),
                                                    "benchmarks.scenarios[].engine");
                    }
                    if (const auto* reference_value = find_member(scenario, "reference"))
                    {
                        scenario_descriptor.reference =
                            parse_benchmark_command(reference_value->as_object("benchmarks.scenarios[].reference"),
                                                    "benchmarks.scenarios[].reference");
                    }

                    scenario_descriptor.metrics.clear();
                    if (const auto* metrics_value = find_member(scenario, "metrics"))
                    {
                        const auto& metrics = metrics_value->as_array("benchmarks.scenarios[].metrics");
                        scenario_descriptor.metrics.reserve(metrics.size());
                        for (const auto& metric_value : metrics)
                        {
                            const auto& metric = metric_value.as_object("benchmarks.scenarios[].metrics[]");
                            BenchmarkMetricDescriptor metric_descriptor{};
                            metric_descriptor.name =
                                get_string(metric, "name", "benchmarks.scenarios[].metrics[].name");
                            metric_descriptor.higher_is_better =
                                get_bool(metric, "higher_is_better", "benchmarks.scenarios[].metrics[].higher_is_better",
                                         false);
                            if (const auto* threshold_value = find_member(metric, "threshold"))
                            {
                                if (!threshold_value->is_null())
                                {
                                    const auto& threshold =
                                        threshold_value->as_object("benchmarks.scenarios[].metrics[].threshold");
                                    metric_descriptor.threshold.mode =
                                        get_string(threshold, "mode",
                                                   "benchmarks.scenarios[].metrics[].threshold.mode");
                                    metric_descriptor.threshold.limit =
                                        get_number(threshold, "limit",
                                                   "benchmarks.scenarios[].metrics[].threshold.limit", 0.0);
                                }
                            }
                            scenario_descriptor.metrics.push_back(std::move(metric_descriptor));
                        }
                    }

                    descriptor.scenarios.push_back(std::move(scenario_descriptor));
                }
            }

            summary.benchmarks = std::move(descriptor);
        }
    } // namespace

    ExperimentConfigurationSummary load_summary_from_json(std::string_view json_buffer)
    {
        JsonParser parser(json_buffer);
        JsonValue root_value = parser.parse();
        const auto& root = root_value.as_object("root");

        ExperimentConfigurationSummary summary{};

        summary.datasets.clear();
        if (const auto* datasets_value = find_member(root, "datasets"))
        {
            const auto& datasets = datasets_value->as_array("datasets");
            summary.datasets.reserve(datasets.size());
            for (const auto& dataset_value : datasets)
            {
                DatasetDescriptor descriptor{};
                populate_dataset(dataset_value, descriptor);
                summary.datasets.push_back(std::move(descriptor));
            }
        }

        const auto* selected_dataset = find_member(root, "selected_dataset");
        if (selected_dataset != nullptr && !selected_dataset->is_null())
        {
            summary.selected_dataset = selected_dataset->as_string("selected_dataset");
        }

        populate_rendering(root, summary);
        populate_algorithm_variants(root, summary);
        populate_runtime(root, summary.runtime);
        populate_telemetry(root, summary);
        populate_benchmarks(root, summary);

        return summary;
    }

    ExperimentConfigurationSummary load_summary_from_json(const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
        {
            throw std::runtime_error("Failed to open configuration summary: " + path.string());
        }
        const std::string buffer((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return load_summary_from_json(std::string_view{buffer});
    }
} // namespace engine::tools::sandbox
