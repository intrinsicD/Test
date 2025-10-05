#include <gtest/gtest.h>

#include "engine/geometry/shapes.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"

namespace engine::geometry
{
    TEST(ShapeInteractionsSphere, CylinderIntersection)
    {
        const Cylinder cylinder{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.0f, 1.5f};
        const Sphere intersecting{{1.3f, 0.0f, 0.0f}, 0.6f};
        const Sphere separated{{1.8f, 0.0f, 0.0f}, 0.6f};

        EXPECT_TRUE(Intersects(cylinder, intersecting));
        EXPECT_TRUE(!Intersects(cylinder, separated));
    }

    TEST(ShapeInteractionsSphere, EllipsoidIntersection)
    {
        const Ellipsoid ellipsoid{{0.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 1.5f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};
        const Sphere intersecting{{1.0f, 0.0f, 0.0f}, 0.75f};
        const Sphere separated{{3.5f, 0.0f, 0.0f}, 0.25f};

        EXPECT_TRUE(Intersects(ellipsoid, intersecting));
        EXPECT_TRUE(!Intersects(ellipsoid, separated));
    }

    TEST(ShapeInteractionsSphere, ObbIntersection)
    {
        const Obb box{{0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 1.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};
        const Sphere inside{{0.5f, 0.0f, 0.0f}, 0.5f};
        const Sphere outside{{3.0f, 0.0f, 0.0f}, 0.25f};

        EXPECT_TRUE(Intersects(box, inside));
        EXPECT_TRUE(!Intersects(box, outside));
    }

    TEST(ShapeInteractionsSphere, SphereIntersection)
    {
        const Sphere a{{0.0f, 0.0f, 0.0f}, 1.5f};
        const Sphere b{{2.0f, 0.0f, 0.0f}, 0.6f};
        const Sphere c{{3.5f, 0.0f, 0.0f}, 0.5f};

        EXPECT_TRUE(Intersects(a, b));
        EXPECT_TRUE(!Intersects(a, c));
    }

    TEST(ShapeInteractionsSphere, SphereSegmentIntersection)
    {
        const Sphere sphere{{0.0f, 0.0f, 0.0f}, 1.0f};
        const Segment hit{{-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}};
        const Segment miss{{2.0f, 2.0f, 0.0f}, {4.0f, 2.0f, 0.0f}};

        EXPECT_TRUE(Intersects(sphere, hit, nullptr));
        EXPECT_TRUE(!Intersects(sphere, miss, nullptr));
    }

    TEST(ShapeInteractionsSphereContains, SphereInsideAabb)
    {
        const Aabb box{{-3.0f, -2.0f, -1.0f}, {3.0f, 2.0f, 1.0f}};
        const Sphere contained{{0.0f, 0.0f, 0.0f}, 1.0f};
        const Sphere spilling{{2.5f, 0.0f, 0.0f}, 1.0f};

        EXPECT_TRUE(Contains(box, contained));
        EXPECT_TRUE(!Contains(box, spilling));
    }

    TEST(ShapeInteractionsSphereContains, SphereInsideCylinder)
    {
        const Cylinder cylinder{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, 2.0f, 2.0f};
        const Sphere contained{{0.0f, 0.0f, 0.0f}, 1.0f};
        const Sphere spilling{{1.5f, 0.0f, 1.75f}, 0.6f};

        EXPECT_TRUE(Contains(cylinder, contained));
        EXPECT_TRUE(!Contains(cylinder, spilling));
    }

    TEST(ShapeInteractionsSphereContains, SphereInsideEllipsoid)
    {
        const Ellipsoid ellipsoid{{1.0f, 0.0f, 0.0f}, {3.0f, 2.0f, 1.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};
        const Sphere contained{{2.0f, 0.0f, 0.0f}, 0.5f};
        const Sphere spilling{{3.5f, 0.0f, 0.0f}, 0.75f};

        EXPECT_TRUE(Contains(ellipsoid, contained));
        EXPECT_TRUE(!Contains(ellipsoid, spilling));
    }

    TEST(ShapeInteractionsSphereContains, SphereInsideObb)
    {
        const Obb box{{0.0f, 0.0f, 0.0f}, {2.0f, 1.5f, 1.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};
        const Sphere contained{{0.5f, 0.0f, 0.0f}, 0.75f};
        const Sphere spilling{{1.8f, 0.0f, 0.0f}, 0.5f};

        EXPECT_TRUE(Contains(box, contained));
        EXPECT_TRUE(!Contains(box, spilling));
    }

    TEST(ShapeInteractionsSphereContains, AabbInsideSphere)
    {
        const Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
        const Aabb inside{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
        const Aabb outside{{-5.5f, 0.0f, 0.0f}, {5.5f, 1.0f, 1.0f}};

        EXPECT_TRUE(Contains(sphere, inside));
        EXPECT_TRUE(!Contains(sphere, outside));
    }

    TEST(ShapeInteractionsSphereContains, CylinderInsideSphere)
    {
        const Sphere sphere{{0.0f, 0.0f, 0.0f}, 4.0f};
        const Cylinder inside{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.0f, 1.5f};
        const Cylinder outside{{3.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.0f, 1.0f};

        EXPECT_TRUE(Contains(sphere, inside));
        EXPECT_TRUE(!Contains(sphere, outside));
    }

    TEST(ShapeInteractionsSphereContains, EllipsoidInsideSphere)
    {
        const Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
        const Ellipsoid inside{{1.0f, 0.0f, 0.0f}, {1.0f, 1.5f, 2.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};
        const Ellipsoid outside{{4.6f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};

        EXPECT_TRUE(Contains(sphere, inside));
        EXPECT_TRUE(!Contains(sphere, outside));
    }

    TEST(ShapeInteractionsSphereContains, ObbInsideSphere)
    {
        const Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
        const Obb inside{{0.0f, 0.0f, 0.0f}, {1.0f, 1.5f, 2.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};
        const Obb outside{{4.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, math::quat{1.0f, 0.0f, 0.0f, 0.0f}};

        EXPECT_TRUE(Contains(sphere, inside));
        EXPECT_TRUE(!Contains(sphere, outside));
    }

    TEST(ShapeInteractionsSphereContains, SphereInsideSphere)
    {
        const Sphere outer{{0.0f, 0.0f, 0.0f}, 3.0f};
        const Sphere inner_ok{{1.0f, 0.0f, 0.0f}, 1.0f};
        const Sphere inner_fail{{2.5f, 0.0f, 0.0f}, 1.0f};

        EXPECT_TRUE(Contains(outer, inner_ok));
        EXPECT_TRUE(!Contains(outer, inner_fail));
    }

    // ============================================================================
    // AABB INTERSECTION TESTS
    // ============================================================================

    TEST(AabbIntersection, AabbAabb)
    {
        const Aabb a{{0, 0, 0}, {2, 2, 2}};
        const Aabb overlapping{{1, 1, 1}, {3, 3, 3}};
        const Aabb separated{{3, 0, 0}, {5, 2, 2}};
        const Aabb touching{{2, 0, 0}, {4, 2, 2}};

        EXPECT_TRUE(Intersects(a, overlapping));
        EXPECT_FALSE(Intersects(a, separated));
        EXPECT_TRUE(Intersects(a, touching)); // Edge case
    }

    TEST(AabbIntersection, AabbCylinder)
    {
        const Aabb box{{-1, -1, -1}, {1, 1, 1}};
        const Cylinder intersecting{{0, 0, 0}, {0, 0, 1}, 0.5f, 0.8f};
        const Cylinder separated{{3, 0, 0}, {0, 0, 1}, 0.5f, 0.5f};

        EXPECT_TRUE(Intersects(box, intersecting));
        EXPECT_FALSE(Intersects(box, separated));
    }

    TEST(AabbIntersection, AabbEllipsoid)
    {
        const Aabb box{{-2, -2, -2}, {2, 2, 2}};
        const Ellipsoid inside{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Ellipsoid outside{{5, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(box, inside));
        EXPECT_FALSE(Intersects(box, outside));
    }

    TEST(AabbIntersection, AabbLine)
    {
        const Aabb box{{0, 0, 0}, {1, 1, 1}};
        Result result;

        // Line through box
        EXPECT_TRUE(Intersects(box, Line{{-1, 0.5f, 0.5f}, {1, 0, 0}}, &result));
        EXPECT_GT(result.t_max, result.t_min);

        // Line missing box
        EXPECT_FALSE(Intersects(box, Line{{2, 2, 2}, {1, 0, 0}}, nullptr));
    }

    TEST(AabbIntersection, AabbObb)
    {
        const Aabb aabb{{-1, -1, -1}, {1, 1, 1}};
        const Obb overlapping{{0, 0, 0}, {0.5f, 0.5f, 0.5f}, math::quat{1, 0, 0, 0}};
        const Obb separated{{5, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(aabb, overlapping));
        EXPECT_FALSE(Intersects(aabb, separated));
    }

    TEST(AabbIntersection, AabbPlane)
    {
        const Aabb box{{-1, -1, -1}, {1, 1, 1}};
        const Plane through{{0, 0, 1}, 0}; // z = 0 plane
        const Plane above{{0, 0, 1}, -2}; // z = 2 plane

        EXPECT_TRUE(Intersects(box, through));
        EXPECT_FALSE(Intersects(box, above));
    }

    TEST(AabbIntersection, AabbRay)
    {
        const Aabb box{{0, 0, 0}, {1, 1, 1}};
        Result result;

        // Ray hitting box
        EXPECT_TRUE(Intersects(box, Ray{{-1, 0.5f, 0.5f}, {1, 0, 0}}, &result));
        EXPECT_GE(result.t_min, 0);

        // Ray pointing away
        EXPECT_FALSE(Intersects(box, Ray{{-1, 0.5f, 0.5f}, {-1, 0, 0}}, nullptr));
    }

    TEST(AabbIntersection, AabbSegment)
    {
        const Aabb box{{0, 0, 0}, {1, 1, 1}};
        const Segment through{{-0.5f, 0.5f, 0.5f}, {1.5f, 0.5f, 0.5f}};
        const Segment outside{{2, 2, 2}, {3, 3, 3}};

        EXPECT_TRUE(Intersects(box, through, nullptr));
        EXPECT_FALSE(Intersects(box, outside, nullptr));
    }

    TEST(AabbIntersection, AabbSphere)
    {
        const Aabb box{{0, 0, 0}, {1, 1, 1}};
        const Sphere overlapping{{1.5f, 0.5f, 0.5f}, 0.6f};
        const Sphere separated{{3, 0, 0}, 0.5f};

        EXPECT_TRUE(Intersects(box, overlapping));
        EXPECT_FALSE(Intersects(box, separated));
    }

    TEST(AabbIntersection, AabbTriangle)
    {
        const Aabb box{{-1, -1, -1}, {1, 1, 1}};
        const Triangle inside{{-0.5f, -0.5f, 0}, {0.5f, -0.5f, 0}, {0, 0.5f, 0}};
        const Triangle outside{{2, 2, 2}, {3, 2, 2}, {2.5f, 3, 2}};

        EXPECT_TRUE(Intersects(box, inside));
        EXPECT_FALSE(Intersects(box, outside));
    }

    // ============================================================================
    // CYLINDER INTERSECTION TESTS
    // ============================================================================

    TEST(CylinderIntersection, CylinderCylinder)
    {
        const Cylinder a{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
        const Cylinder overlapping{{0.5f, 0, 0}, {0, 0, 1}, 0.8f, 2.0f};
        const Cylinder separated{{3, 0, 0}, {0, 0, 1}, 0.5f, 2.0f};

        EXPECT_TRUE(Intersects(a, overlapping));
        EXPECT_FALSE(Intersects(a, separated));
    }

    TEST(CylinderIntersection, CylinderEllipsoid)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 1.5f};
        const Ellipsoid overlapping{{1, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Ellipsoid separated{{5, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(cyl, overlapping));
        EXPECT_FALSE(Intersects(cyl, separated));
    }

    TEST(CylinderIntersection, CylinderLine)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
        Result result;

        EXPECT_TRUE(Intersects(cyl, Line{{0, 0, 0}, {1, 0, 0}}, &result));
        EXPECT_FALSE(Intersects(cyl, Line{{3, 3, 0}, {1, 0, 0}}, nullptr));
    }

    TEST(CylinderIntersection, CylinderObb)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 1.5f};
        const Obb centered_box{{0, 0, 0}, {0.5f, 0.5f, 0.5f}, math::quat{1, 0, 0, 0}};
        const Obb separated{{5, 0, 0}, {0.5f, 0.5f, 0.5f}, math::quat{1, 0, 0, 0}};
        const Cylinder separated_axial{{0, 0, 2}, {0, 0, 1}, 1.0f, 0.5f};

        EXPECT_TRUE(Intersects(cyl, centered_box));
        EXPECT_FALSE(Intersects(cyl, separated));
        EXPECT_FALSE(Intersects(separated_axial, centered_box));
    }

    TEST(CylinderIntersection, CylinderPlane)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
        const Plane through{{1, 0, 0}, 0};
        const Plane outside{{1, 0, 0}, -3};

        EXPECT_TRUE(Intersects(cyl, through));
        EXPECT_FALSE(Intersects(cyl, outside));
    }

    TEST(CylinderIntersection, CylinderRay)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
        Result result;

        EXPECT_TRUE(Intersects(cyl, Ray{{-2, 0, 0}, {1, 0, 0}}, &result));
        EXPECT_GE(result.t_min, 0);
    }

    TEST(CylinderIntersection, CylinderSegment)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
        const Segment through{{-2, 0, 0}, {2, 0, 0}};
        const Segment outside{{3, 3, 0}, {4, 4, 0}};

        EXPECT_TRUE(Intersects(cyl, through, nullptr));
        EXPECT_FALSE(Intersects(cyl, outside, nullptr));
    }

    TEST(CylinderIntersection, CylinderSphere)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 1.5f};
        const Sphere overlapping{{1.3f, 0, 0}, 0.6f};
        const Sphere separated{{1.8f, 0, 0}, 0.6f};

        EXPECT_TRUE(Intersects(cyl, overlapping));
        EXPECT_FALSE(Intersects(cyl, separated));
    }

    TEST(CylinderIntersection, CylinderTriangle)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 1.5f};
        const Triangle intersecting{{-2, 0, 0}, {2, 0, 0}, {0, 2, 0}};
        const Triangle separated{{3, 3, 0}, {4, 3, 0}, {3.5f, 4, 0}};

        EXPECT_TRUE(Intersects(cyl, intersecting));
        EXPECT_FALSE(Intersects(cyl, separated));
    }

    // ============================================================================
    // ELLIPSOID INTERSECTION TESTS
    // ============================================================================

    TEST(EllipsoidIntersection, EllipsoidEllipsoid)
    {
        const Ellipsoid a{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        const Ellipsoid overlapping{{2, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Ellipsoid separated{{5, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(a, overlapping));
        EXPECT_FALSE(Intersects(a, separated));
    }

    TEST(EllipsoidIntersection, EllipsoidLine)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        Result result;

        EXPECT_TRUE(Intersects(ellipsoid, Line{{0, 0, 0}, {1, 0, 0}}, &result));
        EXPECT_FALSE(Intersects(ellipsoid, Line{{0, 3, 0}, {1, 0, 0}}, nullptr));
    }

    TEST(EllipsoidIntersection, EllipsoidObb)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        const Obb overlapping{{1, 0, 0}, {0.5f, 0.5f, 0.5f}, math::quat{1, 0, 0, 0}};
        const Obb separated{{5, 0, 0}, {0.5f, 0.5f, 0.5f}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(ellipsoid, overlapping));
        EXPECT_FALSE(Intersects(ellipsoid, separated));
    }

    TEST(EllipsoidIntersection, EllipsoidPlane)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        const Plane through{{1, 0, 0}, 0};
        const Plane outside{{1, 0, 0}, -3};

        EXPECT_TRUE(Intersects(ellipsoid, through));
        EXPECT_FALSE(Intersects(ellipsoid, outside));
    }

    TEST(EllipsoidIntersection, EllipsoidRay)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        Result result;

        EXPECT_TRUE(Intersects(ellipsoid, Ray{{-3, 0, 0}, {1, 0, 0}}, &result));
        EXPECT_GE(result.t_min, 0);
    }

    TEST(EllipsoidIntersection, EllipsoidSegment)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        const Segment through{{-3, 0, 0}, {3, 0, 0}};
        const Segment outside{{0, 3, 0}, {1, 3, 0}};

        EXPECT_TRUE(Intersects(ellipsoid, through, nullptr));
        EXPECT_FALSE(Intersects(ellipsoid, outside, nullptr));
    }

    TEST(EllipsoidIntersection, EllipsoidSphere)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        const Sphere overlapping{{1, 0, 0}, 0.75f};
        const Sphere separated{{3.5f, 0, 0}, 0.25f};

        EXPECT_TRUE(Intersects(ellipsoid, overlapping));
        EXPECT_FALSE(Intersects(ellipsoid, separated));
    }

    TEST(EllipsoidIntersection, EllipsoidTriangle)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        const Triangle intersecting{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
        const Triangle separated{{3, 3, 0}, {4, 3, 0}, {3.5f, 4, 0}};

        EXPECT_TRUE(Intersects(ellipsoid, intersecting));
        EXPECT_FALSE(Intersects(ellipsoid, separated));
    }

    // ============================================================================
    // LINE INTERSECTION TESTS
    // ============================================================================

    TEST(LineIntersection, LineLine)
    {
        Result result;

        // Intersecting lines
        const Line a{{0, 0, 0}, {1, 0, 0}};
        const Line b{{0, 0, 0}, {0, 1, 0}};
        EXPECT_TRUE(Intersects(a, b, &result));

        // Parallel lines
        const Line c{{0, 1, 0}, {1, 0, 0}};
        EXPECT_FALSE(Intersects(a, c, nullptr));

        // Skew lines
        const Line d{{0, 0, 1}, {1, 0, 0}};
        EXPECT_FALSE(Intersects(a, d, nullptr));
    }

    TEST(LineIntersection, LinePlane)
    {
        const Plane plane{{0, 0, 1}, 0}; // z = 0
        Result result;

        // Intersecting
        EXPECT_TRUE(Intersects(Line{{0, 0, -1}, {0, 0, 1}}, plane, &result));

        // Parallel
        EXPECT_FALSE(Intersects(Line{{0, 0, 1}, {1, 0, 0}}, plane, nullptr));
    }

    TEST(LineIntersection, LineRay)
    {
        const Line line{{0, 0, 0}, {1, 0, 0}};
        const Ray intersecting{{0, 1, 0}, {0, -1, 0}};
        const Ray parallel{{0, 1, 0}, {1, 0, 0}};

        EXPECT_TRUE(Intersects(line, intersecting, nullptr));
        EXPECT_FALSE(Intersects(line, parallel, nullptr));
    }

    TEST(LineIntersection, LineSegment)
    {
        const Line line{{0, 0, 0}, {1, 0, 0}};
        const Segment intersecting{{0, -1, 0}, {0, 1, 0}};
        const Segment outside{{1, 1, 0}, {1, 2, 0}};

        EXPECT_TRUE(Intersects(line, intersecting, nullptr));
        EXPECT_FALSE(Intersects(line, outside, nullptr));
    }

    TEST(LineIntersection, LineSphere)
    {
        const Sphere sphere{{0, 0, 0}, 1.0f};
        Result result;

        // Through center
        EXPECT_TRUE(Intersects(Line{{-2, 0, 0}, {1, 0, 0}}, sphere, &result));
        EXPECT_LT(result.t_min, result.t_max);

        // Missing
        EXPECT_FALSE(Intersects(Line{{0, 2, 0}, {1, 0, 0}}, sphere, nullptr));
    }

    TEST(LineIntersection, LineTriangle)
    {
        const Triangle tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
        const Line through{{0, 0, -1}, {0, 0, 1}};
        const Line outside{{2, 0, -1}, {0, 0, 1}};

        EXPECT_TRUE(Intersects(through, tri, nullptr));
        EXPECT_FALSE(Intersects(outside, tri, nullptr));
    }

    // ============================================================================
    // OBB INTERSECTION TESTS
    // ============================================================================

    TEST(ObbIntersection, ObbObb)
    {
        const Obb a{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Obb overlapping{{0.5f, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Obb separated{{5, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(a, overlapping));
        EXPECT_FALSE(Intersects(a, separated));
    }

    TEST(ObbIntersection, ObbPlane)
    {
        const Obb box{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Plane through{{1, 0, 0}, 0};
        const Plane outside{{1, 0, 0}, -3};

        EXPECT_TRUE(Intersects(box, through));
        EXPECT_FALSE(Intersects(box, outside));
    }

    TEST(ObbIntersection, ObbRay)
    {
        const Obb box{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        Result result;

        EXPECT_TRUE(Intersects(box, Ray{{-2, 0, 0}, {1, 0, 0}}, &result));
        EXPECT_GE(result.t_min, 0);
    }

    TEST(ObbIntersection, ObbSegment)
    {
        const Obb box{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Segment through{{-2, 0, 0}, {2, 0, 0}};
        const Segment outside{{3, 3, 0}, {4, 4, 0}};

        EXPECT_TRUE(Intersects(box, through, nullptr));
        EXPECT_FALSE(Intersects(box, outside, nullptr));
    }

    TEST(ObbIntersection, ObbSphere)
    {
        const Obb box{{0, 0, 0}, {1, 2, 1}, math::quat{1, 0, 0, 0}};
        const Sphere overlapping{{0.5f, 0, 0}, 0.5f};
        const Sphere separated{{3, 0, 0}, 0.25f};

        EXPECT_TRUE(Intersects(box, overlapping));
        EXPECT_FALSE(Intersects(box, separated));
    }

    TEST(ObbIntersection, ObbTriangle)
    {
        const Obb box{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Triangle intersecting{{-2, 0, 0}, {2, 0, 0}, {0, 2, 0}};
        const Triangle separated{{3, 3, 0}, {4, 3, 0}, {3.5f, 4, 0}};

        EXPECT_TRUE(Intersects(box, intersecting));
        EXPECT_FALSE(Intersects(box, separated));
    }

    // ============================================================================
    // PLANE INTERSECTION TESTS
    // ============================================================================

    TEST(PlaneIntersection, PlanePlane)
    {
        const Plane a{{0, 0, 1}, 0};
        const Plane intersecting{{1, 0, 0}, 0};
        const Plane parallel{{0, 0, 1}, -1};
        const Plane coincident{{0, 0, 1}, 0};

        EXPECT_TRUE(Intersects(a, intersecting));
        EXPECT_FALSE(Intersects(a, parallel));
        EXPECT_TRUE(Intersects(a, coincident));
    }

    TEST(PlaneIntersection, PlaneRay)
    {
        const Plane plane{{0, 0, 1}, 0};
        Result result;

        EXPECT_TRUE(Intersects(plane, Ray{{0, 0, -1}, {0, 0, 1}}, &result));
        EXPECT_GE(result.t, 0);

        // Parallel
        EXPECT_FALSE(Intersects(plane, Ray{{0, 0, 1}, {1, 0, 0}}, nullptr));
    }

    TEST(PlaneIntersection, PlaneSegment)
    {
        const Plane plane{{0, 0, 1}, 0};
        const Segment through{{0, 0, -1}, {0, 0, 1}};
        const Segment outside{{0, 0, 1}, {0, 0, 2}};

        EXPECT_TRUE(Intersects(plane, through, nullptr));
        EXPECT_FALSE(Intersects(plane, outside, nullptr));
    }

    TEST(PlaneIntersection, PlaneSphere)
    {
        const Plane plane{{0, 0, 1}, 0};
        const Sphere intersecting{{0, 0, 0}, 1.0f};
        const Sphere separated{{0, 0, 2}, 0.5f};

        EXPECT_TRUE(Intersects(plane, intersecting));
        EXPECT_FALSE(Intersects(plane, separated));
    }

    TEST(PlaneIntersection, PlaneTriangle)
    {
        const Plane plane{{0, 0, 1}, 0};
        const Triangle spanning{{0, 0, -1}, {0, 0, 1}, {1, 0, 0}};
        const Triangle above{{0, 0, 1}, {1, 0, 1}, {0, 1, 1}};

        EXPECT_TRUE(Intersects(plane, spanning));
        EXPECT_FALSE(Intersects(plane, above));
    }

    // ============================================================================
    // RAY INTERSECTION TESTS
    // ============================================================================

    TEST(RayIntersection, RayRay)
    {
        Result result;

        // Intersecting
        EXPECT_TRUE(Intersects(Ray{{0, 0, 0}, {1, 0, 0}},
                               Ray{{0, 0, 0}, {0, 1, 0}}, &result));

        // Parallel
        EXPECT_FALSE(Intersects(Ray{{0, 0, 0}, {1, 0, 0}},
                                Ray{{0, 1, 0}, {1, 0, 0}}, nullptr));

        // Pointing away
        EXPECT_FALSE(Intersects(Ray{{1, 0, 0}, {1, 0, 0}},
                                Ray{{-1, 0, 0}, {-1, 0, 0}}, nullptr));
    }

    TEST(RayIntersection, RaySegment)
    {
        const Ray ray{{0, 0, 0}, {1, 0, 0}};
        const Segment intersecting{{1, -1, 0}, {1, 1, 0}};
        const Segment behind{{-1, -1, 0}, {-1, 1, 0}};

        EXPECT_TRUE(Intersects(ray, intersecting, nullptr));
        EXPECT_FALSE(Intersects(ray, behind, nullptr));
    }

    TEST(RayIntersection, RaySphere)
    {
        const Sphere sphere{{0, 0, 0}, 1.0f};
        Result result;

        // From outside
        EXPECT_TRUE(Intersects(Ray{{-2, 0, 0}, {1, 0, 0}}, sphere, &result));
        EXPECT_GE(result.t_min, 0);

        // Missing
        EXPECT_FALSE(Intersects(Ray{{0, 2, 0}, {1, 0, 0}}, sphere, nullptr));
    }

    TEST(RayIntersection, RayTriangle)
    {
        const Triangle tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
        Result result;

        // Through triangle
        EXPECT_TRUE(Intersects(Ray{{0, 0, -1}, {0, 0, 1}}, tri, &result));
        EXPECT_GE(result.t, 0);

        // Behind ray
        EXPECT_FALSE(Intersects(Ray{{0, 0, 1}, {0, 0, 1}}, tri, nullptr));
    }

    // ============================================================================
    // SEGMENT INTERSECTION TESTS
    // ============================================================================

    TEST(SegmentIntersection, SegmentSegment)
    {
        Result result;

        // Intersecting
        const Segment a{{-1, 0, 0}, {1, 0, 0}};
        const Segment b{{0, -1, 0}, {0, 1, 0}};
        EXPECT_TRUE(Intersects(a, b, &result));

        // Parallel
        const Segment c{{-1, 1, 0}, {1, 1, 0}};
        EXPECT_FALSE(Intersects(a, c, nullptr));

        // Non-intersecting
        const Segment d{{2, 2, 0}, {3, 3, 0}};
        EXPECT_FALSE(Intersects(a, d, nullptr));
    }

    TEST(SegmentIntersection, SegmentSphere)
    {
        const Sphere sphere{{0, 0, 0}, 1.0f};
        const Segment through{{-2, 0, 0}, {2, 0, 0}};
        const Segment outside{{2, 2, 0}, {3, 3, 0}};

        EXPECT_TRUE(Intersects(through, sphere, nullptr));
        EXPECT_FALSE(Intersects(outside, sphere, nullptr));
    }

    TEST(SegmentIntersection, SegmentTriangle)
    {
        const Triangle tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
        const Segment through{{0, 0, -1}, {0, 0, 1}};
        const Segment outside{{2, 2, -1}, {2, 2, 1}};

        EXPECT_TRUE(Intersects(through, tri, nullptr));
        EXPECT_FALSE(Intersects(outside, tri, nullptr));
    }

    // ============================================================================
    // SPHERE INTERSECTION TESTS (Extended)
    // ============================================================================

    TEST(SphereIntersection, SphereSphere)
    {
        const Sphere a{{0, 0, 0}, 1.5f};
        const Sphere overlapping{{2, 0, 0}, 0.6f};
        const Sphere separated{{3.5f, 0, 0}, 0.5f};
        const Sphere touching{{2.1f, 0, 0}, 0.1f};

        EXPECT_TRUE(Intersects(a, overlapping));
        EXPECT_FALSE(Intersects(a, separated));
        EXPECT_FALSE(Intersects(a, touching)); // Just touching
    }

    TEST(SphereIntersection, SphereTriangle)
    {
        const Sphere sphere{{0, 0, 0}, 1.0f};
        const Triangle intersecting{{-2, 0, 0}, {2, 0, 0}, {0, 2, 0}};
        const Triangle separated{{3, 3, 3}, {4, 3, 3}, {3.5f, 4, 3}};

        EXPECT_TRUE(Intersects(sphere, intersecting));
        EXPECT_FALSE(Intersects(sphere, separated));
    }

    // ============================================================================
    // TRIANGLE INTERSECTION TESTS
    // ============================================================================

    TEST(TriangleIntersection, TriangleTriangle)
    {
        const Triangle a{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};

        // Overlapping
        const Triangle b{{-0.5f, -0.5f, 0}, {0.5f, -0.5f, 0}, {0, 0.5f, 0}};
        EXPECT_TRUE(Intersects(a, b));

        // Edge intersection
        const Triangle c{{-2, 0, -1}, {2, 0, -1}, {0, 0, 1}};
        EXPECT_TRUE(Intersects(a, c));

        // Separated
        const Triangle d{{3, 3, 0}, {4, 3, 0}, {3.5f, 4, 0}};
        EXPECT_FALSE(Intersects(a, d));

        // Coplanar non-overlapping
        const Triangle e{{2, -1, 0}, {3, -1, 0}, {2.5f, 1, 0}};
        EXPECT_FALSE(Intersects(a, e));
    }

    // ============================================================================
    // CONTAINMENT TESTS
    // ============================================================================

    TEST(Containment, AabbContainsPoint)
    {
        const Aabb box{{0, 0, 0}, {1, 1, 1}};
        EXPECT_TRUE(Contains(box, math::vec3{0.5f, 0.5f, 0.5f}));
        EXPECT_FALSE(Contains(box, math::vec3{2, 0, 0}));
    }

    TEST(Containment, AabbContainsAabb)
    {
        const Aabb outer{{-2, -2, -2}, {2, 2, 2}};
        const Aabb inner{{-1, -1, -1}, {1, 1, 1}};
        const Aabb spilling{{-1, -1, -1}, {3, 1, 1}};

        EXPECT_TRUE(Contains(outer, inner));
        EXPECT_FALSE(Contains(outer, spilling));
    }

    TEST(Containment, SphereContainsSphere)
    {
        const Sphere outer{{0, 0, 0}, 3.0f};
        const Sphere inner{{1, 0, 0}, 1.0f};
        const Sphere spilling{{2.5f, 0, 0}, 1.0f};

        EXPECT_TRUE(Contains(outer, inner));
        EXPECT_FALSE(Contains(outer, spilling));
    }

    TEST(Containment, CylinderContainsPoint)
    {
        const Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
        EXPECT_TRUE(Contains(cyl, math::vec3{0.5f, 0, 0}));
        EXPECT_FALSE(Contains(cyl, math::vec3{2, 0, 0}));
        EXPECT_FALSE(Contains(cyl, math::vec3{0, 0, 3}));
    }

    TEST(Containment, ObbContainsPoint)
    {
        const Obb box{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        EXPECT_TRUE(Contains(box, math::vec3{0.5f, 0, 0}));
        EXPECT_FALSE(Contains(box, math::vec3{2, 0, 0}));
    }

    TEST(Containment, EllipsoidContainsPoint)
    {
        const Ellipsoid ellipsoid{{0, 0, 0}, {2, 1, 1}, math::quat{1, 0, 0, 0}};
        EXPECT_TRUE(Contains(ellipsoid, math::vec3{1, 0, 0}));
        EXPECT_FALSE(Contains(ellipsoid, math::vec3{3, 0, 0}));
    }

    TEST(Containment, PlaneContainsPoint)
    {
        const Plane plane{{0, 0, 1}, 0};
        EXPECT_TRUE(Contains(plane, math::vec3{1, 1, 0}, 1e-6f));
        EXPECT_FALSE(Contains(plane, math::vec3{0, 0, 1}, 1e-6f));
    }

    TEST(Containment, PlaneContainsLine)
    {
        const Plane plane{{0, 0, 1}, 0};
        const Line on_plane{{0, 0, 0}, {1, 0, 0}};
        const Line off_plane{{0, 0, 1}, {1, 0, 0}};

        EXPECT_TRUE(Contains(plane, on_plane, 1e-6f));
        EXPECT_FALSE(Contains(plane, off_plane, 1e-6f));
    }

    TEST(Containment, TriangleContainsPoint)
    {
        const Triangle tri{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
        EXPECT_TRUE(Contains(tri, math::vec3{0.25f, 0.25f, 0}));
        EXPECT_FALSE(Contains(tri, math::vec3{2, 0, 0}));
    }

    TEST(Containment, TriangleContainsSegment)
    {
        const Triangle tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
        const Segment inside{{-0.25f, -0.25f, 0}, {0.25f, 0.25f, 0}};
        const Segment outside{{2, 0, 0}, {3, 0, 0}};

        EXPECT_TRUE(Contains(tri, inside));
        EXPECT_FALSE(Contains(tri, outside));
    }

    // ============================================================================
    // EDGE CASES AND ROBUSTNESS TESTS
    // ============================================================================

    TEST(EdgeCases, DegenerateShapes)
    {
        // Zero-size shapes
        const Sphere point_sphere{{0, 0, 0}, 0.0f};
        const Sphere normal_sphere{{1, 0, 0}, 1.0f};

        EXPECT_TRUE(Intersects(point_sphere, normal_sphere));

        // Zero-height cylinder
        const Cylinder flat_cylinder{{0, 0, 0}, {0, 0, 1}, 1.0f, 0.0f};
        const Sphere sphere{{0, 0, 0}, 0.5f};

        EXPECT_TRUE(Intersects(flat_cylinder, sphere));
    }

    TEST(EdgeCases, NearlyParallelLines)
    {
        const Line a{{0, 0, 0}, math::normalize(math::vec3{1, 0, 0.00001f})};
        const Line b{{0, 1, 0}, math::normalize(math::vec3{1, 0, 0})};

        // Should handle near-parallel case gracefully
        bool result = Intersects(a, b, nullptr);
        // Result depends on epsilon handling
    }

    TEST(EdgeCases, CoincidentShapes)
    {
        // Identical AABBs
        const Aabb box{{0, 0, 0}, {1, 1, 1}};
        EXPECT_TRUE(Intersects(box, box));

        // Identical spheres
        const Sphere sphere{{0, 0, 0}, 1.0f};
        EXPECT_TRUE(Intersects(sphere, sphere));
    }

    TEST(EdgeCases, BoundaryTouching)
    {
        // Spheres touching at a point
        const Sphere a{{0, 0, 0}, 1.0f};
        const Sphere b{{2, 0, 0}, 1.0f};
        EXPECT_FALSE(Intersects(a, b)); // Should be false for touching

        // AABBs sharing a face
        const Aabb box1{{0, 0, 0}, {1, 1, 1}};
        const Aabb box2{{1, 0, 0}, {2, 1, 1}};
        EXPECT_FALSE(Intersects(box1, box2)); // Should be false for touching
    }

    TEST(RobustnessTests, ResultParameters)
    {
        Result result;

        // Ray-Sphere intersection should set both t_min and t_max
        const Sphere sphere{{0, 0, 0}, 1.0f};
        const Ray ray{{-2, 0, 0}, {1, 0, 0}};

        ASSERT_TRUE(Intersects(ray, sphere, &result));
        EXPECT_LT(result.t_min, result.t_max);
        EXPECT_GT(result.t_min, 0); // Entry point
        EXPECT_GT(result.t_max, 0); // Exit point
    }

    TEST(RobustnessTests, OrientationIndependence)
    {
        // OBB intersection should be independent of orientation representation
        const Obb box1{{0, 0, 0}, {1, 1, 1}, math::quat{1, 0, 0, 0}};
        const Obb box2{{0.5f, 0, 0}, {0.5f, 0.5f, 0.5f}, math::quat{1, 0, 0, 0}};

        EXPECT_TRUE(Intersects(box1, box2));
        EXPECT_TRUE(Intersects(box2, box1)); // Symmetry
    }
} // namespace engine::geometry

