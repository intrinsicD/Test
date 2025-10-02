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
} // namespace engine::geometry

