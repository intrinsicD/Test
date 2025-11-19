#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/graph/graph.hpp"
#include "engine/geometry/mesh/surface_mesh.hpp"

namespace engine::geometry::mesh
{
    /**
     * \brief Build a graph representation capturing the wireframe of a surface mesh.
     *
     * The resulting graph contains one vertex per mesh vertex and an undirected
     * edge for every unique indexed edge in the source mesh. Degenerate edges
     * and duplicate index pairs are ignored.
     */
    [[nodiscard]] ENGINE_GEOMETRY_API geometry::Graph build_wireframe_graph(const SurfaceMesh& surface);
}
