#include "engine/io/geometry_io.hpp"
#include "engine/io/geometry_io_registry.hpp"

#include "engine/math/vector.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::math::vec3;

        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        [[nodiscard]] std::string extension_of(const std::filesystem::path& path)
        {
            return to_lower(path.extension().string());
        }

        [[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix)
        {
            return value.substr(0, prefix.size()) == prefix;
        }

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

        [[nodiscard]] PlyHeaderInfo inspect_ply_header(const std::filesystem::path& path)
        {
            PlyHeaderInfo info{};
            std::ifstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open PLY file for inspection: " + path.string());
            }

            std::string line;
            if (!std::getline(stream, line) || to_lower(line) != "ply")
            {
                throw std::runtime_error("Invalid PLY header in file: " + path.string());
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
                throw std::runtime_error("Failed to open OBJ file: " + path.string());
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
                        throw std::runtime_error("OBJ vertex without 3 coordinates in file: " + path.string());
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
                        throw std::runtime_error("OBJ face with fewer than 3 vertices in file: " + path.string());
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
                            throw std::runtime_error("OBJ face references invalid vertex index in file: " + path.string());
                        }
                        face_vertices.push_back(vertex_handles[static_cast<std::size_t>(positive_index - 1)]);
                    }

                    if (!mesh.add_face(face_vertices))
                    {
                        throw std::runtime_error("Failed to add face while parsing OBJ file: " + path.string());
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
                throw std::runtime_error("Failed to open OBJ file for writing: " + path.string());
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
                        throw std::runtime_error("Mesh contains face with unregistered vertex while writing OBJ");
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
                throw std::runtime_error("Failed to open OFF file: " + path.string());
            }

            std::string header;
            stream >> header;
            if (to_lower(header) != "off")
            {
                throw std::runtime_error("Invalid OFF header in file: " + path.string());
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
                    throw std::runtime_error("OFF face has fewer than 3 vertices in file: " + path.string());
                }
                std::vector<geometry::VertexHandle> face_vertices;
                face_vertices.reserve(n);
                for (std::size_t j = 0; j < n; ++j)
                {
                    std::size_t idx{};
                    stream >> idx;
                    if (idx >= vertices.size())
                    {
                        throw std::runtime_error("OFF face references invalid vertex index in file: " + path.string());
                    }
                    face_vertices.push_back(vertices[idx]);
                }
                if (!mesh.add_face(face_vertices))
                {
                    throw std::runtime_error("Failed to add face while parsing OFF file: " + path.string());
                }
            }
        }

        void write_mesh_off(const std::filesystem::path& path, const geometry::MeshInterface& mesh)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open OFF file for writing: " + path.string());
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
                        throw std::runtime_error("Mesh contains face with unregistered vertex while writing OFF");
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
            const auto header = inspect_ply_header(path);
            if (!header.ascii)
            {
                throw std::runtime_error("Binary PLY meshes are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open PLY file: " + path.string());
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
                    throw std::runtime_error("Unexpected end of file while reading PLY vertices: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    throw std::runtime_error("PLY vertex without 3 coordinates in file: " + path.string());
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
                    throw std::runtime_error("Unexpected end of file while reading PLY faces: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.empty())
                {
                    continue;
                }
                const std::size_t n = static_cast<std::size_t>(std::stoul(tokens[0]));
                if (n < 3 || tokens.size() < n + 1)
                {
                    throw std::runtime_error("PLY face has insufficient vertices in file: " + path.string());
                }
                std::vector<geometry::VertexHandle> face_vertices;
                face_vertices.reserve(n);
                for (std::size_t j = 0; j < n; ++j)
                {
                    const std::size_t idx = static_cast<std::size_t>(std::stoul(tokens[j + 1]));
                    if (idx >= vertices.size())
                    {
                        throw std::runtime_error("PLY face references invalid vertex index in file: " + path.string());
                    }
                    face_vertices.push_back(vertices[idx]);
                }
                if (!mesh.add_face(face_vertices))
                {
                    throw std::runtime_error("Failed to add face while parsing PLY file: " + path.string());
                }
            }
        }

        void write_mesh_ply(const std::filesystem::path& path, const geometry::MeshInterface& mesh)
        {
            ensure_parent_directory(path);
            std::ofstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open PLY file for writing: " + path.string());
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
                        throw std::runtime_error("Mesh contains face with unregistered vertex while writing PLY");
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
            const auto header = inspect_ply_header(path);
            if (!header.ascii)
            {
                throw std::runtime_error("Binary PLY point clouds are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open PLY file: " + path.string());
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
                    throw std::runtime_error("Unexpected end of file while reading PLY point cloud vertices: " + path.string());
                }
                auto tokens = tokenize(line);
                if (tokens.size() < 3)
                {
                    throw std::runtime_error("PLY point cloud vertex without 3 coordinates in file: " + path.string());
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
                throw std::runtime_error("Failed to open PLY file for writing: " + path.string());
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
                throw std::runtime_error("Failed to open XYZ file: " + path.string());
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
                throw std::runtime_error("Failed to open XYZ file for writing: " + path.string());
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
                throw std::runtime_error("Failed to open PCD file: " + path.string());
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
                    if (lower.find("x") == std::string::npos || lower.find("y") == std::string::npos || lower.find("z") == std::string::npos)
                    {
                        throw std::runtime_error("PCD file missing XYZ fields: " + path.string());
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
                throw std::runtime_error("Binary PCD files are not supported: " + path.string());
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
                throw std::runtime_error("Failed to open PCD file for writing: " + path.string());
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
                throw std::runtime_error("Failed to open edge list file: " + path.string());
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
                throw std::runtime_error("Failed to open edge list file for writing: " + path.string());
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
                    throw std::runtime_error("Graph contains edge with unregistered vertex while writing edge list");
                }
                stream << idx0 << ' ' << idx1 << '\n';
            }
        }

        void read_graph_ply(const std::filesystem::path& path, geometry::GraphInterface& graph)
        {
            const auto header = inspect_ply_header(path);
            if (!header.ascii)
            {
                throw std::runtime_error("Binary PLY graphs are not supported: " + path.string());
            }

            std::ifstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open PLY file: " + path.string());
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
                    throw std::runtime_error("Unexpected end of file while reading PLY graph vertices: " + path.string());
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
                    throw std::runtime_error("Unexpected end of file while reading PLY graph edges: " + path.string());
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
                    throw std::runtime_error("PLY graph edge references invalid vertex index: " + path.string());
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
                throw std::runtime_error("Failed to open PLY file for writing: " + path.string());
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
                    throw std::runtime_error("Graph contains edge with unregistered vertex while writing PLY");
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

            void import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override
            {
                read_mesh_obj(path, mesh);
            }
        };

        class ObjMeshExporter final : public MeshExporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::obj;
            }

            void export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override
            {
                write_mesh_obj(path, mesh);
            }
        };

        class OffMeshImporter final : public MeshImporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::off;
            }

            void import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override
            {
                read_mesh_off(path, mesh);
            }
        };

        class OffMeshExporter final : public MeshExporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::off;
            }

            void export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override
            {
                write_mesh_off(path, mesh);
            }
        };

        class PlyMeshImporter final : public MeshImporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::ply;
            }

            void import(const std::filesystem::path& path, geometry::MeshInterface& mesh) const override
            {
                read_mesh_ply(path, mesh);
            }
        };

        class PlyMeshExporter final : public MeshExporter
        {
        public:
            [[nodiscard]] MeshFileFormat format() const noexcept override
            {
                return MeshFileFormat::ply;
            }

            void export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const override
            {
                write_mesh_ply(path, mesh);
            }
        };

        class PlyPointCloudImporter final : public PointCloudImporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::ply;
            }

            void import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override
            {
                read_point_cloud_ply(path, point_cloud);
            }
        };

        class PlyPointCloudExporter final : public PointCloudExporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::ply;
            }

            void export_point_cloud(const std::filesystem::path& path,
                                     const geometry::PointCloudInterface& point_cloud) const override
            {
                write_point_cloud_ply(path, point_cloud);
            }
        };

        class XyzPointCloudImporter final : public PointCloudImporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::xyz;
            }

            void import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override
            {
                read_point_cloud_xyz(path, point_cloud);
            }
        };

        class XyzPointCloudExporter final : public PointCloudExporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::xyz;
            }

            void export_point_cloud(const std::filesystem::path& path,
                                     const geometry::PointCloudInterface& point_cloud) const override
            {
                write_point_cloud_xyz(path, point_cloud);
            }
        };

        class PcdPointCloudImporter final : public PointCloudImporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::pcd;
            }

            void import(const std::filesystem::path& path, geometry::PointCloudInterface& point_cloud) const override
            {
                read_point_cloud_pcd(path, point_cloud);
            }
        };

        class PcdPointCloudExporter final : public PointCloudExporter
        {
        public:
            [[nodiscard]] PointCloudFileFormat format() const noexcept override
            {
                return PointCloudFileFormat::pcd;
            }

            void export_point_cloud(const std::filesystem::path& path,
                                     const geometry::PointCloudInterface& point_cloud) const override
            {
                write_point_cloud_pcd(path, point_cloud);
            }
        };

        class EdgeListGraphImporter final : public GraphImporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::edgelist;
            }

            void import(const std::filesystem::path& path, geometry::GraphInterface& graph) const override
            {
                read_graph_edgelist(path, graph);
            }
        };

        class EdgeListGraphExporter final : public GraphExporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::edgelist;
            }

            void export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const override
            {
                write_graph_edgelist(path, graph);
            }
        };

        class PlyGraphImporter final : public GraphImporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::ply;
            }

            void import(const std::filesystem::path& path, geometry::GraphInterface& graph) const override
            {
                read_graph_ply(path, graph);
            }
        };

        class PlyGraphExporter final : public GraphExporter
        {
        public:
            [[nodiscard]] GraphFileFormat format() const noexcept override
            {
                return GraphFileFormat::ply;
            }

            void export_graph(const std::filesystem::path& path, const geometry::GraphInterface& graph) const override
            {
                write_graph_ply(path, graph);
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
        if (!std::filesystem::exists(path))
        {
            return make_geometry_io_error(
                GeometryIoError::file_not_found,
                "Cannot detect geometry content of non-existent file: " + path.string());
        }

        const auto ext = extension_of(path);
        auto result = classify_extension_only(ext);
        result.format_hint = ext;

        if (ext == ".ply")
        {
            const auto header = inspect_ply_header(path);
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
                const auto header = inspect_ply_header(path);
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
            }
            else if (starts_with(lower, "off"))
            {
                result.kind = GeometryKind::mesh;
                result.mesh_format = MeshFileFormat::off;
            }
        }

        return result;
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

        importer->import(path, mesh);
        return {};
    }

    GeometryIoResult<void> write_mesh(const std::filesystem::path& path,
                                      const geometry::MeshInterface& mesh,
                                      MeshFileFormat format)
    {
        MeshFileFormat resolved = format;
        if (resolved == MeshFileFormat::unknown)
        {
            resolved = mesh_format_from_extension(extension_of(path));
            if (resolved == MeshFileFormat::unknown)
            {
                resolved = MeshFileFormat::obj;
            }
        }

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

        exporter->export_mesh(path, mesh);
        return {};
    }

    GeometryIoResult<void> read_point_cloud(const std::filesystem::path& path,
                                            geometry::PointCloudInterface& point_cloud,
                                            PointCloudFileFormat format)
    {
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

        importer->import(path, point_cloud);
        return {};
    }

    GeometryIoResult<void> write_point_cloud(const std::filesystem::path& path,
                                             const geometry::PointCloudInterface& point_cloud,
                                             PointCloudFileFormat format)
    {
        PointCloudFileFormat resolved = format;
        if (resolved == PointCloudFileFormat::unknown)
        {
            resolved = point_cloud_format_from_extension(extension_of(path));
            if (resolved == PointCloudFileFormat::unknown)
            {
                resolved = PointCloudFileFormat::xyz;
            }
        }

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

        exporter->export_point_cloud(path, point_cloud);
        return {};
    }

    GeometryIoResult<void> read_graph(const std::filesystem::path& path,
                                      geometry::GraphInterface& graph,
                                      GraphFileFormat format)
    {
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
                                          "No graph importer registered for format '" + std::string(to_string(resolved)) +
                                              "' while reading " + path.string());
        }

        importer->import(path, graph);
        return {};
    }

    GeometryIoResult<void> write_graph(const std::filesystem::path& path,
                                       const geometry::GraphInterface& graph,
                                       GraphFileFormat format)
    {
        GraphFileFormat resolved = format;
        if (resolved == GraphFileFormat::unknown)
        {
            resolved = graph_format_from_extension(extension_of(path));
            if (resolved == GraphFileFormat::unknown)
            {
                resolved = GraphFileFormat::edgelist;
            }
        }

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
                                          "No graph exporter registered for format '" + std::string(to_string(resolved)) +
                                              "' while writing " + path.string());
        }

        exporter->export_graph(path, graph);
        return {};
    }

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

