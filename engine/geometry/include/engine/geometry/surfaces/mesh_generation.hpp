#pragma once

#include "engine/geometry/shapes.hpp"

// Forward declarations
namespace engine::geometry::mesh
{
    class HalfedgeMeshInterface;
}

namespace engine::geometry::surfaces
{
    // ============================================================================
    // SurfaceMesh Generation from Geometric Shapes
    // ============================================================================

    /// Create a SurfaceMesh from an AABB (axis-aligned bounding box)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(const Aabb& box);

    /// Create a SurfaceMesh from a Sphere with specified subdivisions
    /// @param sphere The sphere to tesselate
    /// @param subdivisions Number of subdivisions (higher = more triangles)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(
        const Sphere& sphere,
        int subdivisions = 2);

    /// Create a SurfaceMesh from a Cylinder
    /// @param cylinder The cylinder to tesselate
    /// @param radial_segments Number of segments around the circumference
    /// @param height_segments Number of segments along the height
    /// @param with_caps Whether to include top and bottom caps
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(
        const Cylinder& cylinder,
        int radial_segments = 32,
        int height_segments = 1,
        bool with_caps = true);

    /// Create a SurfaceMesh from a Capsule
    /// @param capsule The capsule to tesselate
    /// @param radial_segments Number of segments around the circumference
    /// @param height_segments Number of segments along the cylindrical part
    /// @param hemisphere_rings Number of rings for the hemispherical caps
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(
        const Capsule& capsule,
        int radial_segments = 32,
        int height_segments = 1,
        int hemisphere_rings = 8);

    /// Create a SurfaceMesh from an Ellipsoid
    /// @param ellipsoid The ellipsoid to tesselate
    /// @param lat_segments Number of latitude segments
    /// @param lon_segments Number of longitude segments
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(
        const Ellipsoid& ellipsoid,
        int lat_segments = 16,
        int lon_segments = 32);

    /// Create a SurfaceMesh from an OBB (oriented bounding box)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(const Obb& box);

    /// Create a SurfaceMesh from a Plane with specified dimensions
    /// @param plane The plane
    /// @param width Width of the plane mesh
    /// @param height Height of the plane mesh
    /// @param width_segments Number of segments along width
    /// @param height_segments Number of segments along height
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(
        const Plane& plane,
        float width = 1.0f,
        float height = 1.0f,
        int width_segments = 1,
        int height_segments = 1);

    /// Create a SurfaceMesh from a single Triangle
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(const Triangle& triangle);

    /// Create a SurfaceMesh from a Frustum
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh surface_mesh_from(const Frustum& frustum);

    // ============================================================================
    // HalfedgeMesh Generation from Geometric Shapes
    // ============================================================================

    /// Create a HalfedgeMesh from an AABB
    ENGINE_GEOMETRY_API void halfedge_mesh_from(const Aabb& box, mesh::HalfedgeMeshInterface& out_mesh);

    /// Create a HalfedgeMesh from a Sphere
    ENGINE_GEOMETRY_API void halfedge_mesh_from(
        const Sphere& sphere,
        mesh::HalfedgeMeshInterface& out_mesh,
        int subdivisions = 2);

    /// Create a HalfedgeMesh from a Cylinder
    ENGINE_GEOMETRY_API void halfedge_mesh_from(
        const Cylinder& cylinder,
        mesh::HalfedgeMeshInterface& out_mesh,
        int radial_segments = 32,
        int height_segments = 1,
        bool with_caps = true);

    /// Create a HalfedgeMesh from a Capsule
    ENGINE_GEOMETRY_API void halfedge_mesh_from(
        const Capsule& capsule,
        mesh::HalfedgeMeshInterface& out_mesh,
        int radial_segments = 32,
        int height_segments = 1,
        int hemisphere_rings = 8);

    /// Create a HalfedgeMesh from an Ellipsoid
    ENGINE_GEOMETRY_API void halfedge_mesh_from(
        const Ellipsoid& ellipsoid,
        mesh::HalfedgeMeshInterface& out_mesh,
        int lat_segments = 16,
        int lon_segments = 32);

    /// Create a HalfedgeMesh from an OBB
    ENGINE_GEOMETRY_API void halfedge_mesh_from(const Obb& box, mesh::HalfedgeMeshInterface& out_mesh);

    /// Create a HalfedgeMesh from a Plane
    ENGINE_GEOMETRY_API void halfedge_mesh_from(
        const Plane& plane,
        mesh::HalfedgeMeshInterface& out_mesh,
        float width = 1.0f,
        float height = 1.0f,
        int width_segments = 1,
        int height_segments = 1);

    /// Create a HalfedgeMesh from a Triangle
    ENGINE_GEOMETRY_API void halfedge_mesh_from(const Triangle& triangle, mesh::HalfedgeMeshInterface& out_mesh);

    /// Create a HalfedgeMesh from a Frustum
    ENGINE_GEOMETRY_API void halfedge_mesh_from(const Frustum& frustum, mesh::HalfedgeMeshInterface& out_mesh);

    // ============================================================================
    // Mesh Conversion Utilities (re-exported for convenience)
    // ============================================================================

    /// Convert SurfaceMesh to HalfedgeMesh
    ENGINE_GEOMETRY_API void to_halfedge_mesh(
        const SurfaceMesh& surface,
        mesh::HalfedgeMeshInterface& out_mesh);

    /// Convert HalfedgeMesh to SurfaceMesh
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh to_surface_mesh(
        const mesh::HalfedgeMeshInterface& halfedge_mesh);

} // namespace engine::geometry::surfaces

