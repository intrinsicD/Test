#include "engine/io/detail/geometry_io_common.hpp"

#include <fstream>
#include <sstream>

namespace engine::io::detail
{
    GeometryIoError map_open_error(const std::filesystem::path& path) noexcept
    {
        std::error_code ec;
        const bool exists = std::filesystem::exists(path, ec);
        if (ec)
        {
            return GeometryIoError::io_failure;
        }

        return exists ? GeometryIoError::io_failure : GeometryIoError::file_not_found;
    }

    std::vector<std::string> tokenize(const std::string& line)
    {
        std::istringstream stream{line};
        std::vector<std::string> tokens;
        std::string token;
        while (stream >> token)
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

    GeometryIoResult<PlyHeaderInfo> inspect_ply_header(const std::filesystem::path& path)
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

            (void)current_section;
        }

        return info;
    }
} // namespace engine::io::detail
