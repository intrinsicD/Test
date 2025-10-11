#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/mesh/halfedge_mesh.hpp"

namespace engine::geometry::mesh
{
    /// \brief Populate a halfedge mesh from the indexed triangle soup stored in a SurfaceMesh.
    /// \throws std::runtime_error when the source indices are malformed or produce non-manifold topology.
    ENGINE_GEOMETRY_API void build_halfedge_from_surface_mesh(const SurfaceMesh& surface,
                                                              HalfedgeMeshInterface& mesh);

    /// \brief Convert a halfedge mesh into a SurfaceMesh, triangulating polygonal faces on the fly.
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh build_surface_mesh_from_halfedge(
        const HalfedgeMeshInterface& mesh);
}

