#include "remesh_cli.hpp"

#include "engine/geometry/api.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace engine::geometry::tools
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string_view value) noexcept
        {
            std::string lowered{value};
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch)
            {
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

        [[nodiscard]] std::string_view to_mode_string(RemeshingMode mode) noexcept
        {
            switch (mode)
            {
            case RemeshingMode::kUniform:
                return "uniform";
            case RemeshingMode::kFeaturePreserving:
                return "feature_preserving";
            case RemeshingMode::kAdaptive:
                return "adaptive";
            }
            return "uniform";
        }

        [[nodiscard]] std::string_view to_parameterization_string(ParameterizationMode mode) noexcept
        {
            switch (mode)
            {
            case ParameterizationMode::kNone:
                return "none";
            case ParameterizationMode::kReuseExisting:
                return "reuse_existing";
            case ParameterizationMode::kGenerateLscm:
                return "generate_lscm";
            case ParameterizationMode::kGenerateAbfpp:
                return "generate_abfpp";
            }
            return "none";
        }

        [[nodiscard]] std::string sanitize_identifier(std::string_view identifier)
        {
            std::string sanitized{};
            sanitized.reserve(identifier.size());

            const auto push_separator = [&]()
            {
                if (!sanitized.empty() && sanitized.back() != '-')
                {
                    sanitized.push_back('-');
                }
            };

            for (unsigned char ch : identifier)
            {
                if (std::isalnum(ch))
                {
                    sanitized.push_back(static_cast<char>(std::tolower(ch)));
                    continue;
                }

                if (ch == '-' || ch == '_')
                {
                    sanitized.push_back('-');
                    continue;
                }

                push_separator();
            }

            while (!sanitized.empty() && sanitized.back() == '-')
            {
                sanitized.pop_back();
            }

            if (sanitized.empty())
            {
                sanitized = "remesh-job";
            }

            return sanitized;
        }

        [[nodiscard]] std::string path_or_placeholder(const std::filesystem::path& path,
                                                      std::string_view placeholder)
        {
            if (path.empty())
            {
                return std::string{placeholder};
            }

            const std::string value = path.generic_string();
            return value.empty() ? std::string{placeholder} : value;
        }

        template <typename T>
        [[nodiscard]] T safe_value(T value) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                return std::isfinite(value) ? value : T{0};
            }
            else
            {
                return value;
            }
        }

        class Sha256
        {
        public:
            Sha256() noexcept
            {
                reset();
            }

            void update(const std::uint8_t* data, std::size_t size) noexcept
            {
                if (size == 0)
                {
                    return;
                }

                total_bits_ += static_cast<std::uint64_t>(size) * 8ULL;

                while (size > 0)
                {
                    const std::size_t space = block_.size() - buffer_size_;
                    const std::size_t to_copy = std::min(space, size);
                    std::memcpy(block_.data() + buffer_size_, data, to_copy);
                    buffer_size_ += to_copy;
                    data += to_copy;
                    size -= to_copy;

                    if (buffer_size_ == block_.size())
                    {
                        transform(block_.data());
                        buffer_size_ = 0U;
                    }
                }
            }

            void finalize(std::uint8_t digest[32]) noexcept
            {
                const std::uint64_t total_bits = total_bits_;

                block_[buffer_size_] = 0x80U;
                ++buffer_size_;

                if (buffer_size_ > 56U)
                {
                    std::fill(block_.begin() + buffer_size_, block_.end(), 0);
                    transform(block_.data());
                    buffer_size_ = 0U;
                }

                std::fill(block_.begin() + buffer_size_, block_.begin() + 56U, 0);

                for (std::size_t index = 0; index < 8U; ++index)
                {
                    const std::size_t shift = 56U - (index * 8U);
                    block_[56U + index] = static_cast<std::uint8_t>((total_bits >> shift) & 0xFFU);
                }

                transform(block_.data());
                buffer_size_ = 0U;

                for (std::size_t index = 0; index < state_.size(); ++index)
                {
                    digest[index * 4U] = static_cast<std::uint8_t>((state_[index] >> 24U) & 0xFFU);
                    digest[index * 4U + 1U] = static_cast<std::uint8_t>((state_[index] >> 16U) & 0xFFU);
                    digest[index * 4U + 2U] = static_cast<std::uint8_t>((state_[index] >> 8U) & 0xFFU);
                    digest[index * 4U + 3U] = static_cast<std::uint8_t>(state_[index] & 0xFFU);
                }
            }

        private:
            void reset() noexcept
            {
                state_ = {
                    0x6A09E667U,
                    0xBB67AE85U,
                    0x3C6EF372U,
                    0xA54FF53AU,
                    0x510E527FU,
                    0x9B05688CU,
                    0x1F83D9ABU,
                    0x5BE0CD19U,
                };
                total_bits_ = 0U;
                buffer_size_ = 0U;
            }

            void transform(const std::uint8_t* chunk) noexcept
            {
                static constexpr std::array<std::uint32_t, 64> kConstants{
                    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U, 0x3956C25BU, 0x59F111F1U, 0x923F82A4U,
                    0xAB1C5ED5U, 0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U, 0x72BE5D74U, 0x80DEB1FEU,
                    0x9BDC06A7U, 0xC19BF174U, 0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU, 0x2DE92C6FU,
                    0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU, 0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
                    0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U, 0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU,
                    0x53380D13U, 0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U, 0xA2BFE8A1U, 0xA81A664BU,
                    0xC24B8B70U, 0xC76C51A3U, 0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U, 0x19A4C116U,
                    0x1E376C08U, 0x2748774CU, 0x34B0BCB5U, 0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
                    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U, 0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U,
                    0xC67178F2U,
                };

                auto right_rotate = [](std::uint32_t value, std::uint32_t bits) noexcept -> std::uint32_t
                {
                    return (value >> bits) | (value << (32U - bits));
                };

                std::array<std::uint32_t, 64> w{};
                for (std::size_t index = 0; index < 16U; ++index)
                {
                    w[index] = (static_cast<std::uint32_t>(chunk[index * 4U]) << 24U) |
                        (static_cast<std::uint32_t>(chunk[index * 4U + 1U]) << 16U) |
                        (static_cast<std::uint32_t>(chunk[index * 4U + 2U]) << 8U) |
                        static_cast<std::uint32_t>(chunk[index * 4U + 3U]);
                }

                for (std::size_t index = 16U; index < 64U; ++index)
                {
                    const std::uint32_t s0 = right_rotate(w[index - 15U], 7U) ^ right_rotate(w[index - 15U], 18U) ^
                        (w[index - 15U] >> 3U);
                    const std::uint32_t s1 = right_rotate(w[index - 2U], 17U) ^ right_rotate(w[index - 2U], 19U) ^
                        (w[index - 2U] >> 10U);
                    w[index] = w[index - 16U] + s0 + w[index - 7U] + s1;
                }

                std::uint32_t a = state_[0];
                std::uint32_t b = state_[1];
                std::uint32_t c = state_[2];
                std::uint32_t d = state_[3];
                std::uint32_t e = state_[4];
                std::uint32_t f = state_[5];
                std::uint32_t g = state_[6];
                std::uint32_t h = state_[7];

                for (std::size_t index = 0; index < 64U; ++index)
                {
                    const std::uint32_t S1 = right_rotate(e, 6U) ^ right_rotate(e, 11U) ^ right_rotate(e, 25U);
                    const std::uint32_t ch = (e & f) ^ ((~e) & g);
                    const std::uint32_t temp1 = h + S1 + ch + kConstants[index] + w[index];
                    const std::uint32_t S0 = right_rotate(a, 2U) ^ right_rotate(a, 13U) ^ right_rotate(a, 22U);
                    const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                    const std::uint32_t temp2 = S0 + maj;

                    h = g;
                    g = f;
                    f = e;
                    e = d + temp1;
                    d = c;
                    c = b;
                    b = a;
                    a = temp1 + temp2;
                }

                state_[0] += a;
                state_[1] += b;
                state_[2] += c;
                state_[3] += d;
                state_[4] += e;
                state_[5] += f;
                state_[6] += g;
                state_[7] += h;
            }

            std::array<std::uint32_t, 8> state_{};
            std::uint64_t total_bits_{0U};
            std::array<std::uint8_t, 64> block_{};
            std::size_t buffer_size_{0U};
        };

        [[nodiscard]] std::string to_hex_string(const std::uint8_t* data, std::size_t length)
        {
            static constexpr char digits[] = "0123456789abcdef";
            std::string hex;
            hex.resize(length * 2U);
            for (std::size_t index = 0; index < length; ++index)
            {
                const std::uint8_t value = data[index];
                hex[index * 2U] = digits[value >> 4U];
                hex[index * 2U + 1U] = digits[value & 0x0FU];
            }
            return hex;
        }

        [[nodiscard]] engine::Result<DatasetFileDigest, std::string> compute_file_digest(const std::filesystem::path& path)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
            {
                std::ostringstream message;
                message << "Failed to open file '" << path.string() << "' for hashing";
                return message.str();
            }

            Sha256 hasher{};
            std::array<std::uint8_t, 4096> buffer{};
            std::uintmax_t total_size = 0U;

            while (true)
            {
                stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
                const std::streamsize count = stream.gcount();
                if (count > 0)
                {
                    hasher.update(buffer.data(), static_cast<std::size_t>(count));
                    total_size += static_cast<std::uintmax_t>(count);
                }

                if (stream.eof())
                {
                    break;
                }

                if (stream.fail())
                {
                    std::ostringstream message;
                    message << "Failed to read file '" << path.string() << "' while computing SHA-256";
                    return message.str();
                }
            }

            std::array<std::uint8_t, 32> digest{};
            hasher.finalize(digest.data());

            DatasetFileDigest metadata{};
            metadata.path = path;
            metadata.size_bytes = total_size;
            metadata.sha256 = to_hex_string(digest.data(), digest.size());
            return metadata;
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

            if (argument == "--manifest-output")
            {
                if (index + 1 >= arguments.size())
                {
                    return RemeshCliOptionsResult{missing_value_error(argument)};
                }
                ++index;
                options.manifest_output_path = std::filesystem::path{arguments[index]};
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
                "Uniform and feature-preserving remeshing require --target-edge-length or --relative-edge-scale"
            };
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
                    "Adaptive remeshing requires an error budget (--max-normal-deviation or --max-surface-deviation)"
                };
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

        const auto input_digest = compute_file_digest(options.input_path);
        if (!input_digest.has_value())
        {
            return RemeshCliExecution{input_digest.error()};
        }
        summary.input_file = input_digest.value();

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

        const auto output_digest = compute_file_digest(options.output_path);
        if (!output_digest.has_value())
        {
            return RemeshCliExecution{output_digest.error()};
        }
        summary.output_file = output_digest.value();

        if (options.manifest_output_path.has_value())
        {
            const std::filesystem::path& manifest_path = options.manifest_output_path.value();
            const std::filesystem::path parent = manifest_path.parent_path();
            if (!parent.empty())
            {
                std::error_code ec{};
                std::filesystem::create_directories(parent, ec);
                if (ec)
                {
                    std::ostringstream stream;
                    stream << "Failed to create manifest directory '" << parent.string() << "': " << ec.message();
                    return RemeshCliExecution{stream.str()};
                }
            }

            const std::string manifest = BuildDatasetManifestEntry(options, summary);
            std::ofstream manifest_stream{manifest_path, std::ios::binary};
            if (!manifest_stream)
            {
                std::ostringstream stream;
                stream << "Failed to open dataset manifest path '" << manifest_path.string() << "'";
                return RemeshCliExecution{stream.str()};
            }

            manifest_stream << manifest;
            if (!manifest_stream)
            {
                std::ostringstream stream;
                stream << "Failed to write dataset manifest to '" << manifest_path.string() << "'";
                return RemeshCliExecution{stream.str()};
            }
        }

        return RemeshCliExecution{summary};
    }

    std::string BuildDatasetManifestEntry(const RemeshCliOptions& options,
                                          const RemeshCliExecutionResult& result) noexcept
    {
        const MeshEdgeStatistics output_edges = ComputeMeshEdgeStatistics(result.output.mesh);
        const RemeshStatistics& statistics = result.output.statistics;
        const std::string dataset_id = options.job_label.has_value()
                                           ? sanitize_identifier(options.job_label.value())
                                           : sanitize_identifier(options.output_path.stem().string());

        std::ostringstream yaml;
        yaml << std::fixed << std::setprecision(4) << std::boolalpha;
        yaml << "datasets:\n";
        yaml << "  - id: " << dataset_id << "\n";
        yaml << "    schema:\n";
        yaml << "      id: ai-004.dataset\n";
        yaml << "      version: 2\n";
        yaml << "    kind: geometry.remesh\n";
        if (options.job_label.has_value())
        {
            yaml << "    job_label: \"" << options.job_label.value() << "\"\n";
        }
        yaml << "    tags: [geometry, remesh]\n";
        yaml << "    source:\n";
        yaml << "      generator: geometry_remesh\n";
        yaml << "      mesh: " << path_or_placeholder(options.input_path, "<unknown>") << "\n";
        yaml << "      mesh_sha256: " << result.input_file.sha256 << "\n";
        yaml << "      mesh_size_bytes: " << result.input_file.size_bytes << "\n";
        yaml << "    outputs:\n";
        yaml << "      mesh: " << path_or_placeholder(options.output_path, "<unspecified>") << "\n";
        yaml << "      mesh_sha256: " << result.output_file.sha256 << "\n";
        yaml << "      mesh_size_bytes: " << result.output_file.size_bytes << "\n";
        yaml << "    remeshing:\n";
        yaml << "      mode: " << to_mode_string(options.mode) << "\n";

        const bool has_targets = options.targets.target_edge_length.has_value() ||
            options.targets.relative_edge_scale.has_value() ||
            options.targets.maximum_normal_deviation_degrees.has_value() ||
            options.targets.maximum_surface_deviation.has_value();
        if (has_targets)
        {
            yaml << "      targets:\n";
            if (options.targets.target_edge_length.has_value())
            {
                yaml << "        target_edge_length: " << safe_value(options.targets.target_edge_length.value()) <<
                    "\n";
            }
            if (options.targets.relative_edge_scale.has_value())
            {
                yaml << "        relative_edge_scale: " << safe_value(options.targets.relative_edge_scale.value()) <<
                    "\n";
            }
            if (options.targets.maximum_normal_deviation_degrees.has_value())
            {
                yaml << "        max_normal_deviation_degrees: "
                    << safe_value(options.targets.maximum_normal_deviation_degrees.value()) << "\n";
            }
            if (options.targets.maximum_surface_deviation.has_value())
            {
                yaml << "        max_surface_deviation: "
                    << safe_value(options.targets.maximum_surface_deviation.value()) << "\n";
            }
        }

        yaml << "      feature_preservation:\n";
        yaml << "        lock_boundary_edges: " << options.feature_preservation.lock_boundary_edges << "\n";
        yaml << "        lock_feature_edges: " << options.feature_preservation.lock_feature_edges << "\n";
        yaml << "        minimum_feature_angle_degrees: "
            << safe_value(options.feature_preservation.minimum_feature_angle_degrees) << "\n";

        yaml << "    metrics:\n";
        yaml << "      input:\n";
        yaml << "        vertices: " << result.input_vertex_count << "\n";
        yaml << "        faces: " << result.input_face_count << "\n";
        yaml << "        edge_length:\n";
        yaml << "          min: " << safe_value(result.input_edge_statistics.min_edge_length) << "\n";
        yaml << "          max: " << safe_value(result.input_edge_statistics.max_edge_length) << "\n";
        yaml << "          mean: " << safe_value(result.input_edge_statistics.mean_edge_length()) << "\n";
        yaml << "      output:\n";
        yaml << "        vertices: " << result.output.mesh.positions.size() << "\n";
        yaml << "        faces: " << (result.output.mesh.indices.size() / 3U) << "\n";
        yaml << "        edge_length:\n";
        yaml << "          min: " << safe_value(statistics.min_edge_length) << "\n";
        yaml << "          max: " << safe_value(statistics.max_edge_length) << "\n";
        yaml << "          mean: " << safe_value(output_edges.mean_edge_length()) << "\n";

        if (options.parameterization.mode != ParameterizationMode::kNone)
        {
            const ParameterizationSummary& summary = result.output.parameterization;
            yaml << "    parameterization:\n";
            yaml << "      mode: " << to_parameterization_string(options.parameterization.mode) << "\n";
            if (options.parameterization.target_texel_density > 0.0F)
            {
                yaml << "      target_texel_density: "
                    << safe_value(options.parameterization.target_texel_density) << "\n";
            }
            yaml << "      texel_density: " << safe_value(summary.texel_density) << "\n";
            yaml << "      chart_count: " << summary.chart_count << "\n";
            yaml << "      average_stretch: " << safe_value(summary.average_stretch) << "\n";
            yaml << "      max_stretch: " << safe_value(summary.max_stretch) << "\n";
            yaml << "      fill_ratio: " << safe_value(summary.fill_ratio) << "\n";
            yaml << "      total_seam_length: " << safe_value(summary.total_seam_length) << "\n";
            yaml << "      atlas_area: " << safe_value(summary.atlas_area) << "\n";
            yaml << "      total_chart_area: " << safe_value(summary.total_chart_area) << "\n";
            if (!summary.charts.empty())
            {
                yaml << "      charts:\n";
                for (std::size_t index = 0; index < summary.charts.size(); ++index)
                {
                    const ParameterizationChart& chart = summary.charts[index];
                    yaml << "        - index: " << index << "\n";
                    yaml << "          min_uv: [" << safe_value(chart.min_uv[0]) << ", "
                        << safe_value(chart.min_uv[1]) << "]\n";
                    yaml << "          max_uv: [" << safe_value(chart.max_uv[0]) << ", "
                        << safe_value(chart.max_uv[1]) << "]\n";
                    yaml << "          translation: [" << safe_value(chart.translation[0]) << ", "
                        << safe_value(chart.translation[1]) << "]\n";
                    yaml << "          scale: " << safe_value(chart.scale) << "\n";
                    yaml << "          area: " << safe_value(chart.area) << "\n";
                    yaml << "          boundary_length: " << safe_value(chart.boundary_length) << "\n";
                }
            }
        }

        yaml << "    statistics:\n";
        yaml << "      iterations: " << statistics.iteration_count << "\n";
        yaml << "      splits: " << statistics.split_count << "\n";
        yaml << "      collapses: " << statistics.collapse_count << "\n";
        yaml << "      duration_ms: " << safe_value(statistics.duration_ms) << "\n";
        yaml << "      max_error: " << safe_value(statistics.max_error) << "\n";
        yaml << "      min_edge_length: " << safe_value(statistics.min_edge_length) << "\n";
        yaml << "      max_edge_length: " << safe_value(statistics.max_edge_length) << "\n";
        yaml << "      max_surface_deviation: " << safe_value(statistics.max_surface_deviation) << "\n";
        yaml << "      mean_surface_deviation: " << safe_value(statistics.mean_surface_deviation) << "\n";
        yaml << "      rms_surface_deviation: " << safe_value(statistics.rms_surface_deviation) << "\n";
        yaml << "      triangles: " << result.output.statistics.triangle_count << "\n";
        yaml << "      triangle_quality:\n";
        yaml << "        min: " << safe_value(statistics.min_triangle_quality) << "\n";
        yaml << "        mean: " << safe_value(statistics.mean_triangle_quality) << "\n";
        yaml << "        max: " << safe_value(statistics.max_triangle_quality) << "\n";

        return yaml.str();
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
            << " splits=" << statistics.split_count
            << " collapses=" << statistics.collapse_count << "\n";
        stream << "  Duration(ms)=" << statistics.duration_ms << "\n";
        stream << "  Metrics: min_edge=" << statistics.min_edge_length
            << " max_edge=" << statistics.max_edge_length
            << " max_error=" << statistics.max_error << "\n";
        stream << "  Triangle quality: min=" << statistics.min_triangle_quality
            << " mean=" << statistics.mean_triangle_quality
            << " max=" << statistics.max_triangle_quality << "\n";
        stream << "  Surface deviation: max=" << statistics.max_surface_deviation
            << " mean=" << statistics.mean_surface_deviation
            << " rms=" << statistics.rms_surface_deviation << "\n";

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
                << " max_stretch=" << summary.max_stretch
                << " fill_ratio=" << summary.fill_ratio
                << " seam_length=" << summary.total_seam_length << "\n";

            if (options.verbose)
            {
                stream << "    Atlas: area=" << summary.atlas_area
                    << " charts_area=" << summary.total_chart_area
                    << " fill_ratio=" << summary.fill_ratio << "\n";

                for (std::size_t chart_index = 0; chart_index < summary.charts.size(); ++chart_index)
                {
                    const ParameterizationChart& chart = summary.charts[chart_index];
                    stream << "    Chart " << chart_index
                        << ": min_uv=(" << chart.min_uv[0] << ", " << chart.min_uv[1] << ")"
                        << " max_uv=(" << chart.max_uv[0] << ", " << chart.max_uv[1] << ")"
                        << " translation=(" << chart.translation[0] << ", " << chart.translation[1] << ")"
                        << " scale=" << chart.scale
                        << " area=" << chart.area
                        << " boundary_length=" << chart.boundary_length << "\n";
                }
            }
        }

        stream << '\n';
        if (options.manifest_output_path.has_value())
        {
            stream << "  Dataset manifest written to: "
                   << options.manifest_output_path->generic_string() << "\n";
        }
        stream << "  Dataset manifest snippet:\n";
        const std::string manifest = BuildDatasetManifestEntry(options, result);
        std::istringstream manifest_stream{manifest};
        std::string line;
        while (std::getline(manifest_stream, line))
        {
            stream << "    " << line << '\n';
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
        stream << "  --manifest-output <path>        Write dataset manifest YAML to <path>.\n";
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