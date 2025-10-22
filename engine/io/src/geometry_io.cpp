#include "engine/io/geometry_io.hpp"
#include "engine/io/geometry_io_registry.hpp"
#include "engine/io/telemetry.hpp"

#include "engine/math/vector.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::math::vec3;

        void log_geometry_io_failure(GeometryIoOperation operation,
                                     const std::filesystem::path& path,
                                     std::string_view format,
                                     const GeometryIoErrorCode& error)
        {
            const auto message = error.message();
            if (format.empty())
            {
                spdlog::warn("Geometry IO {} failed for '{}' (error={}, message={})",
                              to_string(operation),
                              path.string(),
                              error.identifier(),
                              message);
            }
            else
            {
                spdlog::warn("Geometry IO {} failed for '{}' (format={}, error={}, message={})",
                              to_string(operation),
                              path.string(),
                              format,
                              error.identifier(),
                              message);
            }
        }

        class GeometryIoException final : public std::runtime_error
        {
        public:
            GeometryIoException(GeometryIoError code, std::string_view message)
                : std::runtime_error(std::string{message})
                , code_{code}
            {
            }

            [[nodiscard]] GeometryIoError code() const noexcept
            {
                return code_;
            }

        private:
            GeometryIoError code_;
        };

        [[nodiscard]] GeometryIoError map_open_error(const std::filesystem::path& path) noexcept
        {
            std::error_code ec;
            const bool exists = std::filesystem::exists(path, ec);
            if (ec)
            {
                return GeometryIoError::io_failure;
            }

            return exists ? GeometryIoError::io_failure : GeometryIoError::file_not_found;
        }

        template <typename Callable>
        [[nodiscard]] GeometryIoResult<void> translate_io_exceptions(const std::filesystem::path& path,
                                                                     Callable&& callable)
        {
            try
            {
                std::forward<Callable>(callable)();
                return {};
            }
            catch (const GeometryIoException& e)
            {
                return make_geometry_io_error(e.code(), std::string{e.what()});
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                return make_geometry_io_error(GeometryIoError::io_failure,
                                              std::string{"Filesystem error while processing '"} + path.string()
                                                  + "': " + e.what());
            }
            catch (const std::bad_alloc&)
            {
                throw;
            }
            catch (const std::exception& e)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              std::string{"Failed to process '"} + path.string() + "': " + e.what());
            }
        }

        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        [[nodiscard]] std::string read_file_prefix(const std::filesystem::path& path, std::size_t max_bytes)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
            {
                return {};
            }

            std::string buffer(max_bytes, '\0');
            stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            buffer.resize(static_cast<std::size_t>(stream.gcount()));
            return buffer;
        }

        [[nodiscard]] std::string extension_of(const std::filesystem::path& path)
        {
            return to_lower(path.extension().string());
        }

        [[nodiscard]] std::string_view ltrim(std::string_view value);

        [[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix)
        {
            return value.substr(0, prefix.size()) == prefix;
        }

        constexpr std::size_t kSignatureScanBytes = 4096U;
        constexpr char kSignatureDatabaseEnvVar[] = "ENGINE_IO_GEOMETRY_SIGNATURE_PATH";

        enum class SignatureMatchType
        {
            byte_prefix,
            line_prefix,
            contains_all
        };

        struct SignatureMatch
        {
            SignatureMatchType type{SignatureMatchType::byte_prefix};
            std::vector<std::string> patterns{};
            std::vector<std::string> normalized_patterns{};
            std::size_t offset{0U};
            std::size_t max_scan_bytes{kSignatureScanBytes};
            bool case_sensitive{false};
            std::vector<std::string> comment_prefixes{};
        };

        struct GeometrySignatureRule
        {
            std::string id{};
            GeometryKind kind{GeometryKind::unknown};
            MeshFileFormat mesh_format{MeshFileFormat::unknown};
            PointCloudFileFormat point_cloud_format{PointCloudFileFormat::unknown};
            GraphFileFormat graph_format{GraphFileFormat::unknown};
            std::string format_hint{};
            SignatureMatch match{};
        };

        [[nodiscard]] std::string lowercase_ascii(std::string_view value)
        {
            std::string result;
            result.reserve(value.size());
            for (unsigned char c : value)
            {
                result.push_back(static_cast<char>(std::tolower(c)));
            }
            return result;
        }

        [[nodiscard]] GeometryKind parse_geometry_kind(const std::string& value)
        {
            const auto lowered = lowercase_ascii(value);
            if (lowered == "mesh")
            {
                return GeometryKind::mesh;
            }
            if (lowered == "point_cloud" || lowered == "point-cloud")
            {
                return GeometryKind::point_cloud;
            }
            if (lowered == "graph")
            {
                return GeometryKind::graph;
            }
            throw std::runtime_error("Unknown geometry kind in signature rule: " + value);
        }

        [[nodiscard]] MeshFileFormat parse_mesh_format(const std::string& value)
        {
            const auto lowered = lowercase_ascii(value);
            if (lowered == "obj")
            {
                return MeshFileFormat::obj;
            }
            if (lowered == "ply")
            {
                return MeshFileFormat::ply;
            }
            if (lowered == "off")
            {
                return MeshFileFormat::off;
            }
            if (lowered == "stl")
            {
                return MeshFileFormat::stl;
            }
            throw std::runtime_error("Unknown mesh format in signature rule: " + value);
        }

        [[nodiscard]] PointCloudFileFormat parse_point_cloud_format(const std::string& value)
        {
            const auto lowered = lowercase_ascii(value);
            if (lowered == "ply")
            {
                return PointCloudFileFormat::ply;
            }
            if (lowered == "xyz")
            {
                return PointCloudFileFormat::xyz;
            }
            if (lowered == "pcd")
            {
                return PointCloudFileFormat::pcd;
            }
            throw std::runtime_error("Unknown point cloud format in signature rule: " + value);
        }

        [[nodiscard]] GraphFileFormat parse_graph_format(const std::string& value)
        {
            const auto lowered = lowercase_ascii(value);
            if (lowered == "edgelist" || lowered == "edge-list")
            {
                return GraphFileFormat::edgelist;
            }
            if (lowered == "ply")
            {
                return GraphFileFormat::ply;
            }
            throw std::runtime_error("Unknown graph format in signature rule: " + value);
        }

        [[nodiscard]] SignatureMatchType parse_match_type(const std::string& value)
        {
            const auto lowered = lowercase_ascii(value);
            if (lowered == "byte_prefix" || lowered == "byte-prefix")
            {
                return SignatureMatchType::byte_prefix;
            }
            if (lowered == "line_prefix" || lowered == "line-prefix")
            {
                return SignatureMatchType::line_prefix;
            }
            if (lowered == "contains_all" || lowered == "contains-all")
            {
                return SignatureMatchType::contains_all;
            }
            throw std::runtime_error("Unknown match type in signature rule: " + value);
        }

        void normalise_match_patterns(SignatureMatch& match)
        {
            match.normalized_patterns.clear();
            match.normalized_patterns.reserve(match.patterns.size());
            if (match.case_sensitive)
            {
                match.normalized_patterns = match.patterns;
            }
            else
            {
                for (const auto& pattern : match.patterns)
                {
                    match.normalized_patterns.push_back(lowercase_ascii(pattern));
                }
            }
        }

        class SignatureDatabaseParser
        {
        public:
            explicit SignatureDatabaseParser(std::string_view data)
                : m_data(data)
            {
            }

            [[nodiscard]] std::vector<GeometrySignatureRule> parse()
            {
                skip_whitespace();
                expect('{');

                std::vector<GeometrySignatureRule> rules;
                bool first = true;
                while (!consume('}'))
                {
                    if (!first)
                    {
                        expect(',');
                    }
                    first = false;

                    const std::string key = parse_string();
                    expect(':');

                    if (key == "rules")
                    {
                        auto parsed_rules = parse_rules_array();
                        rules.insert(rules.end(), parsed_rules.begin(), parsed_rules.end());
                    }
                    else
                    {
                        skip_value();
                    }
                }

                skip_whitespace();
                if (!eof())
                {
                    throw std::runtime_error("Unexpected trailing data in geometry signature database");
                }

                return rules;
            }

        private:
            [[nodiscard]] bool eof() const noexcept
            {
                return m_index >= m_data.size();
            }

            void skip_whitespace()
            {
                while (m_index < m_data.size())
                {
                    const unsigned char c = static_cast<unsigned char>(m_data[m_index]);
                    if (std::isspace(c) == 0)
                    {
                        break;
                    }
                    ++m_index;
                }
            }

            bool consume(char expected)
            {
                skip_whitespace();
                if (m_index < m_data.size() && m_data[m_index] == expected)
                {
                    ++m_index;
                    return true;
                }
                return false;
            }

            void expect(char expected)
            {
                if (!consume(expected))
                {
                    throw std::runtime_error(std::string{"Expected character '"} + expected + "' in signature database");
                }
            }

            [[nodiscard]] char peek() const
            {
                if (m_index >= m_data.size())
                {
                    return '\0';
                }
                return m_data[m_index];
            }

            [[nodiscard]] bool match_literal(std::string_view literal) const noexcept
            {
                if (m_data.size() - m_index < literal.size())
                {
                    return false;
                }
                return m_data.substr(m_index, literal.size()) == literal;
            }

            void skip_literal(std::string_view literal)
            {
                if (!match_literal(literal))
                {
                    throw std::runtime_error("Expected literal '" + std::string{literal} + "' in signature database");
                }
                m_index += literal.size();
            }

            [[nodiscard]] std::string parse_string()
            {
                skip_whitespace();
                if (m_index >= m_data.size() || m_data[m_index] != '"')
                {
                    throw std::runtime_error("Expected string in signature database");
                }
                ++m_index;

                std::string result;
                while (m_index < m_data.size())
                {
                    const char c = m_data[m_index++];
                    if (c == '"')
                    {
                        return result;
                    }
                    if (c != '\\')
                    {
                        result.push_back(c);
                        continue;
                    }

                    if (m_index >= m_data.size())
                    {
                        throw std::runtime_error("Invalid escape sequence in signature database string");
                    }

                    const char escape = m_data[m_index++];
                    switch (escape)
                    {
                    case '"':
                        result.push_back('"');
                        break;
                    case '\\':
                        result.push_back('\\');
                        break;
                    case '/':
                        result.push_back('/');
                        break;
                    case 'b':
                        result.push_back('\b');
                        break;
                    case 'f':
                        result.push_back('\f');
                        break;
                    case 'n':
                        result.push_back('\n');
                        break;
                    case 'r':
                        result.push_back('\r');
                        break;
                    case 't':
                        result.push_back('\t');
                        break;
                    case 'u':
                    {
                        if (m_index + 4 > m_data.size())
                        {
                            throw std::runtime_error("Incomplete unicode escape in signature database string");
                        }
                        unsigned int code_point = 0U;
                        for (int i = 0; i < 4; ++i)
                        {
                            const char hex = m_data[m_index++];
                            code_point <<= 4U;
                            if (hex >= '0' && hex <= '9')
                            {
                                code_point += static_cast<unsigned int>(hex - '0');
                            }
                            else if (hex >= 'a' && hex <= 'f')
                            {
                                code_point += static_cast<unsigned int>(10 + hex - 'a');
                            }
                            else if (hex >= 'A' && hex <= 'F')
                            {
                                code_point += static_cast<unsigned int>(10 + hex - 'A');
                            }
                            else
                            {
                                throw std::runtime_error("Invalid unicode escape in signature database string");
                            }
                        }

                        if (code_point > 0x7FU)
                        {
                            throw std::runtime_error("Non-ASCII unicode escapes are not supported in signature database strings");
                        }
                        result.push_back(static_cast<char>(code_point));
                        break;
                    }
                    default:
                        throw std::runtime_error("Unsupported escape sequence in signature database string");
                    }
                }

                throw std::runtime_error("Unterminated string in signature database");
            }

            void skip_number()
            {
                skip_whitespace();
                if (m_index < m_data.size() && (m_data[m_index] == '-' || m_data[m_index] == '+'))
                {
                    ++m_index;
                }
                while (m_index < m_data.size() && std::isdigit(static_cast<unsigned char>(m_data[m_index])) != 0)
                {
                    ++m_index;
                }
                if (m_index < m_data.size() && m_data[m_index] == '.')
                {
                    ++m_index;
                    while (m_index < m_data.size() && std::isdigit(static_cast<unsigned char>(m_data[m_index])) != 0)
                    {
                        ++m_index;
                    }
                }
                if (m_index < m_data.size() && (m_data[m_index] == 'e' || m_data[m_index] == 'E'))
                {
                    ++m_index;
                    if (m_index < m_data.size() && (m_data[m_index] == '+' || m_data[m_index] == '-'))
                    {
                        ++m_index;
                    }
                    while (m_index < m_data.size() && std::isdigit(static_cast<unsigned char>(m_data[m_index])) != 0)
                    {
                        ++m_index;
                    }
                }
            }

            std::size_t parse_unsigned()
            {
                skip_whitespace();
                std::size_t value = 0U;
                bool has_digit = false;
                while (m_index < m_data.size())
                {
                    const char c = m_data[m_index];
                    if (std::isdigit(static_cast<unsigned char>(c)) == 0)
                    {
                        break;
                    }
                    has_digit = true;
                    value = (value * 10U) + static_cast<std::size_t>(c - '0');
                    ++m_index;
                }
                if (!has_digit)
                {
                    throw std::runtime_error("Expected unsigned integer in signature database");
                }
                return value;
            }

            bool parse_bool()
            {
                skip_whitespace();
                if (match_literal("true"))
                {
                    m_index += 4U;
                    return true;
                }
                if (match_literal("false"))
                {
                    m_index += 5U;
                    return false;
                }
                throw std::runtime_error("Expected boolean value in signature database");
            }

            std::vector<std::string> parse_string_array()
            {
                expect('[');
                std::vector<std::string> values;
                bool first = true;
                while (!consume(']'))
                {
                    if (!first)
                    {
                        expect(',');
                    }
                    first = false;
                    values.push_back(parse_string());
                }
                return values;
            }

            void skip_value()
            {
                skip_whitespace();
                const char current = peek();
                if (current == '{')
                {
                    expect('{');
                    bool first = true;
                    while (!consume('}'))
                    {
                        if (!first)
                        {
                            expect(',');
                        }
                        first = false;
                        static_cast<void>(parse_string());
                        expect(':');
                        skip_value();
                    }
                    return;
                }

                if (current == '[')
                {
                    expect('[');
                    bool first = true;
                    while (!consume(']'))
                    {
                        if (!first)
                        {
                            expect(',');
                        }
                        first = false;
                        skip_value();
                    }
                    return;
                }

                if (current == '"')
                {
                    static_cast<void>(parse_string());
                    return;
                }

                if (std::isdigit(static_cast<unsigned char>(current)) != 0 || current == '-' || current == '+')
                {
                    skip_number();
                    return;
                }

                if (match_literal("true"))
                {
                    skip_literal("true");
                    return;
                }
                if (match_literal("false"))
                {
                    skip_literal("false");
                    return;
                }
                if (match_literal("null"))
                {
                    skip_literal("null");
                    return;
                }

                throw std::runtime_error("Unexpected token while parsing signature database");
            }

            std::vector<GeometrySignatureRule> parse_rules_array()
            {
                std::vector<GeometrySignatureRule> rules;
                expect('[');
                bool first = true;
                while (!consume(']'))
                {
                    if (!first)
                    {
                        expect(',');
                    }
                    first = false;
                    rules.push_back(parse_rule_object());
                }
                return rules;
            }

            GeometrySignatureRule parse_rule_object()
            {
                GeometrySignatureRule rule{};
                expect('{');

                bool has_kind = false;
                bool has_match = false;
                bool first = true;
                while (!consume('}'))
                {
                    if (!first)
                    {
                        expect(',');
                    }
                    first = false;

                    const std::string key = parse_string();
                    expect(':');

                    if (key == "id")
                    {
                        rule.id = parse_string();
                    }
                    else if (key == "kind")
                    {
                        rule.kind = parse_geometry_kind(parse_string());
                        has_kind = true;
                    }
                    else if (key == "mesh_format")
                    {
                        rule.mesh_format = parse_mesh_format(parse_string());
                    }
                    else if (key == "point_cloud_format")
                    {
                        rule.point_cloud_format = parse_point_cloud_format(parse_string());
                    }
                    else if (key == "graph_format")
                    {
                        rule.graph_format = parse_graph_format(parse_string());
                    }
                    else if (key == "format_hint")
                    {
                        rule.format_hint = parse_string();
                    }
                    else if (key == "match")
                    {
                        rule.match = parse_match_object();
                        has_match = true;
                    }
                    else
                    {
                        skip_value();
                    }
                }

                if (!has_kind)
                {
                    throw std::runtime_error("Signature rule missing 'kind'");
                }
                if (!has_match)
                {
                    throw std::runtime_error("Signature rule missing 'match'");
                }

                switch (rule.kind)
                {
                case GeometryKind::mesh:
                    if (rule.mesh_format == MeshFileFormat::unknown)
                    {
                        throw std::runtime_error("Mesh signature rule missing 'mesh_format'");
                    }
                    break;
                case GeometryKind::point_cloud:
                    if (rule.point_cloud_format == PointCloudFileFormat::unknown)
                    {
                        throw std::runtime_error("Point cloud signature rule missing 'point_cloud_format'");
                    }
                    break;
                case GeometryKind::graph:
                    if (rule.graph_format == GraphFileFormat::unknown)
                    {
                        throw std::runtime_error("Graph signature rule missing 'graph_format'");
                    }
                    break;
                default:
                    break;
                }

                if (rule.match.patterns.empty())
                {
                    throw std::runtime_error("Signature rule must provide at least one pattern");
                }

                normalise_match_patterns(rule.match);
                return rule;
            }

            SignatureMatch parse_match_object()
            {
                SignatureMatch match{};
                expect('{');

                bool has_type = false;
                bool first = true;
                while (!consume('}'))
                {
                    if (!first)
                    {
                        expect(',');
                    }
                    first = false;

                    const std::string key = parse_string();
                    expect(':');

                    if (key == "type")
                    {
                        match.type = parse_match_type(parse_string());
                        has_type = true;
                    }
                    else if (key == "pattern")
                    {
                        match.patterns = {parse_string()};
                    }
                    else if (key == "patterns")
                    {
                        match.patterns = parse_string_array();
                    }
                    else if (key == "offset")
                    {
                        match.offset = parse_unsigned();
                    }
                    else if (key == "max_scan_bytes")
                    {
                        match.max_scan_bytes = parse_unsigned();
                    }
                    else if (key == "case_sensitive")
                    {
                        match.case_sensitive = parse_bool();
                    }
                    else if (key == "comment_prefixes")
                    {
                        match.comment_prefixes = parse_string_array();
                    }
                    else
                    {
                        skip_value();
                    }
                }

                if (!has_type)
                {
                    throw std::runtime_error("Signature match is missing 'type'");
                }
                if (match.patterns.empty())
                {
                    throw std::runtime_error("Signature match must specify at least one pattern");
                }

                if (match.max_scan_bytes == 0U)
                {
                    match.max_scan_bytes = kSignatureScanBytes;
                }

                normalise_match_patterns(match);
                return match;
            }

            std::string_view m_data;
            std::size_t m_index{0U};
        };

        class SignatureDatabaseCache
        {
        public:
            [[nodiscard]] const std::vector<GeometrySignatureRule>& rules()
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                if (!m_loaded)
                {
                    m_rules = load();
                    m_loaded = true;
                }
                return m_rules;
            }

#if defined(ENGINE_IO_ENABLE_SIGNATURE_TEST_HOOKS)
            void reset_for_testing()
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                m_rules.clear();
                m_loaded = false;
            }
#endif

        private:
            [[nodiscard]] std::filesystem::path resolve_path() const
            {
                if (const char* override_path = std::getenv(kSignatureDatabaseEnvVar); override_path != nullptr && override_path[0] != '\0')
                {
                    return std::filesystem::path{override_path};
                }
#ifdef ENGINE_IO_GEOMETRY_SIGNATURE_DB_PATH
                return std::filesystem::path{ENGINE_IO_GEOMETRY_SIGNATURE_DB_PATH};
#else
                return {};
#endif
            }

            [[nodiscard]] std::vector<GeometrySignatureRule> load()
            {
                const auto path = resolve_path();
                if (path.empty())
                {
                    spdlog::debug("Geometry signature database path not set; skipping data-driven detection");
                    return {};
                }

                std::ifstream stream{path, std::ios::binary};
                if (!stream)
                {
                    spdlog::warn("Failed to open geometry signature database '{}'", path.string());
                    return {};
                }

                const std::string content{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};

                try
                {
                    SignatureDatabaseParser parser(content);
                    auto parsed_rules = parser.parse();
                    if (parsed_rules.empty())
                    {
                        spdlog::warn("Geometry signature database '{}' contained no rules", path.string());
                    }
                    else
                    {
                        spdlog::debug("Loaded {} geometry signature rules from '{}'", parsed_rules.size(), path.string());
                    }
                    return parsed_rules;
                }
                catch (const std::exception& e)
                {
                    spdlog::warn("Failed to parse geometry signature database '{}': {}", path.string(), e.what());
                    return {};
                }
            }

            std::mutex m_mutex;
            bool m_loaded{false};
            std::vector<GeometrySignatureRule> m_rules{};
        };

        SignatureDatabaseCache& geometry_signature_database()
        {
            static SignatureDatabaseCache cache;
            return cache;
        }

        [[nodiscard]] const std::vector<GeometrySignatureRule>& geometry_signature_rules()
        {
            return geometry_signature_database().rules();
        }

        [[nodiscard]] std::string_view first_non_comment_line(std::string_view data,
                                                              const std::vector<std::string>& comment_prefixes)
        {
            std::size_t offset = 0U;
            while (offset < data.size())
            {
                const auto newline_pos = data.find('\n', offset);
                const std::size_t line_end = (newline_pos == std::string_view::npos) ? data.size() : newline_pos;
                std::string_view line = data.substr(offset, line_end - offset);
                if (!line.empty() && line.back() == '\r')
                {
                    line.remove_suffix(1U);
                }

                auto trimmed = ltrim(line);
                if (!trimmed.empty())
                {
                    bool is_comment = false;
                    for (const auto& prefix : comment_prefixes)
                    {
                        if (!prefix.empty() && starts_with(trimmed, prefix))
                        {
                            is_comment = true;
                            break;
                        }
                    }

                    if (!is_comment)
                    {
                        return trimmed;
                    }
                }

                if (newline_pos == std::string_view::npos)
                {
                    break;
                }
                offset = newline_pos + 1U;
            }

            return {};
        }

        [[nodiscard]] bool match_byte_prefix(const SignatureMatch& match, std::string_view data)
        {
            if (match.patterns.empty())
            {
                return false;
            }

            const auto& pattern = match.patterns.front();
            if (match.offset + pattern.size() > data.size())
            {
                return false;
            }

            const auto slice = data.substr(match.offset, pattern.size());
            if (match.case_sensitive)
            {
                return std::equal(pattern.begin(), pattern.end(), slice.begin(), slice.end());
            }

            const auto lowered = lowercase_ascii(slice);
            return lowered == match.normalized_patterns.front();
        }

        [[nodiscard]] bool match_contains_all(const SignatureMatch& match, std::string_view data)
        {
            if (match.patterns.empty())
            {
                return false;
            }

            const std::size_t scan = std::min(match.max_scan_bytes, data.size());
            const std::string_view window = data.substr(0U, scan);

            if (match.case_sensitive)
            {
                for (const auto& pattern : match.patterns)
                {
                    if (pattern.empty())
                    {
                        continue;
                    }
                    if (window.find(pattern) == std::string_view::npos)
                    {
                        return false;
                    }
                }
                return true;
            }

            const auto lowered_window = lowercase_ascii(window);
            for (const auto& pattern : match.normalized_patterns)
            {
                if (pattern.empty())
                {
                    continue;
                }
                if (lowered_window.find(pattern) == std::string::npos)
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool match_line_prefix(const SignatureMatch& match, std::string_view data)
        {
            const std::size_t scan = std::min(match.max_scan_bytes, data.size());
            const std::string_view window = data.substr(0U, scan);
            const auto line = first_non_comment_line(window, match.comment_prefixes);
            if (line.empty())
            {
                return false;
            }

            if (match.case_sensitive)
            {
                for (const auto& pattern : match.patterns)
                {
                    if (!pattern.empty() && line.substr(0U, pattern.size()) == pattern)
                    {
                        return true;
                    }
                }
                return false;
            }

            const auto lowered_line = lowercase_ascii(line);
            for (const auto& pattern : match.normalized_patterns)
            {
                if (!pattern.empty() && lowered_line.rfind(pattern, 0U) == 0U)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] bool matches_signature_rule(const GeometrySignatureRule& rule, std::string_view data)
        {
            switch (rule.match.type)
            {
            case SignatureMatchType::byte_prefix:
                return match_byte_prefix(rule.match, data);
            case SignatureMatchType::line_prefix:
                return match_line_prefix(rule.match, data);
            case SignatureMatchType::contains_all:
                return match_contains_all(rule.match, data);
            default:
                return false;
            }
        }

        [[nodiscard]] GeometryDetectionResult detect_geometry_from_signature_database(std::string_view data)
        {
            GeometryDetectionResult detection{};
            if (data.empty())
            {
                return detection;
            }

            const auto& rules = geometry_signature_rules();
            for (const auto& rule : rules)
            {
                if (!matches_signature_rule(rule, data))
                {
                    continue;
                }

                detection.kind = rule.kind;
                detection.mesh_format = rule.mesh_format;
                detection.point_cloud_format = rule.point_cloud_format;
                detection.graph_format = rule.graph_format;
                detection.format_hint = rule.format_hint;
                return detection;
            }

            return detection;
        }

#if defined(ENGINE_IO_ENABLE_SIGNATURE_TEST_HOOKS)
        void reset_geometry_signature_cache_for_testing_impl()
        {
            geometry_signature_database().reset_for_testing();
        }
#endif

        [[nodiscard]] std::string_view to_string(GeometryKind kind) noexcept
        {
            switch (kind)
            {
            case GeometryKind::mesh:
                return "mesh";
            case GeometryKind::point_cloud:
                return "point_cloud";
            case GeometryKind::graph:
                return "graph";
            default:
                return "unknown";
            }
        }

        [[nodiscard]] std::string_view to_string(MeshFileFormat format) noexcept
        {
            switch (format)
            {
            case MeshFileFormat::obj:
                return "obj";
            case MeshFileFormat::ply:
                return "ply";
            case MeshFileFormat::off:
                return "off";
            case MeshFileFormat::stl:
                return "stl";
            default:
                return "unknown";
            }
        }

        [[nodiscard]] std::string_view to_string(PointCloudFileFormat format) noexcept
        {
            switch (format)
            {
            case PointCloudFileFormat::ply:
                return "ply";
            case PointCloudFileFormat::xyz:
                return "xyz";
            case PointCloudFileFormat::pcd:
                return "pcd";
            default:
                return "unknown";
            }
        }

        [[nodiscard]] std::string_view to_string(GraphFileFormat format) noexcept
        {
            switch (format)
            {
            case GraphFileFormat::edgelist:
                return "edgelist";
            case GraphFileFormat::ply:
                return "ply";
            default:
                return "unknown";
            }
        }

        [[nodiscard]] bool is_integer_token(std::string_view token) noexcept
        {
            if (token.empty())
            {
                return false;
            }

            std::size_t index = 0U;
            if (token[0] == '+' || token[0] == '-')
            {
                index = 1U;
            }

            bool has_digit = false;
            for (; index < token.size(); ++index)
            {
                const unsigned char c = static_cast<unsigned char>(token[index]);
                if (!std::isdigit(c))
                {
                    return false;
                }
                has_digit = true;
            }

            return has_digit;
        }

        [[nodiscard]] bool looks_like_obj_signature(std::string_view text)
        {
            std::istringstream stream{std::string{text}};
            std::string line;
            int geometry_tokens = 0;
            int face_tokens = 0;
            int lines_inspected = 0;

            while (std::getline(stream, line) && lines_inspected < 64)
            {
                ++lines_inspected;
                auto trimmed_view = ltrim(line);
                if (trimmed_view.empty())
                {
                    continue;
                }

                if (trimmed_view.front() == '#')
                {
                    continue;
                }

                const auto lowered = to_lower(std::string{trimmed_view});
                bool matched = false;
                if (starts_with(lowered, "v ") || starts_with(lowered, "vn ") || starts_with(lowered, "vt "))
                {
                    ++geometry_tokens;
                    matched = true;
                }
                else if (starts_with(lowered, "f "))
                {
                    ++face_tokens;
                    matched = true;
                }
                else if (starts_with(lowered, "o ") || starts_with(lowered, "g ") || starts_with(lowered, "usemtl ")
                         || starts_with(lowered, "mtllib "))
                {
                    ++geometry_tokens;
                    matched = true;
                }

                if ((geometry_tokens >= 2 && face_tokens > 0) || (geometry_tokens + face_tokens) >= 3)
                {
                    return true;
                }

                if (!matched && (geometry_tokens + face_tokens) == 0 && lines_inspected > 16)
                {
                    break;
                }
            }

            return geometry_tokens > 0 && face_tokens > 0;
        }

        [[nodiscard]] bool looks_like_off_signature(std::string_view text)
        {
            std::istringstream stream{std::string{text}};
            std::string line;

            for (int i = 0; i < 8 && std::getline(stream, line); ++i)
            {
                auto trimmed_view = ltrim(line);
                if (trimmed_view.empty())
                {
                    continue;
                }

                if (trimmed_view.front() == '#')
                {
                    continue;
                }

                const auto lowered = to_lower(std::string{trimmed_view});
                if (lowered == "off" || lowered == "coff" || lowered == "noff" || lowered == "cnoff")
                {
                    return true;
                }

                break;
            }

            return false;
        }

        [[nodiscard]] bool looks_like_pcd_signature(std::string_view text)
        {
            std::istringstream stream{std::string{text}};
            std::string line;
            int header_hits = 0;
            bool comment_hint = false;
            int lines_inspected = 0;

            while (std::getline(stream, line) && lines_inspected < 128)
            {
                ++lines_inspected;
                auto trimmed_view = ltrim(line);
                if (trimmed_view.empty())
                {
                    continue;
                }

                const auto lowered = to_lower(std::string{trimmed_view});
                if (trimmed_view.front() == '#')
                {
                    if (lowered.find(".pcd") != std::string::npos)
                    {
                        comment_hint = true;
                    }
                    continue;
                }

                if (starts_with(lowered, "version ") || starts_with(lowered, "fields ") || starts_with(lowered, "size ")
                    || starts_with(lowered, "type ") || starts_with(lowered, "count ") || starts_with(lowered, "width ")
                    || starts_with(lowered, "height ") || starts_with(lowered, "points ") || starts_with(lowered, "data "))
                {
                    ++header_hits;
                    if (starts_with(lowered, "data "))
                    {
                        break;
                    }
                }
            }

            return header_hits >= 3 || (comment_hint && header_hits >= 1);
        }

        [[nodiscard]] bool looks_like_edgelist_signature(std::string_view text)
        {
            std::istringstream stream{std::string{text}};
            std::string line;
            int matches = 0;
            int lines_inspected = 0;

            while (std::getline(stream, line) && lines_inspected < 128)
            {
                ++lines_inspected;
                auto trimmed_view = ltrim(line);
                if (trimmed_view.empty())
                {
                    continue;
                }

                if (trimmed_view.front() == '#')
                {
                    continue;
                }

                std::istringstream token_stream{std::string{trimmed_view}};
                std::string first;
                std::string second;
                if (!(token_stream >> first >> second))
                {
                    continue;
                }

                if (!is_integer_token(first) || !is_integer_token(second))
                {
                    return false;
                }

                ++matches;
                if (matches >= 2)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] GeometryDetectionResult detect_geometry_from_builtin_signatures(std::string_view prefix)
        {
            GeometryDetectionResult detection{};
            if (prefix.empty())
            {
                return detection;
            }

            if (looks_like_off_signature(prefix))
            {
                detection.kind = GeometryKind::mesh;
                detection.mesh_format = MeshFileFormat::off;
                return detection;
            }

            if (looks_like_obj_signature(prefix))
            {
                detection.kind = GeometryKind::mesh;
                detection.mesh_format = MeshFileFormat::obj;
                return detection;
            }

            if (looks_like_pcd_signature(prefix))
            {
                detection.kind = GeometryKind::point_cloud;
                detection.point_cloud_format = PointCloudFileFormat::pcd;
                return detection;
            }

            if (looks_like_edgelist_signature(prefix))
            {
                detection.kind = GeometryKind::graph;
                detection.graph_format = GraphFileFormat::edgelist;
                return detection;
            }

            return detection;
        }

        [[nodiscard]] GeometryDetectionResult detect_geometry_from_signatures(std::string_view data)
        {
            if (auto database_detection = detect_geometry_from_signature_database(data); database_detection.kind != GeometryKind::unknown)
            {
                return database_detection;
            }

            return detect_geometry_from_builtin_signatures(data);
        }

        [[nodiscard]] GeometryDetectionResult detect_geometry_from_signatures(const std::string& data)
        {
            return detect_geometry_from_signatures(std::string_view{data});
        }

        [[nodiscard]] GeometryDetectionResult detect_geometry_from_signatures(const std::filesystem::path& path)
        {
            const auto prefix = read_file_prefix(path, kSignatureScanBytes);
            return detect_geometry_from_signatures(prefix);
        }

        struct PlyHeaderInfo
        {
            std::size_t vertex_count{0};
            std::size_t face_count{0};
            std::size_t edge_count{0};
            bool ascii{true};
        };

        [[nodiscard]] std::string_view ltrim(std::string_view value)
        {
            const auto it = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
                return std::isspace(c) != 0;
            });
            if (it == value.end())
            {
                return std::string_view{};
            }
            const auto offset = static_cast<std::size_t>(std::distance(value.begin(), it));
            return value.substr(offset);
        }

        [[nodiscard]] GeometryIoResult<PlyHeaderInfo> inspect_ply_header(const std::filesystem::path& path)
        {
            PlyHeaderInfo info{};
            if (!std::filesystem::exists(path))
            {
                return make_geometry_io_error(
                    GeometryIoError::file_not_found,
                    "Cannot inspect PLY header of missing file: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                return make_geometry_io_error(
                    GeometryIoError::io_failure,
                    "Failed to open PLY file for inspection: " + path.string());
            }

            std::string line;
            if (!std::getline(stream, line) || to_lower(line) != "ply")
            {
                return make_geometry_io_error(
                    GeometryIoError::invalid_argument,
                    "Invalid PLY header in file: " + path.string());
            }

            enum class Section
            {
                none,
                vertex,
                face,
                edge
            };
            Section current_section{Section::none};

            while (std::getline(stream, line))
            {
                if (line == "end_header")
                {
                    break;
                }

                std::istringstream iss{line};
                std::string token;
                iss >> token;
                token = to_lower(token);
                if (token == "comment" || token == "obj_info")
                {
                    continue;
                }
                if (token == "format")
                {
                    std::string fmt;
                    iss >> fmt;
                    info.ascii = (to_lower(fmt) == "ascii");
                    continue;
                }
                if (token == "element")
                {
                    std::string name;
                    std::size_t count{0};
                    iss >> name >> count;
                    name = to_lower(name);
                    if (name == "vertex")
                    {
                        current_section = Section::vertex;
                        info.vertex_count = count;
                    }
                    else if (name == "face")
                    {
                        current_section = Section::face;
                        info.face_count = count;
                    }
                    else if (name == "edge")
                    {
                        current_section = Section::edge;
                        info.edge_count = count;
                    }
                    else
                    {
                        current_section = Section::none;
                    }
                    continue;
                }
                if (token == "property")
                {
                    continue;
                }
            }

            return info;
        }

        [[nodiscard]] bool looks_like_binary_stl(std::istream& stream, std::uintmax_t file_size)
        {
            if (file_size < 84U)
            {
                return false;
            }

            std::array<char, 80U> header{};
            stream.read(header.data(), static_cast<std::streamsize>(header.size()));
            if (stream.gcount() != static_cast<std::streamsize>(header.size()))
            {
                return false;
            }

            std::uint32_t triangle_count = 0U;
            stream.read(reinterpret_cast<char*>(&triangle_count), sizeof(triangle_count));
            if (stream.gcount() != static_cast<std::streamsize>(sizeof(triangle_count)))
            {
                return false;
            }

            const std::uintmax_t expected_size = 84U + static_cast<std::uintmax_t>(triangle_count) * 50U;
            if (expected_size == file_size)
            {
                return true;
            }

            if (expected_size < file_size)
            {
                const auto remainder = file_size - expected_size;
                return remainder <= 512U;
            }

            return false;
        }

        [[nodiscard]] bool looks_like_ascii_stl(std::istream& stream)
        {
            std::string line;
            if (!std::getline(stream, line))
            {
                return false;
            }

            const auto trimmed = ltrim(line);
            if (trimmed.size() < 5U)
            {
                return false;
            }

            if (!starts_with(to_lower(std::string{trimmed.substr(0U, 5U)}), "solid"))
            {
                return false;
            }

            for (int i = 0; i < 64 && std::getline(stream, line); ++i)
            {
                const auto lower = to_lower(line);
                if (lower.find("facet normal") != std::string::npos || lower.find("endsolid") != std::string::npos)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] GeometryDetectionResult detect_stl_from_signature(const std::filesystem::path& path)
        {
            GeometryDetectionResult detection{};

            const auto file_size = std::filesystem::file_size(path);

            {
                std::ifstream binary_stream{path, std::ios::binary};
                if (binary_stream && looks_like_binary_stl(binary_stream, file_size))
                {
                    detection.kind = GeometryKind::mesh;
                    detection.mesh_format = MeshFileFormat::stl;
                    return detection;
                }
            }

            std::ifstream ascii_stream{path};
            if (ascii_stream && looks_like_ascii_stl(ascii_stream))
            {
                detection.kind = GeometryKind::mesh;
                detection.mesh_format = MeshFileFormat::stl;
            }

            return detection;
        }

        [[nodiscard]] MeshFileFormat mesh_format_from_extension(const std::string& ext)
        {
            if (ext == ".obj")
            {
                return MeshFileFormat::obj;
            }
            if (ext == ".off")
            {
                return MeshFileFormat::off;
            }
            if (ext == ".stl")
            {
                return MeshFileFormat::stl;
            }
            if (ext == ".ply")
            {
                return MeshFileFormat::ply;
            }
            return MeshFileFormat::unknown;
        }

        [[nodiscard]] PointCloudFileFormat point_cloud_format_from_extension(const std::string& ext)
        {
            if (ext == ".xyz")
            {
                return PointCloudFileFormat::xyz;
            }
            if (ext == ".pcd")
            {
                return PointCloudFileFormat::pcd;
            }
            if (ext == ".ply")
            {
                return PointCloudFileFormat::ply;
            }
            return PointCloudFileFormat::unknown;
        }

        [[nodiscard]] GraphFileFormat graph_format_from_extension(const std::string& ext)
        {
            if (ext == ".edgelist" || ext == ".elist" || ext == ".edges")
            {
                return GraphFileFormat::edgelist;
            }
            if (ext == ".ply")
            {
                return GraphFileFormat::ply;
            }
            return GraphFileFormat::unknown;
        }

        [[nodiscard]] GeometryDetectionResult classify_extension_only(const std::string& ext)
        {
            GeometryDetectionResult result{};
            result.format_hint = ext;

            if (auto mesh_format = mesh_format_from_extension(ext); mesh_format != MeshFileFormat::unknown && ext != ".ply")
            {
                result.kind = GeometryKind::mesh;
                result.mesh_format = mesh_format;
                return result;
            }

            if (auto pc_format = point_cloud_format_from_extension(ext); pc_format != PointCloudFileFormat::unknown && ext != ".ply")
            {
                result.kind = GeometryKind::point_cloud;
                result.point_cloud_format = pc_format;
                return result;
            }

            if (auto graph_format = graph_format_from_extension(ext); graph_format != GraphFileFormat::unknown && ext != ".ply")
            {
                result.kind = GeometryKind::graph;
                result.graph_format = graph_format;
                return result;
            }

            if (ext == ".ply")
            {
                result.mesh_format = MeshFileFormat::ply;
                result.point_cloud_format = PointCloudFileFormat::ply;
                result.graph_format = GraphFileFormat::ply;
            }

            return result;
        }

        [[nodiscard]] std::vector<std::string> tokenize(const std::string& line)
        {
            std::istringstream iss{line};
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token)
            {
                tokens.push_back(token);
            }
            return tokens;
        }

        void ensure_parent_directory(const std::filesystem::path& path)
        {
            if (auto parent = path.parent_path(); !parent.empty())
            {
                std::filesystem::create_directories(parent);
            }
        }

        void read_mesh_obj(const std::filesystem::path& path, geometry::MeshInterface& mesh)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open OBJ file: " + path.string());
            }

            mesh.clear();

            std::vector<geometry::VertexHandle> vertex_handles;
            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }

                const auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }

                if (tokens[0] == "v")
                {
                    if (tokens.size() < 4)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "OBJ vertex without 3 coordinates in file: " + path.string());
                    }
                    const float x = std::stof(tokens[1]);
                    const float y = std::stof(tokens[2]);
                    const float z = std::stof(tokens[3]);
                    vertex_handles.push_back(mesh.add_vertex(vec3{x, y, z}));
                }
                else if (tokens[0] == "f")
                {
                    if (tokens.size() < 4)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "OBJ face with fewer than 3 vertices in file: " + path.string());
                    }
                    std::vector<geometry::VertexHandle> face_vertices;
                    face_vertices.reserve(tokens.size() - 1U);
                    for (std::size_t i = 1; i < tokens.size(); ++i)
                    {
                        const auto& token = tokens[i];
                        const auto slash = token.find('/');
                        const std::string index_str = (slash == std::string::npos) ? token : token.substr(0, slash);
                        const int index = std::stoi(index_str);
                        const int positive_index = index > 0 ? index : static_cast<int>(vertex_handles.size()) + index + 1;
                        if (positive_index <= 0 || static_cast<std::size_t>(positive_index) > vertex_handles.size())
                        {
                            throw GeometryIoException(GeometryIoError::invalid_argument,
                                                      "OBJ face references invalid vertex index in file: " + path.string());
                        }
                        face_vertices.push_back(vertex_handles[static_cast<std::size_t>(positive_index - 1)]);
                    }

                    if (!mesh.add_face(face_vertices))
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Failed to add face while parsing OBJ file: " + path.string());
                    }
                }
            }
        }

        void write_mesh_obj(const std::filesystem::path& path, const geometry::MeshInterface& mesh)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open OBJ file for writing: " + path.string());
            }

            const std::size_t invalid = std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> vertex_indices(mesh.vertices_size(), invalid);
            std::size_t index{1};
            for (const auto v : mesh.vertices())
            {
                const auto& position = mesh.position(v);
                stream << "v " << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
                vertex_indices[v.index()] = index++;
            }

            for (const auto f : mesh.faces())
            {
                const auto h_start = mesh.halfedge(f);
                if (!h_start.is_valid())
                {
                    continue;
                }

                stream << 'f';
                auto h = h_start;
                do
                {
                    const auto v = mesh.to_vertex(h);
                    const auto idx = vertex_indices[v.index()];
                    if (idx == invalid)
                    {
                        throw GeometryIoException(
                            GeometryIoError::invalid_argument,
                            "Mesh contains face with unregistered vertex while writing OBJ");
                    }
                    stream << ' ' << idx;
                    h = mesh.next_halfedge(h);
                } while (h != h_start);
                stream << '\n';
            }
        }

        void read_mesh_off(const std::filesystem::path& path, geometry::MeshInterface& mesh)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open OFF file: " + path.string());
            }

            std::string header;
            stream >> header;
            if (to_lower(header) != "off")
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "Invalid OFF header in file: " + path.string());
            }

            std::size_t vertex_count{0};
            std::size_t face_count{0};
            std::size_t edge_count{0};
            stream >> vertex_count >> face_count >> edge_count;

            mesh.clear();
            mesh.reserve(vertex_count, face_count * 2, face_count);

            std::vector<geometry::VertexHandle> vertices;
            vertices.reserve(vertex_count);

            for (std::size_t i = 0; i < vertex_count; ++i)
            {
                float x{}, y{}, z{};
                stream >> x >> y >> z;
                vertices.push_back(mesh.add_vertex(vec3{x, y, z}));
            }

            for (std::size_t i = 0; i < face_count; ++i)
            {
                std::size_t n{};
                stream >> n;
                if (n < 3)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "OFF face has fewer than 3 vertices in file: " + path.string());
                }
                std::vector<geometry::VertexHandle> face_vertices;
                face_vertices.reserve(n);
                for (std::size_t j = 0; j < n; ++j)
                {
                    std::size_t idx{};
                    stream >> idx;
                    if (idx >= vertices.size())
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "OFF face references invalid vertex index in file: " + path.string());
                    }
                    face_vertices.push_back(vertices[idx]);
                }
                if (!mesh.add_face(face_vertices))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Failed to add face while parsing OFF file: " + path.string());
                }
            }
        }

        void write_mesh_off(const std::filesystem::path& path, const geometry::MeshInterface& mesh)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open OFF file for writing: " + path.string());
            }

            const std::size_t vertex_count = mesh.vertex_count();
            const std::size_t face_count = mesh.face_count();

            stream << "OFF\n";
            stream << vertex_count << ' ' << face_count << ' ' << mesh.edge_count() << '\n';

            const std::size_t invalid = std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> vertex_indices(mesh.vertices_size(), invalid);
            std::size_t index{0};
            for (const auto v : mesh.vertices())
            {
                const auto& position = mesh.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
                vertex_indices[v.index()] = index++;
            }

            for (const auto f : mesh.faces())
            {
                std::vector<std::size_t> indices;
                const auto h_start = mesh.halfedge(f);
                if (!h_start.is_valid())
                {
                    continue;
                }
                auto h = h_start;
                do
                {
                    const auto v = mesh.to_vertex(h);
                    const auto idx = vertex_indices[v.index()];
                    if (idx == invalid)
                    {
                        throw GeometryIoException(
                            GeometryIoError::invalid_argument,
                            "Mesh contains face with unregistered vertex while writing OFF");
                    }
                    indices.push_back(idx);
                    h = mesh.next_halfedge(h);
                } while (h != h_start);

                stream << indices.size();
                for (const auto idx : indices)
                {
                    stream << ' ' << idx;
                }
                stream << '\n';
            }
        }

        void read_mesh_ply(const std::filesystem::path& path, geometry::MeshInterface& mesh)
        {
            const auto header_result = inspect_ply_header(path);
            if (!header_result)
            {
                throw GeometryIoException(header_result.error().code(), header_result.error().message());
            }
            const auto header = header_result.value();
            if (!header.ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PLY meshes are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PLY file: " + path.string());
            }

            std::string line;
            std::getline(stream, line); // ply
            while (std::getline(stream, line) && line != "end_header")
            {
            }

            mesh.clear();
            mesh.reserve(header.vertex_count, header.face_count * 2, header.face_count);

            std::vector<geometry::VertexHandle> vertices;
            vertices.reserve(header.vertex_count);
            for (std::size_t i = 0; i < header.vertex_count; ++i)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected end of file while reading PLY vertices: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY vertex without 3 coordinates in file: " + path.string());
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                vertices.push_back(mesh.add_vertex(vec3{x, y, z}));
            }

            for (std::size_t i = 0; i < header.face_count; ++i)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected end of file while reading PLY faces: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }
                const std::size_t n = static_cast<std::size_t>(std::stoul(tokens[0]));
                if (n < 3 || tokens.size() < n + 1)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY face has insufficient vertices in file: " + path.string());
                }
                std::vector<geometry::VertexHandle> face_vertices;
                face_vertices.reserve(n);
                for (std::size_t j = 0; j < n; ++j)
                {
                    const std::size_t idx = static_cast<std::size_t>(std::stoul(tokens[j + 1]));
                    if (idx >= vertices.size())
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "PLY face references invalid vertex index in file: " + path.string());
                    }
                    face_vertices.push_back(vertices[idx]);
                }
                if (!mesh.add_face(face_vertices))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Failed to add face while parsing PLY file: " + path.string());
                }
        }
        }

        void write_mesh_ply(const std::filesystem::path& path, const geometry::MeshInterface& mesh)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PLY file for writing: " + path.string());
            }

            const std::size_t vertex_count = mesh.vertex_count();
            const std::size_t face_count = mesh.face_count();

            stream << "ply\n";
            stream << "format ascii 1.0\n";
            stream << "element vertex " << vertex_count << "\n";
            stream << "property float x\n";
            stream << "property float y\n";
            stream << "property float z\n";
            stream << "element face " << face_count << "\n";
            stream << "property list uchar int vertex_indices\n";
            stream << "end_header\n";

            const std::size_t invalid = std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> vertex_indices(mesh.vertices_size(), invalid);
            std::size_t index{0};
            for (const auto v : mesh.vertices())
            {
                const auto& position = mesh.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
                vertex_indices[v.index()] = index++;
            }

            for (const auto f : mesh.faces())
            {
                std::vector<std::size_t> indices;
                const auto h_start = mesh.halfedge(f);
                if (!h_start.is_valid())
                {
                    continue;
                }
                auto h = h_start;
                do
                {
                    const auto v = mesh.to_vertex(h);
                    const auto idx = vertex_indices[v.index()];
                    if (idx == invalid)
                    {
                        throw GeometryIoException(
                            GeometryIoError::invalid_argument,
                            "Mesh contains face with unregistered vertex while writing PLY");
                    }
                    indices.push_back(idx);
                    h = mesh.next_halfedge(h);
                } while (h != h_start);

                stream << indices.size();
                for (const auto idx : indices)
                {
                    stream << ' ' << idx;
                }
                stream << '\n';
            }
        }

        void read_point_cloud_ply(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            const auto header_result = inspect_ply_header(path);
            if (!header_result)
            {
                throw GeometryIoException(header_result.error().code(), header_result.error().message());
            }
            const auto header = header_result.value();
            if (!header.ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PLY point clouds are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PLY file: " + path.string());
            }

            std::string line;
            std::getline(stream, line); // ply
            while (std::getline(stream, line) && line != "end_header")
            {
            }

            point_cloud.clear();
            point_cloud.reserve(header.vertex_count);

            for (std::size_t i = 0; i < header.vertex_count; ++i)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(
                        GeometryIoError::invalid_argument,
                        "Unexpected end of file while reading PLY point cloud vertices: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY point cloud vertex without 3 coordinates in file: " + path.string());
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                const auto vh = point_cloud.add_vertex(vec3{x, y, z});
                (void)vh;
            }
        }

        void write_point_cloud_ply(const std::filesystem::path& path, const geometry::PointCloudInterface& point_cloud)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PLY file for writing: " + path.string());
            }

            const std::size_t vertex_count = point_cloud.vertex_count();

            stream << "ply\n";
            stream << "format ascii 1.0\n";
            stream << "element vertex " << vertex_count << "\n";
            stream << "property float x\n";
            stream << "property float y\n";
            stream << "property float z\n";
            stream << "end_header\n";

            for (const auto v : point_cloud.vertices())
            {
                const auto& position = point_cloud.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }
        }

        void read_point_cloud_xyz(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open XYZ file: " + path.string());
            }

            point_cloud.clear();

            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    continue;
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                const auto vh = point_cloud.add_vertex(vec3{x, y, z});
                (void)vh;
            }
        }

        void write_point_cloud_xyz(const std::filesystem::path& path, const geometry::PointCloudInterface& point_cloud)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open XYZ file for writing: " + path.string());
            }

            for (const auto v : point_cloud.vertices())
            {
                const auto& position = point_cloud.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }
        }

        void read_point_cloud_pcd(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PCD file: " + path.string());
            }

            std::string line;
            std::size_t point_count{0};
            bool ascii{false};
            while (std::getline(stream, line))
            {
                if (line.empty())
                {
                    continue;
                }
                auto lower = to_lower(line);
                if (starts_with(lower, "#"))
                {
                    continue;
                }
                if (starts_with(lower, "fields"))
                {
                    if (lower.find("x") == std::string::npos || lower.find("y") == std::string::npos
                        || lower.find("z") == std::string::npos)
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "PCD file missing XYZ fields: " + path.string());
                    }
                }
                else if (starts_with(lower, "points"))
                {
                    point_count = static_cast<std::size_t>(std::stoull(lower.substr(lower.find_first_of("0123456789"))));
                }
                else if (starts_with(lower, "data"))
                {
                    ascii = lower.find("ascii") != std::string::npos;
                    break;
                }
            }

            if (!ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PCD files are not supported: " + path.string());
            }

            point_cloud.clear();
            point_cloud.reserve(point_count);

            while (std::getline(stream, line))
            {
                if (line.empty())
                {
                    continue;
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    continue;
                }
                const float x = std::stof(tokens[0]);
                const float y = std::stof(tokens[1]);
                const float z = std::stof(tokens[2]);
                const auto vh = point_cloud.add_vertex(vec3{x, y, z});
                (void)vh;
            }
        }

        void write_point_cloud_pcd(const std::filesystem::path& path, const geometry::PointCloudInterface& point_cloud)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PCD file for writing: " + path.string());
            }

            const std::size_t vertex_count = point_cloud.vertex_count();

            stream << "# .PCD v0.7 - Point Cloud Data file format\n";
            stream << "VERSION 0.7\n";
            stream << "FIELDS x y z\n";
            stream << "SIZE 4 4 4\n";
            stream << "TYPE F F F\n";
            stream << "COUNT 1 1 1\n";
            stream << "WIDTH " << vertex_count << "\n";
            stream << "HEIGHT 1\n";
            stream << "VIEWPOINT 0 0 0 1 0 0 0\n";
            stream << "POINTS " << vertex_count << "\n";
            stream << "DATA ascii\n";

            for (const auto v : point_cloud.vertices())
            {
                const auto& position = point_cloud.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
            }
        }

        void read_graph_edgelist(const std::filesystem::path& path, geometry::GraphInterface& graph)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open edge list file: " + path.string());
            }

            graph.clear();

            std::unordered_map<std::size_t, geometry::VertexHandle> vertex_map;
            auto get_vertex = [&](std::size_t id) {
                auto it = vertex_map.find(id);
                if (it != vertex_map.end())
                {
                    return it->second;
                }
                auto v = graph.add_vertex(vec3{0.0F, 0.0F, 0.0F});
                vertex_map.emplace(id, v);
                return v;
            };

            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 2)
                {
                    continue;
                }
                const std::size_t a = static_cast<std::size_t>(std::stoull(tokens[0]));
                const std::size_t b = static_cast<std::size_t>(std::stoull(tokens[1]));
                const auto va = get_vertex(a);
                const auto vb = get_vertex(b);
                const auto he = graph.add_edge(va, vb);
                (void)he;
            }
        }

        void write_graph_edgelist(const std::filesystem::path& path, const geometry::GraphInterface& graph)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open edge list file for writing: " + path.string());
            }

            const std::size_t invalid = std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> vertex_indices(graph.vertices_size(), invalid);
            std::size_t index{0};
            for (const auto v : graph.vertices())
            {
                vertex_indices[v.index()] = index++;
            }

            for (const auto e : graph.edges())
            {
                const auto v0 = graph.vertex(e, 0);
                const auto v1 = graph.vertex(e, 1);
                const auto idx0 = vertex_indices[v0.index()];
                const auto idx1 = vertex_indices[v1.index()];
                    if (idx0 == invalid || idx1 == invalid)
                    {
                        throw GeometryIoException(
                            GeometryIoError::invalid_argument,
                            "Graph contains edge with unregistered vertex while writing edge list");
                    }
                stream << idx0 << ' ' << idx1 << '\n';
            }
        }

        void read_graph_ply(const std::filesystem::path& path, geometry::GraphInterface& graph)
        {
            const auto header_result = inspect_ply_header(path);
            if (!header_result)
            {
                throw GeometryIoException(header_result.error().code(), header_result.error().message());
            }
            const auto header = header_result.value();
            if (!header.ascii)
            {
                throw GeometryIoException(GeometryIoError::unsupported_format,
                                          "Binary PLY graphs are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open PLY file: " + path.string());
            }

            std::string line;
            std::getline(stream, line); // ply
            while (std::getline(stream, line) && line != "end_header")
            {
            }

            graph.clear();
            graph.reserve(header.vertex_count, header.edge_count);

            std::vector<geometry::VertexHandle> vertices;
            vertices.reserve(header.vertex_count);
            for (std::size_t i = 0; i < header.vertex_count; ++i)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected end of file while reading PLY graph vertices: " + path.string());
                }
                auto tokens = tokenize(line);
                vec3 position{0.0F, 0.0F, 0.0F};
                if (tokens.size() >= 3)
                {
                    position[0] = std::stof(tokens[0]);
                    position[1] = std::stof(tokens[1]);
                    position[2] = std::stof(tokens[2]);
                }
                vertices.push_back(graph.add_vertex(position));
            }

            for (std::size_t i = 0; i < header.edge_count; ++i)
            {
                if (!std::getline(stream, line))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected end of file while reading PLY graph edges: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 2)
                {
                    continue;
                }
                const std::size_t a = static_cast<std::size_t>(std::stoul(tokens[0]));
                const std::size_t b = static_cast<std::size_t>(std::stoul(tokens[1]));
                if (a >= vertices.size() || b >= vertices.size())
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY graph edge references invalid vertex index: " + path.string());
                }
                const auto he = graph.add_edge(vertices[a], vertices[b]);
                (void)he;
            }
        }

        void write_graph_ply(const std::filesystem::path& path, const geometry::GraphInterface& graph)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(GeometryIoError::io_failure,
                                          "Failed to open PLY file for writing: " + path.string());
            }

            const std::size_t vertex_count = graph.vertex_count();
            const std::size_t edge_count = graph.edge_count();

            stream << "ply\n";
            stream << "format ascii 1.0\n";
            stream << "element vertex " << vertex_count << "\n";
            stream << "property float x\n";
            stream << "property float y\n";
            stream << "property float z\n";
            stream << "element edge " << edge_count << "\n";
            stream << "property int vertex1\n";
            stream << "property int vertex2\n";
            stream << "end_header\n";

            const std::size_t invalid = std::numeric_limits<std::size_t>::max();
            std::vector<std::size_t> vertex_indices(graph.vertices_size(), invalid);
            std::size_t index{0};
            for (const auto v : graph.vertices())
            {
                const auto& position = graph.position(v);
                stream << position[0] << ' ' << position[1] << ' ' << position[2] << '\n';
                vertex_indices[v.index()] = index++;
            }

            for (const auto e : graph.edges())
            {
                const auto v0 = graph.vertex(e, 0);
                const auto v1 = graph.vertex(e, 1);
                const auto idx0 = vertex_indices[v0.index()];
                const auto idx1 = vertex_indices[v1.index()];
                    if (idx0 == invalid || idx1 == invalid)
                    {
                        throw GeometryIoException(
                            GeometryIoError::invalid_argument,
                            "Graph contains edge with unregistered vertex while writing PLY");
                    }
                stream << idx0 << ' ' << idx1 << '\n';
            }
        }

        class ObjMeshImporter final : public MeshImporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::obj;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override
            {
                return translate_io_exceptions(path, [&] { read_mesh_obj(path, mesh); });
            }
        };

        class ObjMeshExporter final : public MeshExporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::obj;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override
            {
                return translate_io_exceptions(path, [&] { write_mesh_obj(path, mesh); });
            }
        };

        class OffMeshImporter final : public MeshImporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::off;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override
            {
                return translate_io_exceptions(path, [&] { read_mesh_off(path, mesh); });
            }
        };

        class OffMeshExporter final : public MeshExporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::off;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override
            {
                return translate_io_exceptions(path, [&] { write_mesh_off(path, mesh); });
            }
        };

        class PlyMeshImporter final : public MeshImporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::ply;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override
            {
                return translate_io_exceptions(path, [&] { read_mesh_ply(path, mesh); });
            }
        };

        class PlyMeshExporter final : public MeshExporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::ply;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override
            {
                return translate_io_exceptions(path, [&] { write_mesh_ply(path, mesh); });
            }
        };

        class PlyPointCloudImporter final : public PointCloudImporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::ply;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override
            {
                return translate_io_exceptions(path, [&] { read_point_cloud_ply(path, point_cloud); });
            }
        };

        class PlyPointCloudExporter final : public PointCloudExporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::ply;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_point_cloud(const std::filesystem::path& path,
                               const geometry::PointCloudInterface& point_cloud) const override
            {
                return translate_io_exceptions(path, [&] { write_point_cloud_ply(path, point_cloud); });
            }
        };

        class XyzPointCloudImporter final : public PointCloudImporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::xyz;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override
            {
                return translate_io_exceptions(path, [&] { read_point_cloud_xyz(path, point_cloud); });
            }
        };

        class XyzPointCloudExporter final : public PointCloudExporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::xyz;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_point_cloud(const std::filesystem::path& path,
                               const geometry::PointCloudInterface& point_cloud) const override
            {
                return translate_io_exceptions(path, [&] { write_point_cloud_xyz(path, point_cloud); });
            }
        };

        class PcdPointCloudImporter final : public PointCloudImporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::pcd;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override
            {
                return translate_io_exceptions(path, [&] { read_point_cloud_pcd(path, point_cloud); });
            }
        };

        class PcdPointCloudExporter final : public PointCloudExporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::pcd;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_point_cloud(const std::filesystem::path& path,
                               const geometry::PointCloudInterface& point_cloud) const override
            {
                return translate_io_exceptions(path, [&] { write_point_cloud_pcd(path, point_cloud); });
            }
        };

        class EdgeListGraphImporter final : public GraphImporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::edgelist;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::GraphInterface& graph) const override
            {
                return translate_io_exceptions(path, [&] { read_graph_edgelist(path, graph); });
            }
        };

        class EdgeListGraphExporter final : public GraphExporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::edgelist;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const override
            {
                return translate_io_exceptions(path, [&] { write_graph_edgelist(path, graph); });
            }
        };

        class PlyGraphImporter final : public GraphImporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::ply;
            }

            [[nodiscard]] GeometryIoResult<void>
            import(const std::filesystem::path& path, geometry::GraphInterface& graph) const override
            {
                return translate_io_exceptions(path, [&] { read_graph_ply(path, graph); });
            }
        };

        class PlyGraphExporter final : public GraphExporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::ply;
            }

            [[nodiscard]] GeometryIoResult<void>
            export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const override
            {
                return translate_io_exceptions(path, [&] { write_graph_ply(path, graph); });
            }
        };

        void register_default_geometry_io_plugins_impl(GeometryIORegistry& registry)
        {
            registry.register_mesh_importer(std::make_unique<ObjMeshImporter>());
            registry.register_mesh_exporter(std::make_unique<ObjMeshExporter>());
            registry.register_mesh_importer(std::make_unique<OffMeshImporter>());
            registry.register_mesh_exporter(std::make_unique<OffMeshExporter>());
            registry.register_mesh_importer(std::make_unique<PlyMeshImporter>());
            registry.register_mesh_exporter(std::make_unique<PlyMeshExporter>());

            registry.register_point_cloud_importer(std::make_unique<PlyPointCloudImporter>());
            registry.register_point_cloud_exporter(std::make_unique<PlyPointCloudExporter>());
            registry.register_point_cloud_importer(std::make_unique<XyzPointCloudImporter>());
            registry.register_point_cloud_exporter(std::make_unique<XyzPointCloudExporter>());
            registry.register_point_cloud_importer(std::make_unique<PcdPointCloudImporter>());
            registry.register_point_cloud_exporter(std::make_unique<PcdPointCloudExporter>());

            registry.register_graph_importer(std::make_unique<EdgeListGraphImporter>());
            registry.register_graph_exporter(std::make_unique<EdgeListGraphExporter>());
            registry.register_graph_importer(std::make_unique<PlyGraphImporter>());
            registry.register_graph_exporter(std::make_unique<PlyGraphExporter>());
        }
    } // namespace

    void register_default_geometry_io_plugins(GeometryIORegistry& registry)
    {
        register_default_geometry_io_plugins_impl(registry);
    }


    GeometryIoResult<GeometryDetectionResult> detect_geometry_file(const std::filesystem::path& path)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::detect);

        auto outcome = [&]() -> GeometryIoResult<GeometryDetectionResult> {
            if (!std::filesystem::exists(path))
            {
                return make_geometry_io_error(
                    GeometryIoError::file_not_found,
                    "Cannot detect geometry content of non-existent file: " + path.string());
            }

            std::string signature_prefix;
            const auto ext = extension_of(path);
            auto result = classify_extension_only(ext);
            result.format_hint = ext;

            if (ext == ".ply")
            {
                const auto header_result = inspect_ply_header(path);
                if (!header_result)
                {
                    return header_result.error();
                }
                const auto header = header_result.value();
                if (header.face_count > 0)
                {
                    result.kind = GeometryKind::mesh;
                    result.mesh_format = MeshFileFormat::ply;
                }
                else if (header.edge_count > 0)
                {
                    result.kind = GeometryKind::graph;
                    result.graph_format = GraphFileFormat::ply;
                }
                else if (header.vertex_count > 0)
                {
                    result.kind = GeometryKind::point_cloud;
                    result.point_cloud_format = PointCloudFileFormat::ply;
                }
                return result;
            }

            if (ext == ".stl" || result.kind == GeometryKind::unknown)
            {
                if (auto stl_result = detect_stl_from_signature(path); stl_result.kind != GeometryKind::unknown)
                {
                    if (result.kind == GeometryKind::unknown)
                    {
                        result = stl_result;
                    }
                    else
                    {
                        result.mesh_format = MeshFileFormat::stl;
                    }

                    if (result.format_hint.empty())
                    {
                        result.format_hint = ".stl";
                    }

                    return result;
                }
            }

            if (result.kind == GeometryKind::unknown)
            {
                if (signature_prefix.empty())
                {
                    signature_prefix = read_file_prefix(path, kSignatureScanBytes);
                }

                if (auto signature_result = detect_geometry_from_signatures(signature_prefix);
                    signature_result.kind != GeometryKind::unknown)
                {
                    if (signature_result.format_hint.empty())
                    {
                        signature_result.format_hint = result.format_hint;
                    }
                    return signature_result;
                }
            }

            if (result.kind != GeometryKind::unknown)
            {
                return result;
            }

            std::ifstream stream{path};
            if (!stream)
            {
                return make_geometry_io_error(GeometryIoError::io_failure,
                                              "Failed to open file for detection: " + path.string());
            }

            std::string line;
            if (std::getline(stream, line))
            {
                const auto lower = to_lower(line);
                if (starts_with(lower, "ply"))
                {
                    stream.close();
                    const auto header_result = inspect_ply_header(path);
                    if (!header_result)
                    {
                        return header_result.error();
                    }
                    const auto header = header_result.value();
                    if (header.face_count > 0)
                    {
                        result.kind = GeometryKind::mesh;
                        result.mesh_format = MeshFileFormat::ply;
                    }
                    else if (header.edge_count > 0)
                    {
                        result.kind = GeometryKind::graph;
                        result.graph_format = GraphFileFormat::ply;
                    }
                    else if (header.vertex_count > 0)
                    {
                        result.kind = GeometryKind::point_cloud;
                        result.point_cloud_format = PointCloudFileFormat::ply;
                    }
                    return result;
                }

                if (starts_with(lower, "OFF"))
                {
                    result.kind = GeometryKind::mesh;
                    result.mesh_format = MeshFileFormat::off;
                    return result;
                }

                if (starts_with(lower, "ply"))
                {
                    result.kind = GeometryKind::point_cloud;
                    result.point_cloud_format = PointCloudFileFormat::ply;
                    return result;
                }
            }

            stream.clear();
            stream.seekg(0);

            if (result.kind == GeometryKind::unknown)
            {
                result.kind = GeometryKind::unknown;
            }

            return result;
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::detect);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::detect, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::detect, path, {}, outcome.error());
        }

        return outcome;
    }

    GeometryIoResult<GeometryDetectionResult> load_geometry(const std::filesystem::path& path,
                                                            geometry::MeshInterface* mesh,
                                                            geometry::PointCloudInterface* point_cloud,
                                                            geometry::GraphInterface* graph)
    {
        auto detection_result = detect_geometry_file(path);
        if (!detection_result)
        {
            return detection_result.error();
        }

        auto& detection = detection_result.value();
        switch (detection.kind)
        {
        case GeometryKind::mesh:
            if (mesh == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              "Mesh pointer must not be null when loading a mesh");
            }
            if (auto result = read_mesh(path, *mesh, detection.mesh_format); !result)
            {
                return result.error();
            }
            break;
        case GeometryKind::point_cloud:
            if (point_cloud == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              "Point cloud pointer must not be null when loading a point cloud");
            }
            if (auto result = read_point_cloud(path, *point_cloud, detection.point_cloud_format); !result)
            {
                return result.error();
            }
            break;
        case GeometryKind::graph:
            if (graph == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              "Graph pointer must not be null when loading a graph");
            }
            if (auto result = read_graph(path, *graph, detection.graph_format); !result)
            {
                return result.error();
            }
            break;
        case GeometryKind::unknown:
            return make_geometry_io_error(GeometryIoError::unsupported_format,
                                          "Unable to determine geometry content type for file: " + path.string());
        }

        return detection_result;
    }

    GeometryIoResult<GeometryDetectionResult> save_geometry(const std::filesystem::path& path,
                                                            const geometry::MeshInterface* mesh,
                                                            const geometry::PointCloudInterface* point_cloud,
                                                            const geometry::GraphInterface* graph)
    {
        const bool has_mesh = mesh != nullptr;
        const bool has_point_cloud = point_cloud != nullptr;
        const bool has_graph = graph != nullptr;
        const int provided = static_cast<int>(has_mesh) + static_cast<int>(has_point_cloud) + static_cast<int>(has_graph);
        if (provided != 1)
        {
            return make_geometry_io_error(GeometryIoError::invalid_argument,
                                          "Exactly one geometry pointer must be provided when saving");
        }

        const auto ext = extension_of(path);
        auto detection = classify_extension_only(ext);
        detection.format_hint = ext;

        if (ext == ".ply")
        {
            if (has_mesh)
            {
                detection.kind = GeometryKind::mesh;
                detection.mesh_format = MeshFileFormat::ply;
            }
            else if (has_point_cloud)
            {
                detection.kind = GeometryKind::point_cloud;
                detection.point_cloud_format = PointCloudFileFormat::ply;
            }
            else
            {
                detection.kind = GeometryKind::graph;
                detection.graph_format = GraphFileFormat::ply;
            }
        }

        if (detection.kind == GeometryKind::unknown)
        {
            if (has_mesh)
            {
                detection.kind = GeometryKind::mesh;
                detection.mesh_format = MeshFileFormat::obj;
            }
            else if (has_point_cloud)
            {
                detection.kind = GeometryKind::point_cloud;
                detection.point_cloud_format = PointCloudFileFormat::xyz;
            }
            else
            {
                detection.kind = GeometryKind::graph;
                detection.graph_format = GraphFileFormat::edgelist;
            }
        }

        switch (detection.kind)
        {
        case GeometryKind::mesh:
            if (!has_mesh)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              "Mesh data not provided for mesh export");
            }
            if (auto result = write_mesh(path, *mesh, detection.mesh_format); !result)
            {
                return result.error();
            }
            break;
        case GeometryKind::point_cloud:
            if (!has_point_cloud)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              "Point cloud data not provided for point cloud export");
            }
            if (auto result = write_point_cloud(path, *point_cloud, detection.point_cloud_format); !result)
            {
                return result.error();
            }
            break;
        case GeometryKind::graph:
            if (!has_graph)
            {
                return make_geometry_io_error(GeometryIoError::invalid_argument,
                                              "Graph data not provided for graph export");
            }
            if (auto result = write_graph(path, *graph, detection.graph_format); !result)
            {
                return result.error();
            }
            break;
        case GeometryKind::unknown:
            return make_geometry_io_error(GeometryIoError::unsupported_format,
                                          "Unable to infer target format for export: " + path.string());
        }

        return detection;
    }


    GeometryIoResult<void> read_mesh(const std::filesystem::path& path,
                                     geometry::MeshInterface& mesh,
                                     MeshFileFormat format)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::read_mesh);
        std::string_view format_label = to_string(format);

        auto outcome = [&]() -> GeometryIoResult<void> {
            MeshFileFormat resolved = format;
            if (resolved == MeshFileFormat::unknown)
            {
                auto detection = detect_geometry_file(path);
                if (!detection)
                {
                    return detection.error();
                }
                resolved = detection.value().mesh_format;
            }

            format_label = to_string(resolved);

            if (resolved == MeshFileFormat::unknown)
            {
                return make_geometry_io_error(GeometryIoError::unsupported_format,
                                              "Unable to determine mesh format for file: " + path.string());
            }

            const auto& registry = global_geometry_io_registry();
            const auto* importer = registry.mesh_importer(resolved);
            if (importer == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::plugin_missing,
                                              "No mesh importer registered for format '" +
                                                  std::string(to_string(resolved)) + "' while reading " + path.string());
            }

            if (auto import_result = importer->import(path, mesh); !import_result)
            {
                return import_result.error();
            }

            return {};
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::read_mesh);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::read_mesh, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::read_mesh, path, format_label, outcome.error());
        }

        return outcome;
    }


    GeometryIoResult<void> write_mesh(const std::filesystem::path& path,
                                      const geometry::MeshInterface& mesh,
                                      MeshFileFormat format)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::write_mesh);
        std::string_view format_label = to_string(format);

        auto outcome = [&]() -> GeometryIoResult<void> {
            MeshFileFormat resolved = format;
            if (resolved == MeshFileFormat::unknown)
            {
                resolved = mesh_format_from_extension(extension_of(path));
                if (resolved == MeshFileFormat::unknown)
                {
                    resolved = MeshFileFormat::obj;
                }
            }

            format_label = to_string(resolved);

            if (resolved == MeshFileFormat::unknown)
            {
                return make_geometry_io_error(GeometryIoError::unsupported_format,
                                              "Unable to determine mesh export format for file: " + path.string());
            }

            const auto& registry = global_geometry_io_registry();
            const auto* exporter = registry.mesh_exporter(resolved);
            if (exporter == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::plugin_missing,
                                              "No mesh exporter registered for format '" +
                                                  std::string(to_string(resolved)) + "' while writing " + path.string());
            }

            if (auto export_result = exporter->export_mesh(path, mesh); !export_result)
            {
                return export_result.error();
            }

            return {};
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::write_mesh);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::write_mesh, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::write_mesh, path, format_label, outcome.error());
        }

        return outcome;
    }


    GeometryIoResult<void> read_point_cloud(const std::filesystem::path& path,
                                            geometry::PointCloudInterface& point_cloud,
                                            PointCloudFileFormat format)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::read_point_cloud);
        std::string_view format_label = to_string(format);

        auto outcome = [&]() -> GeometryIoResult<void> {
            PointCloudFileFormat resolved = format;
            if (resolved == PointCloudFileFormat::unknown)
            {
                auto detection = detect_geometry_file(path);
                if (!detection)
                {
                    return detection.error();
                }
                resolved = detection.value().point_cloud_format;
            }

            format_label = to_string(resolved);

            if (resolved == PointCloudFileFormat::unknown)
            {
                return make_geometry_io_error(GeometryIoError::unsupported_format,
                                              "Unable to determine point cloud format for file: " + path.string());
            }

            const auto& registry = global_geometry_io_registry();
            const auto* importer = registry.point_cloud_importer(resolved);
            if (importer == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::plugin_missing,
                                              "No point cloud importer registered for format '" +
                                                  std::string(to_string(resolved)) + "' while reading " + path.string());
            }

            if (auto import_result = importer->import(path, point_cloud); !import_result)
            {
                return import_result.error();
            }

            return {};
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::read_point_cloud);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::read_point_cloud, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::read_point_cloud, path, format_label, outcome.error());
        }

        return outcome;
    }


    GeometryIoResult<void> write_point_cloud(const std::filesystem::path& path,
                                             const geometry::PointCloudInterface& point_cloud,
                                             PointCloudFileFormat format)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::write_point_cloud);
        std::string_view format_label = to_string(format);

        auto outcome = [&]() -> GeometryIoResult<void> {
            PointCloudFileFormat resolved = format;
            if (resolved == PointCloudFileFormat::unknown)
            {
                resolved = point_cloud_format_from_extension(extension_of(path));
                if (resolved == PointCloudFileFormat::unknown)
                {
                    resolved = PointCloudFileFormat::ply;
                }
            }

            format_label = to_string(resolved);

            if (resolved == PointCloudFileFormat::unknown)
            {
                return make_geometry_io_error(GeometryIoError::unsupported_format,
                                              "Unable to determine point cloud export format for file: " + path.string());
            }

            const auto& registry = global_geometry_io_registry();
            const auto* exporter = registry.point_cloud_exporter(resolved);
            if (exporter == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::plugin_missing,
                                              "No point cloud exporter registered for format '" +
                                                  std::string(to_string(resolved)) + "' while writing " + path.string());
            }

            if (auto export_result = exporter->export_point_cloud(path, point_cloud); !export_result)
            {
                return export_result.error();
            }

            return {};
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::write_point_cloud);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::write_point_cloud, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::write_point_cloud, path, format_label, outcome.error());
        }

        return outcome;
    }


    GeometryIoResult<void> read_graph(const std::filesystem::path& path,
                                      geometry::GraphInterface& graph,
                                      GraphFileFormat format)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::read_graph);
        std::string_view format_label = to_string(format);

        auto outcome = [&]() -> GeometryIoResult<void> {
            GraphFileFormat resolved = format;
            if (resolved == GraphFileFormat::unknown)
            {
                auto detection = detect_geometry_file(path);
                if (!detection)
                {
                    return detection.error();
                }
                resolved = detection.value().graph_format;
            }

            format_label = to_string(resolved);

            if (resolved == GraphFileFormat::unknown)
            {
                return make_geometry_io_error(GeometryIoError::unsupported_format,
                                              "Unable to determine graph format for file: " + path.string());
            }

            const auto& registry = global_geometry_io_registry();
            const auto* importer = registry.graph_importer(resolved);
            if (importer == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::plugin_missing,
                                              "No graph importer registered for format '" +
                                                  std::string(to_string(resolved)) + "' while reading " + path.string());
            }

            if (auto import_result = importer->import(path, graph); !import_result)
            {
                return import_result.error();
            }

            return {};
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::read_graph);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::read_graph, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::read_graph, path, format_label, outcome.error());
        }

        return outcome;
    }


    GeometryIoResult<void> write_graph(const std::filesystem::path& path,
                                       const geometry::GraphInterface& graph,
                                       GraphFileFormat format)
    {
        auto& telemetry = GeometryIoTelemetry::instance();
        telemetry.record_attempt(GeometryIoOperation::write_graph);
        std::string_view format_label = to_string(format);

        auto outcome = [&]() -> GeometryIoResult<void> {
            GraphFileFormat resolved = format;
            if (resolved == GraphFileFormat::unknown)
            {
                resolved = graph_format_from_extension(extension_of(path));
                if (resolved == GraphFileFormat::unknown)
                {
                    resolved = GraphFileFormat::edgelist;
                }
            }

            format_label = to_string(resolved);

            if (resolved == GraphFileFormat::unknown)
            {
                return make_geometry_io_error(GeometryIoError::unsupported_format,
                                              "Unable to determine graph export format for file: " + path.string());
            }

            const auto& registry = global_geometry_io_registry();
            const auto* exporter = registry.graph_exporter(resolved);
            if (exporter == nullptr)
            {
                return make_geometry_io_error(GeometryIoError::plugin_missing,
                                              "No graph exporter registered for format '" +
                                                  std::string(to_string(resolved)) + "' while writing " + path.string());
            }

            if (auto export_result = exporter->export_graph(path, graph); !export_result)
            {
                return export_result.error();
            }

            return {};
        }();

        if (outcome)
        {
            telemetry.record_success(GeometryIoOperation::write_graph);
        }
        else
        {
            telemetry.record_failure(GeometryIoOperation::write_graph, outcome.error().code());
            log_geometry_io_failure(GeometryIoOperation::write_graph, path, format_label, outcome.error());
        }

        return outcome;
    }

#if defined(ENGINE_IO_ENABLE_SIGNATURE_TEST_HOOKS)
    namespace detail
    {
        void reset_geometry_signature_cache_for_testing()
        {
            reset_geometry_signature_cache_for_testing_impl();
        }
    } // namespace detail
#endif

    std::ostream& operator<<(std::ostream& stream, GeometryKind kind)
    {
        stream << to_string(kind);
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, MeshFileFormat format)
    {
        stream << to_string(format);
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, PointCloudFileFormat format)
    {
        stream << to_string(format);
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, GraphFileFormat format)
    {
        stream << to_string(format);
        return stream;
    }
} // namespace engine::io

