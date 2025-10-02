#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/shapes.hpp"

namespace engine::geometry {
    bool Intersects(const Aabb &a, const Aabb &b) noexcept {
        for (std::size_t i = 0; i < 3; ++i) {
            if (a.max[i] < b.min[i] || a.min[i] > b.max[i]) {
                return false;
            }
        }
        return true;
    }

    bool Intersects(const Aabb &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &a, const Line &b, Result<Aabb, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Aabb &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &a, const Ray &b, Result<Aabb, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Aabb &a, const Segment &b, Result<Aabb, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Aabb &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &a, const Triangle &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Aabb &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Line &b, Result<Cylinder, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Cylinder &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Ray &b, Result<Cylinder, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Cylinder &a, const Segment &b, Result<Cylinder, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Cylinder &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Cylinder &a, const Triangle &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Aabb &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Line &b, Result<Ellipsoid, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ellipsoid &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Ray &b, Result<Ellipsoid, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ellipsoid &a, const Segment &b, Result<Ellipsoid, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ellipsoid &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Ellipsoid &a, const Triangle &b) noexcept {
        //TODO
    }

    bool Intersects(const Line &a, const Aabb &b, Result<Aabb, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Cylinder &b, Result<Cylinder, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Ellipsoid &b, Result<Ellipsoid, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Line &b, Result<Line, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Obb &b, Result<Obb, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Plane &b, Result<Plane, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Ray &b, Result<Line, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Segment &b, Result<Line, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Sphere &b, Result<Sphere, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Line &a, const Triangle &b, Result<Triangle, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Obb &a, const Aabb &b) noexcept {
        //TODO
    }

    bool Intersects(const Obb &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Obb &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Obb &a, const Line &b, Result<Obb, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Obb &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Obb &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Obb &a, const Ray &b, Result<Obb, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Obb &a, const Segment &b, Result<Obb, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Obb &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Obb &a, const Triangle &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Aabb &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Line &b, Result<Plane, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Plane &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Ray &b, Result<Plane, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Plane &a, const Segment &b, Result<Plane, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Plane &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Plane &a, const Triangle &b) noexcept {
        //TODO
    }

    bool Intersects(const Ray &a, const Aabb &b, Result<Ray, Aabb> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Cylinder &b, Result<Ray, Cylinder> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Ellipsoid &b, Result<Ray, Ellipsoid> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Line &b, Result<Ray, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Obb &b, Result<Ray, Obb> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Plane &b, Result<Ray, Plane> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Ray &b, Result<Ray, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Segment &b, Result<Ray, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Sphere &b, Result<Ray, Sphere> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Ray &a, const Triangle &b, Result<Ray, Triangle> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Aabb &b, Result<Segment, Aabb> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Cylinder &b, Result<Segment, Cylinder> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Ellipsoid &b, Result<Segment, Ellipsoid> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Line &b, Result<Segment, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Obb &b, Result<Segment, Obb> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Plane &b, Result<Segment, Plane> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Ray &b, Result<Segment, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Segment &b, Result<Segment, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Sphere &b, Result<Segment, Sphere> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Segment &a, const Triangle &b, Result<Segment, Triangle> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Sphere &a, const Aabb &b) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &a, const Line &b, Result<Sphere, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Sphere &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &a, const Ray &b, Result<Sphere, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Sphere &a, const Segment &b, Result<Sphere, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Sphere &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &a, const Triangle &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Aabb &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Cylinder &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Ellipsoid &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Line &b, Result<Triangle, Line> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Triangle &a, const Obb &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Plane &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Ray &b, Result<Triangle, Ray> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Triangle &a, const Segment &b, Result<Triangle, Segment> *result) noexcept {
        //TODO
        if (result) {
            //set result values
        }
    }

    bool Intersects(const Triangle &a, const Sphere &b) noexcept {
        //TODO
    }

    bool Intersects(const Triangle &a, const Triangle &b) noexcept {
        //TODO
    }

    //------------------------------------------------------------------------------------------------------------------

    bool Contains(const Aabb &outer, const math::vec3 &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Aabb &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Cylinder &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Ellipsoid &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Obb &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Segment &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Sphere &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Triangle &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const math::vec3 &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Aabb &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Cylinder &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Ellipsoid &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Obb &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Segment &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Sphere &inner) noexcept {
        //TODO
    }

    bool Contains(const Cylinder &outer, const Triangle &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const math::vec3 &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Aabb &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Cylinder &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Ellipsoid &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Obb &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Segment &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Sphere &inner) noexcept {
        //TODO
    }

    bool Contains(const Ellipsoid &outer, const Triangle &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const math::vec3 &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Aabb &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Cylinder &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Ellipsoid &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Obb &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Segment &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Sphere &inner) noexcept {
        //TODO
    }

    bool Contains(const Obb &outer, const Triangle &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const math::vec3 &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Aabb &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Cylinder &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Ellipsoid &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Obb &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Segment &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Sphere &inner) noexcept {
        //TODO
    }

    bool Contains(const Sphere &outer, const Triangle &inner) noexcept {
        //TODO
    }
} // namespace engine::geometry
