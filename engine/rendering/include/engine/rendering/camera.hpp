#pragma once

#include "engine/rendering/api.hpp"

#include "engine/math/matrix.hpp"
#include "engine/math/transform.hpp"
#include "engine/math/utils/utils_camera.hpp"

namespace engine::rendering
{
    /// Camera description exposing model, view, and projection matrices.
    struct ENGINE_RENDERING_API Camera
    {
        math::mat4 view{math::identity_matrix<float, 4>()};
        math::mat4 model{math::identity_matrix<float, 4>()};
        math::mat4 projection{math::identity_matrix<float, 4>()};

        [[nodiscard]] math::mat4 view_projection() const noexcept
        {
            return projection * view;
        }

        void set_view(const math::mat4& value) noexcept
        {
            view = value;
            if (auto inverse = math::try_inverse(view))
            {
                model = *inverse;
            }
            else
            {
                model = math::identity_matrix<float, 4>();
            }
        }

        void set_model(const math::mat4& value) noexcept
        {
            model = value;
            if (auto inverse = math::try_inverse(model))
            {
                view = *inverse;
            }
            else
            {
                view = math::identity_matrix<float, 4>();
            }
        }

        void set_projection(const math::mat4& value) noexcept
        {
            projection = value;
        }

        void set_transform(const math::Transform<float>& transform) noexcept
        {
            set_model(math::to_matrix(transform));
        }

        [[nodiscard]] math::Transform<float> transform() const noexcept
        {
            return math::from_matrix(model);
        }

        void look_at(const math::vec3& eye, const math::vec3& target, const math::vec3& up) noexcept
        {
            set_view(math::utils::look_at(eye, target, up));
        }

        void set_perspective(float fov_y_radians, float aspect_ratio, float near_plane, float far_plane) noexcept
        {
            set_projection(math::utils::perspective(fov_y_radians, aspect_ratio, near_plane, far_plane));
        }

        void set_orthographic(float left, float right, float bottom, float top, float near_plane,
                              float far_plane) noexcept
        {
            set_projection(math::utils::orthographic(left, right, bottom, top, near_plane, far_plane));
        }
    };
} // namespace engine::rendering
