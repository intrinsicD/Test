#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/math/math.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_ANIMATION_EXPORTS)
#    define ENGINE_ANIMATION_API __declspec(dllexport)
#  else
#    define ENGINE_ANIMATION_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_ANIMATION_API
#endif

namespace engine::animation {

struct JointPose {
    math::vec3 translation{0.0F, 0.0F, 0.0F};
    math::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
    math::vec3 scale{1.0F, 1.0F, 1.0F};
};

struct Keyframe {
    double time{0.0};
    JointPose pose{};
};

struct JointTrack {
    std::string joint_name;
    std::vector<Keyframe> keyframes;
};

struct AnimationClip {
    std::string name;
    double duration{0.0};
    std::vector<JointTrack> tracks;
};

struct AnimationController {
    AnimationClip clip{};
    double playback_time{0.0};
    double playback_speed{1.0};
    bool looping{true};
};

struct AnimationRigPose {
    std::vector<std::pair<std::string, JointPose>> joints;

    [[nodiscard]] const JointPose* find(std::string_view joint) const noexcept;
    [[nodiscard]] JointPose* find(std::string_view joint) noexcept;
};

[[nodiscard]] ENGINE_ANIMATION_API std::string_view module_name() noexcept;

ENGINE_ANIMATION_API void sort_keyframes(JointTrack& track);

[[nodiscard]] ENGINE_ANIMATION_API JointPose sample_track(const JointTrack& track, double time);

[[nodiscard]] ENGINE_ANIMATION_API JointPose sample_clip(
    const AnimationClip& clip,
    std::string_view joint,
    double time);

ENGINE_ANIMATION_API void advance_controller(AnimationController& controller, double dt) noexcept;

[[nodiscard]] ENGINE_ANIMATION_API AnimationRigPose evaluate_controller(const AnimationController& controller);

[[nodiscard]] ENGINE_ANIMATION_API AnimationController make_linear_controller(AnimationClip clip);

[[nodiscard]] ENGINE_ANIMATION_API AnimationClip make_default_clip();

}  // namespace engine::animation

extern "C" ENGINE_ANIMATION_API const char* engine_animation_module_name() noexcept;
