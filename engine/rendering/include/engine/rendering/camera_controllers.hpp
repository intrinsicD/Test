#pragma once

#include "engine/rendering/api.hpp"
#include "engine/rendering/camera.hpp"

#include "engine/math/vector.hpp"
#include "engine/platform/input/input_state.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace engine::rendering
{
    struct CameraControlState
    {
        math::vec3 translation{0.0F, 0.0F, 0.0F};
        math::vec2 rotation{0.0F, 0.0F};
        float zoom{0.0F};
    };

    class ENGINE_RENDERING_API CameraController
    {
    public:
        explicit CameraController(Camera& camera) noexcept : camera_(camera) { }
        virtual ~CameraController() = default;

        [[nodiscard]] Camera& camera() noexcept { return camera_.get(); }
        [[nodiscard]] const Camera& camera() const noexcept { return camera_.get(); }

        virtual void update(const CameraControlState& state, float delta_seconds) noexcept = 0;

    protected:
        [[nodiscard]] math::vec3 camera_position() const noexcept
        {
            return math::from_matrix(camera_.get().model).translation;
        }

        void set_camera_position(const math::vec3& position) const noexcept
        {
            auto transform = camera_.get().transform();
            transform.translation = position;
            [[maybe_unused]] auto result = camera_.get().set_transform(transform);
        }

        [[nodiscard]] static math::vec3 world_up() noexcept { return math::vec3{0.0F, 1.0F, 0.0F}; }

    private:
        std::reference_wrapper<Camera> camera_;
    };

    class ENGINE_RENDERING_API FirstPersonCameraController : public CameraController
    {
    public:
        explicit FirstPersonCameraController(Camera& camera, math::vec3 position = {0.0F, 0.0F, 0.0F}) noexcept;

        void set_position(const math::vec3& position) noexcept;
        [[nodiscard]] math::vec3 position() const noexcept { return position_; }

        void set_orientation(float yaw_radians, float pitch_radians) noexcept;

        void update(const CameraControlState& state, float delta_seconds) noexcept override;

    private:
        math::vec3 position_;
        float yaw_{0.0F};
        float pitch_{0.0F};

        static constexpr float min_pitch_{-1.55334306F}; // ~-89 degrees
        static constexpr float max_pitch_{1.55334306F};  // ~89 degrees

        void update_camera() noexcept;
    };

    class ENGINE_RENDERING_API WasdCameraController final : public FirstPersonCameraController
    {
    public:
        WasdCameraController(Camera& camera, platform::input::InputState& input_state,
            math::vec3 position = {0.0F, 0.0F, 0.0F}) noexcept;

        void set_move_speed(float units_per_second) noexcept;
        [[nodiscard]] float move_speed() const noexcept { return move_speed_; }

        void update(const CameraControlState& state, float delta_seconds) noexcept override;

    private:
        [[nodiscard]] math::vec3 resolve_wasd_direction() const noexcept;

        std::reference_wrapper<platform::input::InputState> input_state_;
        float move_speed_{5.0F};
    };

    class ENGINE_RENDERING_API OrbitCameraController final : public CameraController
    {
    public:
        OrbitCameraController(Camera& camera, math::vec3 target, float radius) noexcept;

        void set_target(const math::vec3& target) noexcept;
        [[nodiscard]] math::vec3 target() const noexcept { return target_; }

        void set_radius(float radius) noexcept;
        [[nodiscard]] float radius() const noexcept { return radius_; }

        void set_orientation(float yaw_radians, float pitch_radians) noexcept;

        void update(const CameraControlState& state, float delta_seconds) noexcept override;

    private:
        math::vec3 target_;
        float radius_;
        float yaw_{0.0F};
        float pitch_{0.0F};

        static constexpr float min_radius_{0.1F};

        [[nodiscard]] static math::vec3 compute_orbit_right(const math::vec3& forward) noexcept;

        void update_camera() noexcept;
    };

    /// Trackball/Arcball camera controller using quaternion rotation for intuitive manipulation.
    /// Maps screen space mouse movements directly to sphere rotations (like PMP library).
    class ENGINE_RENDERING_API TrackballCameraController final : public CameraController
    {
    public:
        TrackballCameraController(Camera& camera, math::vec3 center, float distance) noexcept;

        void set_center(const math::vec3& center) noexcept;
        [[nodiscard]] math::vec3 center() const noexcept { return center_; }

        void set_distance(float distance) noexcept;
        [[nodiscard]] float distance() const noexcept { return distance_; }

        /// Rotate trackball based on normalized screen coordinates delta
        // Rotate from previous normalized screen point to current (both in trackball coords)
        void rotate_from(const math::vec2& prev, const math::vec2& curr) noexcept;

        // Backwards-compatible wrapper that treats delta as movement from origin
        void rotate(const math::vec2& delta) noexcept;

        /// Zoom by changing distance from center
        void zoom(float delta) noexcept;

        /// Reset to initial orientation
        void reset() noexcept;

        void update(const CameraControlState& state, float delta_seconds) noexcept override;

    private:
        math::vec3 center_;
        float distance_;
        math::Quaternion<float> rotation_{1.0f, 0.0f, 0.0f, 0.0f}; // Identity quaternion

        static constexpr float min_distance_{0.1F};
        static constexpr float trackball_size_{0.8F}; // Virtual trackball radius

        /// Project 2D screen point onto virtual sphere surface
        [[nodiscard]] math::vec3 project_onto_sphere(const math::vec2& point) const noexcept;

        void update_camera() noexcept;
    };

    inline FirstPersonCameraController::FirstPersonCameraController(Camera& camera, math::vec3 position) noexcept
        : CameraController(camera), position_(position)
    {
        update_camera();
    }

    inline void FirstPersonCameraController::set_position(const math::vec3& position) noexcept
    {
        position_ = position;
        update_camera();
    }

    inline void FirstPersonCameraController::set_orientation(float yaw_radians, float pitch_radians) noexcept
    {
        yaw_ = yaw_radians;
        pitch_ = std::clamp(pitch_radians, min_pitch_, max_pitch_);
        update_camera();
    }

    inline void FirstPersonCameraController::update(const CameraControlState& state, float delta_seconds) noexcept
    {
        const float clamped_pitch = std::clamp(pitch_ + state.rotation[1], min_pitch_, max_pitch_);
        yaw_ += state.rotation[0];
        pitch_ = clamped_pitch;

        const float cos_pitch = std::cos(pitch_);
        const float sin_pitch = std::sin(pitch_);
        const float cos_yaw = std::cos(yaw_);
        const float sin_yaw = std::sin(yaw_);

        const math::vec3 forward{
            cos_pitch * sin_yaw,
            sin_pitch,
            -cos_pitch * cos_yaw};
        const math::vec3 right = math::normalize(math::cross(forward, world_up()));

        position_ += right * state.translation[0] * delta_seconds;
        position_ += world_up() * state.translation[1] * delta_seconds;
        position_ += forward * state.translation[2] * delta_seconds;

        update_camera();
    }

    inline void FirstPersonCameraController::update_camera() noexcept
    {
        const float cos_pitch = std::cos(pitch_);
        const float sin_pitch = std::sin(pitch_);
        const float cos_yaw = std::cos(yaw_);
        const float sin_yaw = std::sin(yaw_);

        const math::vec3 forward{
            cos_pitch * sin_yaw,
            sin_pitch,
            -cos_pitch * cos_yaw};
        const math::vec3 target = position_ + forward;
        [[maybe_unused]] auto result = camera().look_at(position_, target, world_up());
    }

    inline WasdCameraController::WasdCameraController(
        Camera& camera, platform::input::InputState& input_state, math::vec3 position) noexcept
        : FirstPersonCameraController(camera, position), input_state_(input_state)
    {
    }

    inline void WasdCameraController::set_move_speed(float units_per_second) noexcept
    {
        move_speed_ = std::max(units_per_second, 0.0F);
    }

    inline void WasdCameraController::update(
        const CameraControlState& state, float delta_seconds) noexcept
    {
        CameraControlState combined_state = state;
        const math::vec3 direction = resolve_wasd_direction();
        combined_state.translation += direction * move_speed_;
        FirstPersonCameraController::update(combined_state, delta_seconds);
    }

    inline math::vec3 WasdCameraController::resolve_wasd_direction() const noexcept
    {
        math::vec3 direction{0.0F, 0.0F, 0.0F};
        auto& input_state = input_state_.get();

        if (input_state.is_key_down(platform::input::Key::W))
        {
            direction[2] += 1.0F;
        }
        if (input_state.is_key_down(platform::input::Key::S))
        {
            direction[2] -= 1.0F;
        }
        if (input_state.is_key_down(platform::input::Key::D))
        {
            direction[0] += 1.0F;
        }
        if (input_state.is_key_down(platform::input::Key::A))
        {
            direction[0] -= 1.0F;
        }

        const float length_sq = math::length_squared(direction);
        if (length_sq <= std::numeric_limits<float>::epsilon())
        {
            return math::vec3{0.0F, 0.0F, 0.0F};
        }

        const float inv_length = 1.0F / std::sqrt(length_sq);
        return direction * inv_length;
    }

    inline OrbitCameraController::OrbitCameraController(Camera& camera, math::vec3 target, float radius) noexcept
        : CameraController(camera), target_(target), radius_(std::max(radius, min_radius_))
    {
        update_camera();
    }

    inline void OrbitCameraController::set_target(const math::vec3& target) noexcept
    {
        target_ = target;
        update_camera();
    }

    inline void OrbitCameraController::set_radius(float radius) noexcept
    {
        radius_ = std::max(radius, min_radius_);
        update_camera();
    }

    inline void OrbitCameraController::set_orientation(float yaw_radians, float pitch_radians) noexcept
    {
        yaw_ = yaw_radians;
        pitch_ = pitch_radians;
        update_camera();
    }

    inline void OrbitCameraController::update(const CameraControlState& state, float delta_seconds) noexcept
    {
        yaw_ += state.rotation[0];
        pitch_ += state.rotation[1];  // No clamping - allow continuous rotation
        radius_ = std::max(min_radius_, radius_ + state.zoom);

        const float cos_pitch = std::cos(pitch_);
        const float sin_pitch = std::sin(pitch_);
        const float cos_yaw = std::cos(yaw_);
        const float sin_yaw = std::sin(yaw_);

        const math::vec3 forward{
            cos_pitch * sin_yaw,
            sin_pitch,
            -cos_pitch * cos_yaw};
        const math::vec3 right = compute_orbit_right(forward);
        const math::vec3 up = math::normalize(math::cross(right, forward));

        target_ += right * state.translation[0] * delta_seconds;
        target_ += up * state.translation[1] * delta_seconds;
        target_ += forward * state.translation[2] * delta_seconds;

        update_camera();
    }

    inline void OrbitCameraController::update_camera() noexcept
    {
        const float cos_pitch = std::cos(pitch_);
        const float sin_pitch = std::sin(pitch_);
        const float cos_yaw = std::cos(yaw_);
        const float sin_yaw = std::sin(yaw_);

        // Calculate camera position using spherical coordinates
        const math::vec3 forward{
            cos_pitch * sin_yaw,
            sin_pitch,
            -cos_pitch * cos_yaw};

        const math::vec3 position = target_ - forward * radius_;

        // Compute proper up vector to avoid gimbal lock issues
        // When looking straight up or down, use the yaw direction as reference
        const math::vec3 right = compute_orbit_right(forward);
        const math::vec3 up = math::normalize(math::cross(right, forward));

        [[maybe_unused]] auto result = camera().look_at(position, target_, up);
    }

    inline math::vec3 OrbitCameraController::compute_orbit_right(const math::vec3& forward) noexcept
    {
        constexpr float kMinLengthSq = 1.0e-6F;
        math::vec3 right = math::cross(forward, world_up());
        float length_sq = math::length_squared(right);

        if (length_sq <= kMinLengthSq)
        {
            const math::vec3 fallback_reference{0.0F, 0.0F, 1.0F};
            right = math::cross(forward, fallback_reference);
            length_sq = math::length_squared(right);

            if (length_sq <= kMinLengthSq)
            {
                return math::vec3{1.0F, 0.0F, 0.0F};
            }
        }

        const float inv_length = 1.0F / std::sqrt(length_sq);
        return right * inv_length;
    }

    // TrackballCameraController implementation

    inline TrackballCameraController::TrackballCameraController(Camera& camera, math::vec3 center,
                                                               float distance) noexcept
        : CameraController(camera), center_(center), distance_(distance)
    {
        update_camera();
    }

    inline void TrackballCameraController::set_center(const math::vec3& center) noexcept
    {
        center_ = center;
        update_camera();
    }

    inline void TrackballCameraController::set_distance(float distance) noexcept
    {
        distance_ = std::max(min_distance_, distance);
        update_camera();
    }

    inline void TrackballCameraController::rotate(const math::vec2& delta) noexcept
    {
        // Treat delta as movement from origin to delta (legacy behavior)
        rotate_from(math::vec2{0.0f, 0.0f}, delta);
    }

    inline void TrackballCameraController::rotate_from(const math::vec2& prev, const math::vec2& curr) noexcept
    {
        // Project both points onto virtual sphere
        const math::vec3 v1 = project_onto_sphere(prev);
        const math::vec3 v2 = project_onto_sphere(curr);

        // Compute rotation axis and angle robustly
        const math::vec3 axis = math::cross(v1, v2);
        const float axis_len = math::length(axis);

        if (axis_len < 1e-8f)
        {
            // Vectors nearly parallel - fallback to small rotation from dot
            const float d = std::clamp(math::dot(v1, v2), -1.0f, 1.0f);
            const float angle = std::acos(d);
            if (angle < 1e-8f)
            {
                return;
            }
            const math::vec3 fallback_axis = math::normalize(math::cross(v1, math::vec3{1.0f, 0.0f, 0.0f}));
            const math::Quaternion<float> delta_q = math::from_angle_axis(angle, fallback_axis);
            rotation_ = math::normalize(delta_q * rotation_);
            update_camera();
            return;
        }

        const float dot = std::clamp(math::dot(v1, v2), -1.0f, 1.0f);
        const float angle = std::atan2(axis_len, dot);

        const math::Quaternion<float> delta_rotation = math::from_angle_axis(angle, axis / axis_len);
        rotation_ = math::normalize(delta_rotation * rotation_);

        update_camera();
    }

    inline void TrackballCameraController::zoom(float delta) noexcept
    {
        distance_ = std::max(min_distance_, distance_ + delta);
        update_camera();
    }

    inline void TrackballCameraController::reset() noexcept
    {
        rotation_ = math::Quaternion<float>{1.0f, 0.0f, 0.0f, 0.0f};
        update_camera();
    }

    inline void TrackballCameraController::update(const CameraControlState& state, float delta_seconds) noexcept
    {
        // Apply rotation from control state
        if (state.rotation[0] != 0.0f || state.rotation[1] != 0.0f)
        {
            const math::vec2 delta{state.rotation[0], state.rotation[1]};
            rotate(delta);
        }

        // Apply zoom
        if (state.zoom != 0.0f)
        {
            zoom(state.zoom);
        }

        // Apply translation (pan the center point)
        if (state.translation[0] != 0.0f || state.translation[1] != 0.0f || state.translation[2] != 0.0f)
        {
            // Get current camera basis vectors
            const math::mat4 rotation_matrix = math::utils::to_rotation_matrix(rotation_);
            const math::vec3 right{rotation_matrix[0][0], rotation_matrix[1][0], rotation_matrix[2][0]};
            const math::vec3 up{rotation_matrix[0][1], rotation_matrix[1][1], rotation_matrix[2][1]};
            const math::vec3 forward{rotation_matrix[0][2], rotation_matrix[1][2], rotation_matrix[2][2]};

            center_ += right * state.translation[0] * delta_seconds;
            center_ += up * state.translation[1] * delta_seconds;
            center_ += forward * state.translation[2] * delta_seconds;

            update_camera();
        }
    }

    inline math::vec3 TrackballCameraController::project_onto_sphere(const math::vec2& point) const noexcept
    {
        const float r = trackball_size_;
        const float d = math::length(point);

        if (d < r * 0.70710678118654752440f) // Inside sphere (sqrt(2)/2 * r)
        {
            // Project onto sphere
            const float z = std::sqrt(r * r - d * d);
            return math::normalize(math::vec3{point[0], point[1], z});
        }
        else
        {
            // Outside sphere - project onto hyperbolic sheet
            const float t = r / 1.41421356237309504880f; // r / sqrt(2)
            const float z = t * t / d;
            return math::normalize(math::vec3{point[0], point[1], z});
        }
    }

    inline void TrackballCameraController::update_camera() noexcept
    {
        // Convert quaternion to rotation matrix
        const math::mat4 rotation_matrix = math::utils::to_rotation_matrix(rotation_);

        // Camera looks down negative Z axis in its local space
        // Extract the forward direction (negative Z column of rotation matrix)
        const math::vec3 forward{-rotation_matrix[0][2], -rotation_matrix[1][2], -rotation_matrix[2][2]};

        // Camera position is center + distance along the forward direction
        const math::vec3 position = center_ - forward * distance_;

        // Extract up vector (Y column of rotation matrix)
        const math::vec3 up{rotation_matrix[0][1], rotation_matrix[1][1], rotation_matrix[2][1]};

        [[maybe_unused]] auto result = camera().look_at(position, center_, up);
    }
} // namespace engine::rendering
