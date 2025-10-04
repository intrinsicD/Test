#pragma once

#include "engine/math/common.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/quaternion.hpp"

namespace engine::math::utils
{
    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> to_rotation_matrix(const Quaternion<T>& quat) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //TODO: maybe remove this function an replace with utils::rotation(quat)?
        Matrix<T, 3, 3> result;
        const T xx = quat.x * quat.x;
        const T yy = quat.y * quat.y;
        const T zz = quat.z * quat.z;
        const T xy = quat.x * quat.y;
        const T xz = quat.x * quat.z;
        const T yz = quat.y * quat.z;
        const T wx = quat.w * quat.x;
        const T wy = quat.w * quat.y;
        const T wz = quat.w * quat.z;

        result[0][0] = detail::one<T>() - static_cast<T>(2) * (yy + zz);
        result[0][1] = static_cast<T>(2) * (xy - wz);
        result[0][2] = static_cast<T>(2) * (xz + wy);

        result[1][0] = static_cast<T>(2) * (xy + wz);
        result[1][1] = detail::one<T>() - static_cast<T>(2) * (xx + zz);
        result[1][2] = static_cast<T>(2) * (yz - wx);

        result[2][0] = static_cast<T>(2) * (xz - wy);
        result[2][1] = static_cast<T>(2) * (yz + wx);
        result[2][2] = detail::one<T>() - static_cast<T>(2) * (xx + yy);

        return result;
    }

    template <typename T>
    ENGINE_MATH_INLINE Quaternion<T> to_quaternion(const Matrix<T, 3, 3>& rot) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> to_rotation_matrix(const Vector<T, 3>& angle_axis) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //The input vector is expected to be (axis.x, axis.y, axis.z) * angle
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> to_rotation_matrix(T angle, const Vector<T, 3>& axis) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //The input vector is expected to be a normalized axis of rotation
    }

    template <typename T>
    ENGINE_MATH_INLINE Matrix<T, 3, 3> to_rotation_matrix(const Vector<T, 4>& angle_axis) noexcept
    {
        //TODO: check if this is correct! Also check if there is a more efficient way to do this!
        //TODO: add tests for this function!
        //The input vector is expected to be (angle, axis.x, axis.y, axis.z)
    }





} // namespace engine::math::utils