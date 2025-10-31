#include <gtest/gtest.h>

#include <cstddef>

#include "engine/rendering/camera.hpp"
#include "engine/rendering/camera_controllers.hpp"

#include "engine/math/quaternion.hpp"
#include "engine/math/transform.hpp"
#include "engine/math/utils/utils_camera.hpp"

namespace
{
    void expect_matrix_near(const engine::math::mat4& lhs, const engine::math::mat4& rhs,
                            float tolerance = 1.0e-4F)
    {
        for (std::size_t row = 0; row < 4; ++row)
        {
            for (std::size_t col = 0; col < 4; ++col)
            {
                EXPECT_NEAR(lhs[row][col], rhs[row][col], tolerance);
            }
        }
    }
}

TEST(Camera, DefaultsToIdentityMatrices)
{
    engine::rendering::Camera camera;

    const auto identity = engine::math::identity_matrix<float, 4>();
    expect_matrix_near(camera.view, identity);
    expect_matrix_near(camera.model, identity);
    expect_matrix_near(camera.projection, identity);
}

TEST(Camera, SynchronizesViewWhenModelChanges)
{
    engine::rendering::Camera camera;

    engine::math::Transform<float> transform{};
    transform.translation = {0.0F, 1.0F, 5.0F};
    transform.rotation = engine::math::angle_axis(
        engine::math::utils::radians(45.0F), engine::math::vec3{0.0F, 1.0F, 0.0F});
    transform.scale = {1.0F, 2.0F, 1.0F};

    const auto model = engine::math::to_matrix(transform);
    camera.set_model(model);

    expect_matrix_near(camera.model, model);
    const auto expected_view = engine::math::try_inverse(model);
    ASSERT_TRUE(expected_view.has_value());
    expect_matrix_near(camera.view, expected_view.value());
}

TEST(Camera, LookAtUpdatesViewAndModel)
{
    engine::rendering::Camera camera;

    const engine::math::vec3 eye{0.0F, 2.0F, 5.0F};
    const engine::math::vec3 target{0.0F, 0.0F, 0.0F};
    const engine::math::vec3 up{0.0F, 1.0F, 0.0F};

    camera.look_at(eye, target, up);

    const auto expected_view = engine::math::utils::look_at(eye, target, up);
    expect_matrix_near(camera.view, expected_view);
    const auto expected_model = engine::math::try_inverse(expected_view);
    ASSERT_TRUE(expected_model.has_value());
    expect_matrix_near(camera.model, expected_model.value());
}

TEST(Camera, PerspectiveProjectionMatchesMathUtils)
{
    engine::rendering::Camera camera;

    const float fov = engine::math::utils::radians(60.0F);
    const float aspect = 16.0F / 9.0F;
    const float near_plane = 0.1F;
    const float far_plane = 100.0F;

    camera.set_perspective(fov, aspect, near_plane, far_plane);

    const auto expected_projection = engine::math::utils::perspective(fov, aspect, near_plane, far_plane);
    expect_matrix_near(camera.projection, expected_projection);
}

TEST(Camera, OrthographicProjectionMatchesMathUtils)
{
    engine::rendering::Camera camera;

    const float left = -5.0F;
    const float right = 5.0F;
    const float bottom = -3.0F;
    const float top = 3.0F;
    const float near_plane = 0.01F;
    const float far_plane = 50.0F;

    camera.set_orthographic(left, right, bottom, top, near_plane, far_plane);

    const auto expected_projection =
        engine::math::utils::orthographic(left, right, bottom, top, near_plane, far_plane);
    expect_matrix_near(camera.projection, expected_projection);
}

TEST(CameraControllers, FirstPersonMovesForward)
{
    engine::rendering::Camera camera;
    engine::rendering::FirstPersonCameraController controller(camera, {0.0F, 0.0F, 5.0F});

    engine::rendering::CameraControlState state{};
    state.translation[2] = 1.0F;

    controller.update(state, 1.0F);

    const auto position = controller.position();
    EXPECT_NEAR(position[0], 0.0F, 1.0e-4F);
    EXPECT_NEAR(position[1], 0.0F, 1.0e-4F);
    EXPECT_NEAR(position[2], 4.0F, 1.0e-4F);

    const auto expected_view = engine::math::utils::look_at(
        engine::math::vec3{0.0F, 0.0F, 4.0F}, engine::math::vec3{0.0F, 0.0F, 3.0F}, engine::math::vec3{0.0F, 1.0F, 0.0F});
    expect_matrix_near(camera.view, expected_view);
}

TEST(CameraControllers, FirstPersonAppliesYawRotation)
{
    engine::rendering::Camera camera;
    engine::rendering::FirstPersonCameraController controller(camera);

    engine::rendering::CameraControlState state{};
    state.rotation[0] = engine::math::utils::radians(90.0F);

    controller.update(state, 1.0F);

    const auto expected_view = engine::math::utils::look_at(
        engine::math::vec3{0.0F, 0.0F, 0.0F}, engine::math::vec3{1.0F, 0.0F, 0.0F}, engine::math::vec3{0.0F, 1.0F, 0.0F});
    expect_matrix_near(camera.view, expected_view);
}

TEST(CameraControllers, OrbitControllerOrbitsAndZooms)
{
    engine::rendering::Camera camera;
    engine::rendering::OrbitCameraController controller(camera, {0.0F, 0.0F, 0.0F}, 5.0F);

    // Rotate 90 degrees around the Y axis.
    engine::rendering::CameraControlState rotation_state{};
    rotation_state.rotation[0] = engine::math::utils::radians(90.0F);
    controller.update(rotation_state, 1.0F);

    auto transform = engine::math::from_matrix(camera.model);
    const auto target = controller.target();
    EXPECT_NEAR(transform.translation[1], 0.0F, 1.0e-4F);
    EXPECT_NEAR(transform.translation[2], 0.0F, 1.0e-4F);

    const engine::math::vec4 target_view = camera.view * engine::math::vec4{target[0], target[1], target[2], 1.0F};
    EXPECT_NEAR(target_view[0], 0.0F, 1.0e-4F);
    EXPECT_NEAR(target_view[1], 0.0F, 1.0e-4F);

    // Zoom in by reducing the orbit radius.
    engine::rendering::CameraControlState zoom_state{};
    zoom_state.zoom = -2.0F;
    controller.update(zoom_state, 1.0F);

    transform = engine::math::from_matrix(camera.model);
    const float expected_radius = 3.0F;
    EXPECT_NEAR(engine::math::distance(transform.translation, controller.target()), expected_radius, 1.0e-4F);
}
