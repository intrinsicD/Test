#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"
#include "engine/core/log.hpp"

namespace engine::geometry::surfaces
{
    // ============================================================================
    // Conversion Utilities
    // ============================================================================

    void to_halfedge_mesh(const SurfaceMesh& surface, mesh::HalfedgeMeshInterface& out_mesh)
    {
        ENGINE_CORE_TRACE("Converting SurfaceMesh to HalfedgeMesh");
        mesh::build_halfedge_from_surface_mesh(surface, out_mesh);
        ENGINE_CORE_DEBUG("HalfedgeMesh created: {} vertices, {} edges, {} faces",
            out_mesh.vertex_count(), out_mesh.edge_count(), out_mesh.face_count());
    }

    SurfaceMesh to_surface_mesh(const mesh::HalfedgeMeshInterface& halfedge_mesh)
    {
        ENGINE_CORE_TRACE("Converting HalfedgeMesh to SurfaceMesh");
        auto surface = mesh::build_surface_mesh_from_halfedge(halfedge_mesh);
        ENGINE_CORE_DEBUG("SurfaceMesh created: {} vertices, {} triangles",
            surface.positions.size(), surface.indices.size() / 3);
        return surface;
    }

} // namespace engine::geometry::surfaces

