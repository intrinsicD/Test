#pragma once

#include "engine/math/common.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/Matrix.hpp"
#include "engine/math/quaternion.hpp"

namespace engine::math::utils
{
    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> rotation(const quaternion<T>& q) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        const T xx = q.x * q.x;
        const T yy = q.y * q.y;
        const T zz = q.z * q.z;
        const T xy = q.x * q.y;
        const T xz = q.x * q.z;
        const T yz = q.y * q.z;
        const T wx = q.w * q.x;
        const T wy = q.w * q.y;
        const T wz = q.w * q.z;

        Matrix<T, 3, 3> result{
            T(1) - T(2) * (yy + zz), T(2) * (xy + wz),       T(2) * (xz - wy),      
            T(2) * (xy - wz),       T(1) - T(2) * (xx + zz), T(2) * (yz + wx),      
            T(2) * (xz + wy),       T(2) * (yz - wx),       T(1) - T(2) * (xx + yy),
        };
        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE quaternion<T> rotation(const Matrix<T, 3, 3>& m) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> rotation(const Vector<T, 3>& angle_axis) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //The input vector is expected to be (axis.x, axis.y, axis.z) * angle
}

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> rotation(T angle, const Vector<T, 3>& axis) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //The input vector is expected to be a normalized axis of rotation
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> rotation(const Vector<T, 4>& angle_axis) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //The input vector is expected to be (angle, axis.x, axis.y, axis.z)
    }





} // namespace engine::math::utils