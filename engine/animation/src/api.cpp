#include "engine/animation/api.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>

namespace engine::animation {

namespace {

constexpr double epsilon_time = 1e-6;

using PoseMap = std::unordered_map<std::string, JointPose>;

[[nodiscard]] math::vec3 lerp(const math::vec3& a, const math::vec3& b, float t) {
    return a + (b - a) * t;
}

[[nodiscard]] PoseMap to_pose_map(const AnimationRigPose& pose) {
    PoseMap map;
    map.reserve(pose.joints.size());
    for (const auto& entry : pose.joints) {
        map.insert(entry);
    }
    return map;
}

[[nodiscard]] AnimationRigPose to_rig_pose(PoseMap map) {
    AnimationRigPose pose;
    pose.joints.reserve(map.size());
    for (auto& entry : map) {
        pose.joints.emplace_back(entry.first, entry.second);
    }
    std::sort(pose.joints.begin(), pose.joints.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
    return pose;
}

[[nodiscard]] JointPose blend_joint_pose(const JointPose& lhs, const JointPose& rhs, float weight) {
    JointPose blended;
    blended.translation = lerp(lhs.translation, rhs.translation, weight);
    blended.scale = lerp(lhs.scale, rhs.scale, weight);
    blended.rotation = math::normalize(math::slerp(lhs.rotation, rhs.rotation, weight));
    return blended;
}

[[nodiscard]] AnimationRigPose blend_linear(const AnimationRigPose& lhs,
                                            const AnimationRigPose& rhs,
                                            float weight) {
    if (weight <= 0.0F) {
        return lhs;
    }
    if (weight >= 1.0F) {
        return rhs;
    }

    auto lhs_map = to_pose_map(lhs);
    auto rhs_map = to_pose_map(rhs);
    PoseMap result;
    result.reserve(lhs_map.size() + rhs_map.size());

    const auto accumulate = [&](const PoseMap& map) {
        for (const auto& [joint, pose] : map) {
            result.insert({joint, pose});
        }
    };
    accumulate(lhs_map);
    accumulate(rhs_map);

    for (auto& [joint, pose] : result) {
        const auto lhs_it = lhs_map.find(joint);
        const auto rhs_it = rhs_map.find(joint);
        if (lhs_it != lhs_map.end() && rhs_it != rhs_map.end()) {
            pose = blend_joint_pose(lhs_it->second, rhs_it->second, weight);
        } else if (rhs_it != rhs_map.end()) {
            pose = blend_joint_pose(JointPose{}, rhs_it->second, weight);
        } else if (lhs_it != lhs_map.end()) {
            pose = blend_joint_pose(lhs_it->second, JointPose{}, weight);
        }
    }

    return to_rig_pose(std::move(result));
}

[[nodiscard]] bool node_index_valid(const AnimationBlendTree& tree, std::size_t node) noexcept {
    return node < tree.nodes.size();
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

std::size_t add_clip_node(AnimationBlendTree& tree, AnimationClip clip) {
    BlendTreeNode node;
    node.data = BlendTreeClipNode{make_linear_controller(std::move(clip))};
    tree.nodes.push_back(std::move(node));
    return tree.nodes.size() - 1U;
}

std::size_t add_controller_node(AnimationBlendTree& tree, AnimationController controller) {
    BlendTreeNode node;
    node.data = BlendTreeClipNode{std::move(controller)};
    tree.nodes.push_back(std::move(node));
    return tree.nodes.size() - 1U;
}

std::size_t add_linear_blend_node(AnimationBlendTree& tree, std::size_t lhs, std::size_t rhs, float weight) {
    BlendTreeNode node;
    BlendTreeLinearBlendNode blend;
    blend.lhs = lhs;
    blend.rhs = rhs;
    blend.weight = std::clamp(weight, 0.0F, 1.0F);
    node.data = blend;
    tree.nodes.push_back(node);
    return tree.nodes.size() - 1U;
}

void set_blend_tree_root(AnimationBlendTree& tree, std::size_t node) noexcept {
    tree.root = node;
}

void set_linear_blend_weight(AnimationBlendTree& tree, std::size_t node, float weight) noexcept {
    if (!node_index_valid(tree, node)) {
        return;
    }
    auto& variant = tree.nodes[node].data;
    if (auto* blend = std::get_if<BlendTreeLinearBlendNode>(&variant)) {
        blend->weight = std::clamp(weight, 0.0F, 1.0F);
    }
}

void advance_blend_tree(AnimationBlendTree& tree, double dt) noexcept {
    for (auto& node : tree.nodes) {
        if (auto* clip = std::get_if<BlendTreeClipNode>(&node.data)) {
            advance_controller(clip->controller, dt);
        }
    }
}

bool blend_tree_valid(const AnimationBlendTree& tree) noexcept {
    if (!node_index_valid(tree, tree.root)) {
        return false;
    }
    for (const auto& node : tree.nodes) {
        if (const auto* blend = std::get_if<BlendTreeLinearBlendNode>(&node.data)) {
            if (!node_index_valid(tree, blend->lhs) || !node_index_valid(tree, blend->rhs)) {
                return false;
            }
            if (!std::isfinite(blend->weight)) {
                return false;
            }
        }
    }
    return true;
}

namespace {

AnimationRigPose evaluate_node(const AnimationBlendTree& tree,
                               std::size_t index,
                               std::vector<std::optional<AnimationRigPose>>& cache) {
    if (!node_index_valid(tree, index)) {
        return {};
    }
    if (cache[index].has_value()) {
        return cache[index].value();
    }

    const auto& node = tree.nodes[index].data;
    AnimationRigPose pose;
    if (const auto* clip = std::get_if<BlendTreeClipNode>(&node)) {
        pose = evaluate_controller(clip->controller);
    } else if (const auto* blend = std::get_if<BlendTreeLinearBlendNode>(&node)) {
        const auto lhs = evaluate_node(tree, blend->lhs, cache);
        const auto rhs = evaluate_node(tree, blend->rhs, cache);
        pose = blend_linear(lhs, rhs, blend->weight);
    }

    cache[index] = pose;
    return pose;
}

}  // namespace

AnimationRigPose evaluate_blend_tree(const AnimationBlendTree& tree) {
    if (!blend_tree_valid(tree)) {
        return {};
    }
    std::vector<std::optional<AnimationRigPose>> cache(tree.nodes.size());
    return evaluate_node(tree, tree.root, cache);
}

}  // namespace engine::animation

extern "C" ENGINE_ANIMATION_API const char* engine_animation_module_name() noexcept {
    return engine::animation::module_name().data();
}
