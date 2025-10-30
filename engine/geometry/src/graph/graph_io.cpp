#include "engine/geometry/graph/graph.hpp"
#include "../../io/include/engine/io/geometry_io.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace engine::geometry::graph
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        [[nodiscard]] IOFlags::Format resolve_format(IOFlags::Format requested,
                                                     const std::filesystem::path& path)
        {
            if (requested != IOFlags::Format::kAuto)
            {
                return requested;
            }

            std::string extension = path.extension().string();
            extension = to_lower(std::move(extension));

            if (extension == ".graph" || extension == ".edge")
            {
                return IOFlags::Format::kEdgeList;
            }

            throw std::runtime_error("Unsupported graph format for file \"" + path.string() + "\"");
        }

        struct ParsedEdge
        {
            std::size_t start{std::numeric_limits<std::size_t>::max()};
            std::size_t end{std::numeric_limits<std::size_t>::max()};
        };
    } // namespace

    void read(GraphInterface& graph, const std::filesystem::path& path)
    {
        const auto result = engine::io::read_graph(path, graph, engine::io::GraphFileFormat::unknown);
        if (!result)
        {
            const auto err = result.error();
            const auto msg = std::string{err.has_message() ? err.message() : err.identifier()};
            throw std::runtime_error(msg.empty() ? std::string{engine::io::to_string(err.code())} : msg);
        }
    }

    void write(const GraphInterface& graph, const std::filesystem::path& path, const IOFlags& flags)
    {
        engine::io::GraphFileFormat fmt = engine::io::GraphFileFormat::unknown;
        switch (resolve_format(flags.format, path))
        {
        case IOFlags::Format::kEdgeList:
            fmt = engine::io::GraphFileFormat::edgelist;
            break;
        case IOFlags::Format::kAuto:
        default:
            fmt = engine::io::GraphFileFormat::unknown;
            break;
        }

        const auto result = engine::io::write_graph(path, graph, fmt);
        if (!result)
        {
            const auto err = result.error();
            const auto msg = std::string{err.has_message() ? err.message() : err.identifier()};
            throw std::runtime_error(msg.empty() ? std::string{engine::io::to_string(err.code())} : msg);
        }
    }
} // namespace engine::geometry::graph