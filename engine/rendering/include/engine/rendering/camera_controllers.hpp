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

    class ENGINE_RENDERING_API FirstPersonCameraController final : public CameraController
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

        static constexpr float min_pitch_{-1.55334306F}; // ~-89 degrees
        static constexpr float max_pitch_{1.55334306F};  // ~89 degrees
        static constexpr float min_radius_{0.1F};

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
        pitch_ = std::clamp(pitch_radians, min_pitch_, max_pitch_);
        update_camera();
    }

    inline void OrbitCameraController::update(const CameraControlState& state, float delta_seconds) noexcept
    {
        yaw_ += state.rotation[0];
        pitch_ = std::clamp(pitch_ + state.rotation[1], min_pitch_, max_pitch_);
        radius_ = std::max(min_radius_, radius_ + state.zoom);

        const float cos_pitch = std::cos(pitch_);
        const float sin_pitch = std::sin(pitch_);
        const float cos_yaw = std::cos(yaw_);
        const float sin_yaw = std::sin(yaw_);

        const math::vec3 forward{
            cos_pitch * sin_yaw,
            sin_pitch,
            -cos_pitch * cos_yaw};
        const math::vec3 right = math::normalize(math::cross(forward, world_up()));
        const math::vec3 up = math::cross(right, forward);

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

        const math::vec3 forward{
            cos_pitch * sin_yaw,
            sin_pitch,
            -cos_pitch * cos_yaw};
        const math::vec3 right = math::normalize(math::cross(forward, world_up()));
        const math::vec3 up = math::cross(right, forward);
        const math::vec3 position = target_ - forward * radius_;

        [[maybe_unused]] auto result = camera().look_at(position, target_, up);
    }
} // namespace engine::rendering
