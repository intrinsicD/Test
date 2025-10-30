#include "engine/io/importers/mesh.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"
#include "engine/math/vector.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::map_open_error;
        using engine::io::detail::tokenize;
        using engine::math::vec3;

        void read_mesh_obj(const std::filesystem::path& path, geometry::MeshInterface& mesh)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw GeometryIoException(map_open_error(path), "Failed to open OBJ file: " + path.string());
            }

            mesh.clear();

            std::vector<geometry::VertexHandle> vertex_handles;
            bool saw_vertex = false;
            bool saw_face = false;
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
                    saw_vertex = true;
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
                        const int positive_index = index > 0
                                                       ? index
                                                       : static_cast<int>(vertex_handles.size()) + index + 1;
                        if (positive_index <= 0 || static_cast<std::size_t>(positive_index) > vertex_handles.size())
                        {
                            throw GeometryIoException(GeometryIoError::invalid_argument,
                                                      "OBJ face references invalid vertex index in file: "
                                                          + path.string());
                        }
                        face_vertices.push_back(vertex_handles[static_cast<std::size_t>(positive_index - 1)]);
                    }

                    if (!mesh.add_face(face_vertices))
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "Failed to add face while parsing OBJ file: " + path.string());
                    }
                    saw_face = true;
                }
            }

            if (!saw_vertex)
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "OBJ file does not define any vertices: " + path.string());
            }

            if (!saw_face)
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "OBJ file does not define any faces: " + path.string());
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
            if (detail::to_lower(header) != "off")
            {
                throw GeometryIoException(GeometryIoError::invalid_argument,
                                          "Invalid OFF header in file: " + path.string());
            }

            std::size_t vertex_count = 0;
            std::size_t face_count = 0;
            std::size_t edge_count = 0;
            stream >> vertex_count >> face_count >> edge_count;

            mesh.clear();
            mesh.reserve(vertex_count, face_count * 2, face_count);

            std::vector<geometry::VertexHandle> vertices;
            vertices.reserve(vertex_count);

            for (std::size_t i = 0; i < vertex_count; ++i)
            {
                float x = 0.0F;
                float y = 0.0F;
                float z = 0.0F;
                if (!(stream >> x >> y >> z))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected EOF while reading OFF vertices in file: " + path.string());
                }
                vertices.push_back(mesh.add_vertex(vec3{x, y, z}));
            }

            for (std::size_t i = 0; i < face_count; ++i)
            {
                std::size_t vertex_per_face = 0;
                if (!(stream >> vertex_per_face))
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "Unexpected EOF while reading OFF faces in file: " + path.string());
                }

                std::vector<geometry::VertexHandle> face_vertices;
                face_vertices.reserve(vertex_per_face);
                for (std::size_t j = 0; j < vertex_per_face; ++j)
                {
                    std::size_t idx = 0;
                    if (!(stream >> idx) || idx >= vertices.size())
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "OFF face references invalid vertex index in file: "
                                                      + path.string());
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

        void read_mesh_ply(const std::filesystem::path& path, geometry::MeshInterface& mesh)
        {
            const auto header_result = detail::inspect_ply_header(path);
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
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY face without vertex count in file: " + path.string());
                }
                const std::size_t vertex_per_face = static_cast<std::size_t>(std::stoi(tokens[0]));
                if (tokens.size() < vertex_per_face + 1)
                {
                    throw GeometryIoException(GeometryIoError::invalid_argument,
                                              "PLY face has insufficient vertex indices in file: " + path.string());
                }

                std::vector<geometry::VertexHandle> face_vertices;
                face_vertices.reserve(vertex_per_face);
                for (std::size_t j = 0; j < vertex_per_face; ++j)
                {
                    const std::size_t idx = static_cast<std::size_t>(std::stoi(tokens[j + 1]));
                    if (idx >= vertices.size())
                    {
                        throw GeometryIoException(GeometryIoError::invalid_argument,
                                                  "PLY face references invalid vertex index in file: "
                                                      + path.string());
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
    } // namespace

    MeshFileFormat ObjMeshImporter::format() const noexcept
    {
        return MeshFileFormat::obj;
    }

    GeometryIoResult<void> ObjMeshImporter::import(const std::filesystem::path& path,
                                                   geometry::MeshInterface& mesh) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_mesh_obj(path, mesh); });
    }

    MeshFileFormat OffMeshImporter::format() const noexcept
    {
        return MeshFileFormat::off;
    }

    GeometryIoResult<void> OffMeshImporter::import(const std::filesystem::path& path,
                                                   geometry::MeshInterface& mesh) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_mesh_off(path, mesh); });
    }

    MeshFileFormat PlyMeshImporter::format() const noexcept
    {
        return MeshFileFormat::ply;
    }

    GeometryIoResult<void> PlyMeshImporter::import(const std::filesystem::path& path,
                                                   geometry::MeshInterface& mesh) const
    {
        return detail::translate_io_exceptions(path, [&]() { read_mesh_ply(path, mesh); });
    }
} // namespace engine::io
