#include "engine/io/exporters/mesh.hpp"

#include "engine/io/detail/geometry_io_common.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace engine::io
{
    namespace
    {
        using engine::io::detail::GeometryIoException;
        using engine::io::detail::ensure_parent_directory;

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
                }
                while (h != h_start);
                stream << '\n';
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
                }
                while (h != h_start);

                stream << indices.size();
                for (const auto idx : indices)
                {
                    stream << ' ' << idx;
                }
                stream << '\n';
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
                }
                while (h != h_start);

                stream << indices.size();
                for (const auto idx : indices)
                {
                    stream << ' ' << idx;
                }
                stream << '\n';
            }
        }
    } // namespace

    MeshFileFormat ObjMeshExporter::format() const noexcept
    {
        return MeshFileFormat::obj;
    }

    GeometryIoResult<void>
    ObjMeshExporter::export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_mesh_obj(path, mesh); });
    }

    MeshFileFormat OffMeshExporter::format() const noexcept
    {
        return MeshFileFormat::off;
    }

    GeometryIoResult<void>
    OffMeshExporter::export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_mesh_off(path, mesh); });
    }

    MeshFileFormat PlyMeshExporter::format() const noexcept
    {
        return MeshFileFormat::ply;
    }

    GeometryIoResult<void>
    PlyMeshExporter::export_mesh(const std::filesystem::path& path, const geometry::MeshInterface& mesh) const
    {
        return detail::translate_io_exceptions(path, [&]() { write_mesh_ply(path, mesh); });
    }
} // namespace engine::io
