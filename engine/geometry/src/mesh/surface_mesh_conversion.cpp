#include "engine/geometry/mesh/surface_mesh_conversion.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

namespace engine::geometry::mesh
{
    namespace
    {
        [[nodiscard]] bool is_valid_triangle(const SurfaceMesh& surface, std::size_t base_index)
        {
            const auto i0 = surface.indices[base_index];
            const auto i1 = surface.indices[base_index + 1U];
            const auto i2 = surface.indices[base_index + 2U];
            return i0 != i1 && i1 != i2 && i0 != i2;
        }
    } // namespace

    void build_halfedge_from_surface_mesh(const SurfaceMesh& surface, HalfedgeMeshInterface& mesh)
    {
        mesh.clear();

        if (surface.positions.empty())
        {
            return;
        }

        if (surface.indices.size() % 3U != 0U)
        {
            throw std::runtime_error("SurfaceMesh indices are not a multiple of three; only triangles are supported");
        }

        const std::size_t triangle_count = surface.indices.size() / 3U;
        mesh.reserve(surface.positions.size(), triangle_count * 3U, triangle_count);

        std::vector<VertexHandle> vertex_handles;
        vertex_handles.reserve(surface.positions.size());
        for (const auto& position : surface.positions)
        {
            vertex_handles.push_back(mesh.add_vertex(position));
        }

        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle)
        {
            const std::size_t base_index = triangle * 3U;
            const auto i0 = surface.indices[base_index];
            const auto i1 = surface.indices[base_index + 1U];
            const auto i2 = surface.indices[base_index + 2U];

            if (i0 >= vertex_handles.size() || i1 >= vertex_handles.size() || i2 >= vertex_handles.size())
            {
                throw std::runtime_error("SurfaceMesh indices reference vertices outside the available range");
            }

            if (!is_valid_triangle(surface, base_index))
            {
                throw std::runtime_error("SurfaceMesh contains degenerate triangles which cannot form a halfedge face");
            }

            const auto face = mesh.add_triangle(vertex_handles[i0], vertex_handles[i1], vertex_handles[i2]);
            if (!face.has_value())
            {
                throw std::runtime_error("Failed to insert triangle while constructing halfedge mesh; topology may be non-manifold");
            }
        }
    }

    SurfaceMesh build_surface_mesh_from_halfedge(const HalfedgeMeshInterface& mesh)
    {
        SurfaceMesh surface;
        surface.rest_positions.reserve(mesh.vertex_count());
        surface.positions.reserve(mesh.vertex_count());

        if (mesh.vertex_count() == 0U)
        {
            return surface;
        }

        std::vector<std::uint32_t> index_map(mesh.vertices_size(), std::numeric_limits<std::uint32_t>::max());
        for (auto vertex : mesh.vertices())
        {
            if (mesh.is_deleted(vertex))
            {
                continue;
            }

            const auto mapped_index = static_cast<std::uint32_t>(surface.positions.size());
            index_map[vertex.index()] = mapped_index;
            const auto& position = mesh.position(vertex);
            surface.positions.push_back(position);
            surface.rest_positions.push_back(position);
        }

        std::vector<std::uint32_t> polygon;
        for (auto face : mesh.faces())
        {
            if (mesh.is_deleted(face))
            {
                continue;
            }

            polygon.clear();
            auto vertices = mesh.vertices(face);
            const auto end = vertices;
            if (!vertices)
            {
                continue;
            }

            do
            {
                const auto vertex = *vertices;
                const auto mapped = index_map[vertex.index()];
                if (mapped == std::numeric_limits<std::uint32_t>::max())
                {
                    throw std::runtime_error("Halfedge mesh references a deleted vertex while exporting SurfaceMesh");
                }
                polygon.push_back(mapped);
            }
            while (++vertices != end);

            if (polygon.size() < 3U)
            {
                continue;
            }

            const auto base = polygon.front();
            for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
            {
                surface.indices.push_back(base);
                surface.indices.push_back(polygon[i]);
                surface.indices.push_back(polygon[i + 1]);
            }
        }

        if (!surface.indices.empty())
        {
            surface.normals.assign(surface.positions.size(), math::vec3{0.0F, 0.0F, 0.0F});
            recompute_vertex_normals(surface);
        }

        update_bounds(surface);
        return surface;
    }
} // namespace engine::geometry::mesh

