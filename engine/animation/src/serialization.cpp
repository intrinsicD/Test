#include "engine/animation/api.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace engine::animation
{
    namespace
    {
        constexpr double kTimeEpsilon = 1e-6;

        [[nodiscard]] bool is_finite(const math::vec3& value) noexcept
        {
            return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
        }

        [[nodiscard]] bool is_finite(const math::quat& value) noexcept
        {
            return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]) && std::isfinite(value[3]);
        }

        class JsonParser
        {
        public:
            explicit JsonParser(std::string_view data)
                : m_data(data)
            {
            }

            [[nodiscard]] AnimationClip parse_clip()
            {
                AnimationClip clip{};
                expect_character('{');
                bool first = true;
                while (!consume_character('}'))
                {
                    if (!first)
                    {
                        expect_character(',');
                    }
                    first = false;

                    const std::string key = parse_string();
                    expect_character(':');
                    if (key == "name")
                    {
                        clip.name = parse_string();
                    }
                    else if (key == "duration")
                    {
                        clip.duration = parse_number();
                    }
                    else if (key == "tracks")
                    {
                        parse_tracks(clip);
                    }
                    else
                    {
                        skip_value();
                    }
                }

                return clip;
            }

            void ensure_end()
            {
                skip_whitespace();
                if (m_index != m_data.size())
                {
                    throw std::runtime_error("Unexpected trailing data in animation clip JSON");
                }
            }

        private:
            void parse_tracks(AnimationClip& clip)
            {
                expect_character('[');
                bool first_track = true;
                while (!consume_character(']'))
                {
                    if (!first_track)
                    {
                        expect_character(',');
                    }
                    first_track = false;

                    clip.tracks.emplace_back();
                    parse_track(clip.tracks.back());
                }
            }

            void parse_track(JointTrack& track)
            {
                expect_character('{');
                bool first_field = true;
                while (!consume_character('}'))
                {
                    if (!first_field)
                    {
                        expect_character(',');
                    }
                    first_field = false;

                    const std::string key = parse_string();
                    expect_character(':');
                    if (key == "joint")
                    {
                        track.joint_name = parse_string();
                    }
                    else if (key == "keyframes")
                    {
                        parse_keyframes(track);
                    }
                    else
                    {
                        skip_value();
                    }
                }
            }

            void parse_keyframes(JointTrack& track)
            {
                expect_character('[');
                bool first_keyframe = true;
                while (!consume_character(']'))
                {
                    if (!first_keyframe)
                    {
                        expect_character(',');
                    }
                    first_keyframe = false;

                    track.keyframes.emplace_back();
                    parse_keyframe(track.keyframes.back());
                }
            }

            void parse_keyframe(Keyframe& keyframe)
            {
                expect_character('{');
                bool first_field = true;
                while (!consume_character('}'))
                {
                    if (!first_field)
                    {
                        expect_character(',');
                    }
                    first_field = false;

                    const std::string key = parse_string();
                    expect_character(':');
                    if (key == "time")
                    {
                        keyframe.time = parse_number();
                    }
                    else if (key == "translation")
                    {
                        keyframe.pose.translation = parse_vec3();
                    }
                    else if (key == "scale")
                    {
                        keyframe.pose.scale = parse_vec3();
                    }
                    else if (key == "rotation")
                    {
                        keyframe.pose.rotation = parse_quat();
                    }
                    else
                    {
                        skip_value();
                    }
                }
            }

            [[nodiscard]] math::vec3 parse_vec3()
            {
                math::vec3 value{0.0F, 0.0F, 0.0F};
                expect_character('[');
                for (int i = 0; i < 3; ++i)
                {
                    if (i > 0)
                    {
                        expect_character(',');
                    }
                    value[i] = static_cast<float>(parse_number());
                }
                expect_character(']');
                return value;
            }

            [[nodiscard]] math::quat parse_quat()
            {
                math::quat value{1.0F, 0.0F, 0.0F, 0.0F};
                expect_character('[');
                for (int i = 0; i < 4; ++i)
                {
                    if (i > 0)
                    {
                        expect_character(',');
                    }
                    value[i] = static_cast<float>(parse_number());
                }
                expect_character(']');
                return value;
            }

            [[nodiscard]] double parse_number()
            {
                skip_whitespace();
                const std::size_t start = m_index;
                if (peek_character() == '-' || peek_character() == '+')
                {
                    ++m_index;
                }
                while (std::isdigit(static_cast<unsigned char>(peek_character())) != 0)
                {
                    ++m_index;
                }
                if (peek_character() == '.')
                {
                    ++m_index;
                    while (std::isdigit(static_cast<unsigned char>(peek_character())) != 0)
                    {
                        ++m_index;
                    }
                }
                if (peek_character() == 'e' || peek_character() == 'E')
                {
                    ++m_index;
                    if (peek_character() == '+' || peek_character() == '-')
                    {
                        ++m_index;
                    }
                    while (std::isdigit(static_cast<unsigned char>(peek_character())) != 0)
                    {
                        ++m_index;
                    }
                }

                const std::string_view token = m_data.substr(start, m_index - start);
                if (token.empty())
                {
                    throw std::runtime_error("Invalid numeric literal in animation clip JSON");
                }
                double result = 0.0;
                const auto conversion = std::from_chars(token.data(), token.data() + token.size(), result);
                if (conversion.ec != std::errc{})
                {
                    throw std::runtime_error("Invalid numeric literal in animation clip JSON");
                }
                return result;
            }

            [[nodiscard]] std::string parse_string()
            {
                expect_character('"');
                std::string result;
                bool terminated = false;
                while (m_index < m_data.size())
                {
                    const char ch = m_data[m_index++];
                    if (ch == '"')
                    {
                        terminated = true;
                        break;
                    }
                    if (ch == '\\')
                    {
                        if (m_index >= m_data.size())
                        {
                            throw std::runtime_error("Invalid escape sequence in animation clip JSON");
                        }
                        const char escape = m_data[m_index++];
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
                                if (m_index + 4 > m_data.size())
                                {
                                    throw std::runtime_error("Invalid Unicode escape in animation clip JSON");
                                }
                                unsigned int codepoint = 0;
                                for (int i = 0; i < 4; ++i)
                                {
                                    const char digit = m_data[m_index++];
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
                                        throw std::runtime_error("Invalid Unicode escape in animation clip JSON");
                                    }
                                }
                                if (codepoint <= 0x7F)
                                {
                                    result.push_back(static_cast<char>(codepoint));
                                }
                                else
                                {
                                    throw std::runtime_error("Non-ASCII Unicode escapes are not supported in animation clip JSON");
                                }
                                break;
                            }
                            default:
                                throw std::runtime_error("Unsupported escape sequence in animation clip JSON");
                        }
                        continue;
                    }
                    result.push_back(ch);
                }
                if (!terminated)
                {
                    throw std::runtime_error("Unterminated string in animation clip JSON");
                }
                return result;
            }

            void skip_value()
            {
                skip_whitespace();
                const char ch = peek_character();
                if (ch == '{')
                {
                    expect_character('{');
                    bool first = true;
                    while (!consume_character('}'))
                    {
                        if (!first)
                        {
                            expect_character(',');
                        }
                        first = false;
                        parse_string();
                        expect_character(':');
                        skip_value();
                    }
                    return;
                }
                if (ch == '[')
                {
                    expect_character('[');
                    bool first = true;
                    while (!consume_character(']'))
                    {
                        if (!first)
                        {
                            expect_character(',');
                        }
                        first = false;
                        skip_value();
                    }
                    return;
                }
                if (ch == '"')
                {
                    parse_string();
                    return;
                }
                if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '-' || ch == '+')
                {
                    (void)parse_number();
                    return;
                }
                if (match_literal("true"))
                {
                    return;
                }
                if (match_literal("false"))
                {
                    return;
                }
                if (match_literal("null"))
                {
                    return;
                }
                throw std::runtime_error("Unexpected token in animation clip JSON");
            }

            [[nodiscard]] bool match_literal(std::string_view literal)
            {
                skip_whitespace();
                if (m_data.substr(m_index, literal.size()) == literal)
                {
                    m_index += literal.size();
                    return true;
                }
                return false;
            }

            void expect_character(char expected)
            {
                skip_whitespace();
                if (peek_character() != expected)
                {
                    throw std::runtime_error("Unexpected token in animation clip JSON");
                }
                ++m_index;
            }

            [[nodiscard]] bool consume_character(char candidate)
            {
                skip_whitespace();
                if (peek_character() == candidate)
                {
                    ++m_index;
                    return true;
                }
                return false;
            }

            [[nodiscard]] char peek_character() const
            {
                if (m_index >= m_data.size())
                {
                    return '\0';
                }
                return m_data[m_index];
            }

            void skip_whitespace()
            {
                while (m_index < m_data.size() && std::isspace(static_cast<unsigned char>(m_data[m_index])) != 0)
                {
                    ++m_index;
                }
            }

            std::string_view m_data;
            std::size_t m_index{0};
        };

        void ensure_directory_exists(const std::filesystem::path& path)
        {
            const auto parent = path.parent_path();
            if (!parent.empty())
            {
                std::error_code ec;
                std::filesystem::create_directories(parent, ec);
            }
        }

        [[nodiscard]] std::string escape_json(std::string_view input)
        {
            std::string result;
            result.reserve(input.size());
            for (char ch : input)
            {
                const unsigned char byte = static_cast<unsigned char>(ch);
                switch (ch)
                {
                    case '\\': result.append("\\\\"); break;
                    case '"': result.append("\\\""); break;
                    case '\b': result.append("\\b"); break;
                    case '\f': result.append("\\f"); break;
                    case '\n': result.append("\\n"); break;
                    case '\r': result.append("\\r"); break;
                    case '\t': result.append("\\t"); break;
                    default:
                        if (byte < 0x20U)
                        {
                            constexpr char hex_digits[] = "0123456789abcdef";
                            result.append("\\u00");
                            result.push_back(hex_digits[(byte >> 4U) & 0x0FU]);
                            result.push_back(hex_digits[byte & 0x0FU]);
                        }
                        else
                        {
                            result.push_back(ch);
                        }
                        break;
                }
            }
            return result;
        }

        void write_indent(std::ostream& stream, bool pretty, int depth)
        {
            if (pretty)
            {
                for (int i = 0; i < depth; ++i)
                {
                    stream << "  ";
                }
            }
        }

        void write_number(std::ostream& stream, double value)
        {
            const auto previous_flags = stream.flags();
            const auto previous_precision = stream.precision();
            stream.setf(std::ios::fmtflags(0), std::ios::floatfield);
            stream << std::setprecision(9) << value;
            stream.flags(previous_flags);
            stream.precision(previous_precision);
        }

        void write_vec3(std::ostream& stream, const math::vec3& value, bool pretty)
        {
            stream << '[';
            write_number(stream, value[0]);
            stream << (pretty ? ", " : ",");
            write_number(stream, value[1]);
            stream << (pretty ? ", " : ",");
            write_number(stream, value[2]);
            stream << ']';
        }

        void write_quat(std::ostream& stream, const math::quat& value, bool pretty)
        {
            stream << '[';
            write_number(stream, value[0]);
            stream << (pretty ? ", " : ",");
            write_number(stream, value[1]);
            stream << (pretty ? ", " : ",");
            write_number(stream, value[2]);
            stream << (pretty ? ", " : ",");
            write_number(stream, value[3]);
            stream << ']';
        }
    } // namespace

    std::vector<ClipValidationError> validate_clip(const AnimationClip& clip)
    {
        std::vector<ClipValidationError> errors;
        if (clip.name.empty())
        {
            errors.push_back({"Animation clip must have a non-empty name",
                              {},
                              std::numeric_limits<std::size_t>::max(),
                              std::numeric_limits<std::size_t>::max()});
        }
        if (!std::isfinite(clip.duration) || clip.duration < 0.0)
        {
            errors.push_back({"Animation clip duration must be non-negative and finite",
                              {},
                              std::numeric_limits<std::size_t>::max(),
                              std::numeric_limits<std::size_t>::max()});
        }

        if (clip.tracks.empty())
        {
            errors.push_back({"Animation clip must contain at least one joint track",
                              {},
                              std::numeric_limits<std::size_t>::max(),
                              std::numeric_limits<std::size_t>::max()});
        }

        std::unordered_set<std::string> joint_names;
        double max_time = 0.0;
        for (std::size_t track_index = 0; track_index < clip.tracks.size(); ++track_index)
        {
            const JointTrack& track = clip.tracks[track_index];
            if (track.joint_name.empty())
            {
                errors.push_back({"Joint track must provide a joint name",
                                  {},
                                  track_index,
                                  std::numeric_limits<std::size_t>::max()});
            }
            else if (!joint_names.insert(track.joint_name).second)
            {
                errors.push_back({"Duplicate joint track detected", track.joint_name, track_index, std::numeric_limits<std::size_t>::max()});
            }

            if (track.keyframes.empty())
            {
                errors.push_back({"Joint track must contain at least one keyframe",
                                  track.joint_name,
                                  track_index,
                                  std::numeric_limits<std::size_t>::max()});
                continue;
            }

            double previous_time = -std::numeric_limits<double>::infinity();
            for (std::size_t keyframe_index = 0; keyframe_index < track.keyframes.size(); ++keyframe_index)
            {
                const Keyframe& keyframe = track.keyframes[keyframe_index];
                if (!std::isfinite(keyframe.time) || keyframe.time < 0.0)
                {
                    errors.push_back({"Keyframe time must be finite and non-negative",
                                      track.joint_name,
                                      track_index,
                                      keyframe_index});
                }
                if (keyframe_index > 0 && keyframe.time <= previous_time + kTimeEpsilon)
                {
                    errors.push_back({"Keyframe times must be strictly increasing",
                                      track.joint_name,
                                      track_index,
                                      keyframe_index});
                }
                previous_time = keyframe.time;
                max_time = std::max(max_time, keyframe.time);

                if (!is_finite(keyframe.pose.translation))
                {
                    errors.push_back({"Keyframe translation contains non-finite values",
                                      track.joint_name,
                                      track_index,
                                      keyframe_index});
                }
                if (!is_finite(keyframe.pose.scale))
                {
                    errors.push_back({"Keyframe scale contains non-finite values",
                                      track.joint_name,
                                      track_index,
                                      keyframe_index});
                }
                if (!is_finite(keyframe.pose.rotation))
                {
                    errors.push_back({"Keyframe rotation contains non-finite values",
                                      track.joint_name,
                                      track_index,
                                      keyframe_index});
                }
                if (math::length_squared(keyframe.pose.rotation) <= std::numeric_limits<float>::epsilon())
                {
                    errors.push_back({"Keyframe rotation must be non-zero",
                                      track.joint_name,
                                      track_index,
                                      keyframe_index});
                }
            }
        }

        if (clip.duration > 0.0 && clip.duration + kTimeEpsilon < max_time)
        {
            errors.push_back({"Clip duration is shorter than the final keyframe",
                              {},
                              std::numeric_limits<std::size_t>::max(),
                              std::numeric_limits<std::size_t>::max()});
        }

        return errors;
    }

    void write_clip_json(const AnimationClip& clip, std::ostream& stream, bool pretty)
    {
        const auto errors = validate_clip(clip);
        if (!errors.empty())
        {
            throw std::runtime_error("Cannot serialise invalid animation clip");
        }

        stream << '{';
        if (pretty)
        {
            stream << '\n';
        }

        write_indent(stream, pretty, 1);
        stream << "\"name\": \"" << escape_json(clip.name) << "\"";
        stream << (pretty ? ",\n" : ",");

        write_indent(stream, pretty, 1);
        stream << "\"duration\": ";
        write_number(stream, clip.duration);
        stream << (pretty ? ",\n" : ",");

        write_indent(stream, pretty, 1);
        stream << "\"tracks\": [";
        if (!clip.tracks.empty())
        {
            if (pretty)
            {
                stream << '\n';
            }
            for (std::size_t track_index = 0; track_index < clip.tracks.size(); ++track_index)
            {
                const JointTrack& track = clip.tracks[track_index];
                write_indent(stream, pretty, 2);
                stream << '{';
                if (pretty)
                {
                    stream << '\n';
                }

                write_indent(stream, pretty, 3);
                stream << "\"joint\": \"" << escape_json(track.joint_name) << "\"";
                stream << (pretty ? ",\n" : ",");

                write_indent(stream, pretty, 3);
                stream << "\"keyframes\": [";
                if (!track.keyframes.empty())
                {
                    if (pretty)
                    {
                        stream << '\n';
                    }
                    for (std::size_t keyframe_index = 0; keyframe_index < track.keyframes.size(); ++keyframe_index)
                    {
                        const Keyframe& keyframe = track.keyframes[keyframe_index];
                        write_indent(stream, pretty, 4);
                        stream << '{';
                        if (pretty)
                        {
                            stream << '\n';
                        }

                        write_indent(stream, pretty, 5);
                        stream << "\"time\": ";
                        write_number(stream, keyframe.time);
                        stream << (pretty ? ",\n" : ",");

                        write_indent(stream, pretty, 5);
                        stream << "\"translation\": ";
                        write_vec3(stream, keyframe.pose.translation, pretty);
                        stream << (pretty ? ",\n" : ",");

                        write_indent(stream, pretty, 5);
                        stream << "\"rotation\": ";
                        write_quat(stream, keyframe.pose.rotation, pretty);
                        stream << (pretty ? ",\n" : ",");

                        write_indent(stream, pretty, 5);
                        stream << "\"scale\": ";
                        write_vec3(stream, keyframe.pose.scale, pretty);

                        if (pretty)
                        {
                            stream << '\n';
                        }
                        write_indent(stream, pretty, 4);
                        stream << '}';
                        if (keyframe_index + 1 < track.keyframes.size())
                        {
                            stream << (pretty ? ",\n" : ",");
                        }
                    }
                    if (pretty)
                    {
                        stream << '\n';
                    }
                    write_indent(stream, pretty, 3);
                }
                stream << ']';

                if (pretty)
                {
                    stream << '\n';
                }
                write_indent(stream, pretty, 2);
                stream << '}';
                if (track_index + 1 < clip.tracks.size())
                {
                    stream << (pretty ? ",\n" : ",");
                }
            }
            if (pretty)
            {
                stream << '\n';
            }
            write_indent(stream, pretty, 1);
        }
        stream << ']';
        if (pretty)
        {
            stream << '\n';
        }
        stream << '}';
    }

    AnimationClip read_clip_json(std::istream& stream)
    {
        std::string buffer;
        stream.seekg(0, std::ios::end);
        const std::streampos end_position = stream.tellg();
        if (end_position > std::streampos{0})
        {
            buffer.reserve(static_cast<std::size_t>(end_position));
        }
        stream.seekg(0, std::ios::beg);
        buffer.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        JsonParser parser(buffer);
        AnimationClip clip = parser.parse_clip();
        parser.ensure_end();

        const auto errors = validate_clip(clip);
        if (!errors.empty())
        {
            std::ostringstream message;
            message << "Invalid animation clip JSON:";
            for (const auto& error : errors)
            {
                message << "\n - " << error.message;
                if (!error.joint_name.empty())
                {
                    message << " (joint: " << error.joint_name << ')';
                }
                if (error.keyframe_index != std::numeric_limits<std::size_t>::max())
                {
                    message << " [keyframe " << error.keyframe_index << ']';
                }
            }
            throw std::runtime_error(message.str());
        }

        double max_time = 0.0;
        for (auto& track : clip.tracks)
        {
            sort_keyframes(track);
            if (!track.keyframes.empty())
            {
                max_time = std::max(max_time, track.keyframes.back().time);
            }
        }
        clip.duration = std::max(clip.duration, max_time);

        return clip;
    }

    void save_clip_json(const AnimationClip& clip, const std::filesystem::path& path, bool pretty)
    {
        ensure_directory_exists(path);
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream.is_open())
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Failed to open animation clip file for writing");
        }

        write_clip_json(clip, stream, pretty);
        stream.flush();
        if (!stream)
        {
            throw std::runtime_error("Failed while writing animation clip JSON");
        }
    }

    AnimationClip load_clip_json(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open())
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Failed to open animation clip file for reading");
        }

        return read_clip_json(stream);
    }
} // namespace engine::animation

