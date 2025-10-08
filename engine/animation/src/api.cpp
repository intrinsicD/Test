#include "engine/animation/api.hpp"

#include <algorithm>
#include <cmath>

namespace engine::animation {

namespace {

constexpr double epsilon_time = 1e-6;

[[nodiscard]] math::vec3 lerp(const math::vec3& a, const math::vec3& b, float t) {
    return a + (b - a) * t;
}

}  // namespace

const JointPose* AnimationRigPose::find(std::string_view joint) const noexcept {
    const auto it = std::find_if(joints.begin(), joints.end(), [&](const auto& entry) {
        return entry.first == joint;
    });
    return it != joints.end() ? &it->second : nullptr;
}

JointPose* AnimationRigPose::find(std::string_view joint) noexcept {
    const auto it = std::find_if(joints.begin(), joints.end(), [&](const auto& entry) {
        return entry.first == joint;
    });
    return it != joints.end() ? &it->second : nullptr;
}

std::string_view module_name() noexcept {
    return "animation";
}

void sort_keyframes(JointTrack& track) {
    std::stable_sort(track.keyframes.begin(), track.keyframes.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.time < rhs.time;
    });

    auto last_unique = std::unique(track.keyframes.begin(), track.keyframes.end(), [](const auto& lhs, const auto& rhs) {
        return std::abs(lhs.time - rhs.time) <= epsilon_time;
    });
    track.keyframes.erase(last_unique, track.keyframes.end());
}

JointPose sample_track(const JointTrack& track, double time) {
    if (track.keyframes.empty()) {
        return {};
    }

    if (track.keyframes.size() == 1) {
        return track.keyframes.front().pose;
    }

    const double end_time = track.keyframes.back().time;
    if (end_time <= epsilon_time) {
        return track.keyframes.back().pose;
    }

    const double wrapped = [&]() {
        const double span = end_time;
        double t = std::fmod(time, span);
        if (t < 0.0) {
            t += span;
        }
        return t;
    }();

    for (std::size_t index = 0; index + 1 < track.keyframes.size(); ++index) {
        const auto& lhs = track.keyframes[index];
        const auto& rhs = track.keyframes[index + 1];
        if (wrapped < rhs.time || index + 2 == track.keyframes.size()) {
            const double segment = std::max(rhs.time - lhs.time, epsilon_time);
            const double alpha = std::clamp((wrapped - lhs.time) / segment, 0.0, 1.0);
            const float t = static_cast<float>(alpha);
            JointPose result;
            result.translation = lerp(lhs.pose.translation, rhs.pose.translation, t);
            result.scale = lerp(lhs.pose.scale, rhs.pose.scale, t);
            result.rotation = math::normalize(math::slerp(lhs.pose.rotation, rhs.pose.rotation, t));
            return result;
        }
    }

    return track.keyframes.back().pose;
}

JointPose sample_clip(const AnimationClip& clip, std::string_view joint, double time) {
    const auto it = std::find_if(clip.tracks.begin(), clip.tracks.end(), [&](const auto& track) {
        return track.joint_name == joint;
    });
    if (it == clip.tracks.end()) {
        return {};
    }
    return sample_track(*it, time);
}

void advance_controller(AnimationController& controller, double dt) noexcept {
    if (controller.clip.duration <= epsilon_time) {
        return;
    }

    controller.playback_time += dt * controller.playback_speed;

    if (controller.looping) {
        controller.playback_time = std::fmod(controller.playback_time, controller.clip.duration);
        if (controller.playback_time < 0.0) {
            controller.playback_time += controller.clip.duration;
        }
    } else {
        controller.playback_time = std::clamp(controller.playback_time, 0.0, controller.clip.duration);
    }
}

AnimationRigPose evaluate_controller(const AnimationController& controller) {
    AnimationRigPose pose;
    pose.joints.reserve(controller.clip.tracks.size());
    for (const auto& track : controller.clip.tracks) {
        pose.joints.emplace_back(track.joint_name, sample_track(track, controller.playback_time));
    }
    return pose;
}

AnimationController make_linear_controller(AnimationClip clip) {
    double max_time = clip.duration;
    for (auto& track : clip.tracks) {
        sort_keyframes(track);
        if (!track.keyframes.empty()) {
            max_time = std::max(max_time, track.keyframes.back().time);
        }
    }
    clip.duration = std::max(max_time, epsilon_time);

    AnimationController controller;
    controller.clip = std::move(clip);
    controller.playback_time = 0.0;
    controller.playback_speed = 1.0;
    controller.looping = true;
    return controller;
}

AnimationClip make_default_clip() {
    AnimationClip clip;
    clip.name = "runtime.rig.oscillator";
    clip.duration = 1.0;

    JointTrack root_track;
    root_track.joint_name = "root";
    root_track.keyframes = {
        Keyframe{0.0, JointPose{math::vec3{0.0F, 0.0F, 0.0F}, math::quat{1.0F, 0.0F, 0.0F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}},
        Keyframe{0.5, JointPose{math::vec3{0.0F, 0.5F, 0.0F}, math::quat{1.0F, 0.0F, 0.0F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}},
        Keyframe{1.0, JointPose{math::vec3{0.0F, 0.0F, 0.0F}, math::quat{1.0F, 0.0F, 0.0F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}},
    };

    sort_keyframes(root_track);
    clip.tracks.push_back(std::move(root_track));

    return clip;
}

}  // namespace engine::animation

extern "C" ENGINE_ANIMATION_API const char* engine_animation_module_name() noexcept {
    return engine::animation::module_name().data();
}
