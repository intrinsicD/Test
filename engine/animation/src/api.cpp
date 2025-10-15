#include "engine/animation/api.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <optional>
#include <unordered_map>

namespace engine::animation {

namespace {

constexpr double epsilon_time = 1e-6;
constexpr std::size_t invalid_index = std::numeric_limits<std::size_t>::max();
constexpr float weight_min = 0.0F;
constexpr float weight_max = 1.0F;

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

[[nodiscard]] JointPose apply_additive_pose(const JointPose& base, const JointPose& additive, float weight) {
    if (weight <= weight_min) {
        return base;
    }

    JointPose result = base;
    result.translation = base.translation + additive.translation * weight;

    const auto apply_scale = [&](std::size_t axis) {
        const float base_scale = base.scale[axis];
        const float additive_scale = additive.scale[axis];
        const float delta = (additive_scale - 1.0F) * weight;
        result.scale[axis] = base_scale * (1.0F + delta);
    };
    apply_scale(0);
    apply_scale(1);
    apply_scale(2);

    const math::quat identity = math::quat::Identity();
    const math::quat additive_delta = math::normalize(additive.rotation);
    const math::quat weighted_delta = math::normalize(math::slerp(identity, additive_delta, weight));
    result.rotation = math::normalize(weighted_delta * base.rotation);

    return result;
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

[[nodiscard]] AnimationRigPose blend_additive(const AnimationRigPose& base,
                                             const AnimationRigPose& additive,
                                             float weight) {
    if (weight <= weight_min) {
        return base;
    }

    auto base_map = to_pose_map(base);
    auto additive_map = to_pose_map(additive);

    PoseMap result = base_map;

    for (auto& [joint, pose] : result) {
        if (const auto it = additive_map.find(joint); it != additive_map.end()) {
            pose = apply_additive_pose(pose, it->second, weight);
        }
    }

    for (const auto& [joint, pose] : additive_map) {
        if (result.find(joint) == result.end()) {
            result.insert({joint, apply_additive_pose(JointPose{}, pose, weight)});
        }
    }

    return to_rig_pose(std::move(result));
}

[[nodiscard]] bool node_index_valid(const AnimationBlendTree& tree, std::size_t node) noexcept {
    return node < tree.nodes.size();
}

[[nodiscard]] bool parameter_index_valid(const AnimationBlendTree& tree, std::size_t parameter) noexcept {
    return parameter < tree.parameters.size();
}

[[nodiscard]] BlendTreeParameter* parameter_at(AnimationBlendTree& tree, std::size_t parameter) noexcept {
    if (!parameter_index_valid(tree, parameter)) {
        return nullptr;
    }
    return &tree.parameters[parameter];
}

[[nodiscard]] const BlendTreeParameter* parameter_at(const AnimationBlendTree& tree, std::size_t parameter) noexcept {
    if (!parameter_index_valid(tree, parameter)) {
        return nullptr;
    }
    return &tree.parameters[parameter];
}

[[nodiscard]] std::optional<std::size_t> find_parameter_index(const AnimationBlendTree& tree,
                                                              std::string_view name) noexcept {
    const auto it = std::find_if(tree.parameters.begin(), tree.parameters.end(), [&](const auto& parameter) {
        return parameter.name == name;
    });
    if (it == tree.parameters.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(tree.parameters.begin(), it));
}

[[nodiscard]] float resolved_weight(const AnimationBlendTree& tree,
                                    float node_weight,
                                    std::size_t parameter_index) noexcept {
    float weight = node_weight;
    if (const auto* parameter = parameter_at(tree, parameter_index);
        parameter != nullptr && parameter->type == BlendTreeParameterType::kFloat) {
        weight = parameter->float_value;
    }
    if (!std::isfinite(weight)) {
        return weight_min;
    }
    return std::clamp(weight, weight_min, weight_max);
}

[[nodiscard]] float resolved_blend_weight(const AnimationBlendTree& tree,
                                          const BlendTreeLinearBlendNode& node) noexcept {
    return resolved_weight(tree, node.weight, node.weight_parameter);
}

[[nodiscard]] float resolved_additive_weight(const AnimationBlendTree& tree,
                                             const BlendTreeAdditiveNode& node) noexcept {
    return resolved_weight(tree, node.weight, node.weight_parameter);
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
    blend.weight_parameter = invalid_index;
    node.data = blend;
    tree.nodes.push_back(node);
    return tree.nodes.size() - 1U;
}

std::size_t add_additive_blend_node(AnimationBlendTree& tree,
                                    std::size_t base,
                                    std::size_t additive,
                                    float weight) {
    BlendTreeNode node;
    BlendTreeAdditiveNode blend;
    blend.base = base;
    blend.additive = additive;
    blend.weight = std::clamp(weight, weight_min, weight_max);
    blend.weight_parameter = invalid_index;
    node.data = blend;
    tree.nodes.push_back(node);
    return tree.nodes.size() - 1U;
}

std::size_t add_float_parameter(AnimationBlendTree& tree, std::string name, float initial_value) {
    if (const auto existing = find_parameter_index(tree, name)) {
        auto& parameter = tree.parameters[*existing];
        parameter.type = BlendTreeParameterType::kFloat;
        parameter.float_value = initial_value;
        parameter.bool_value = false;
        parameter.event_value = false;
        return *existing;
    }

    BlendTreeParameter parameter;
    parameter.name = std::move(name);
    parameter.type = BlendTreeParameterType::kFloat;
    parameter.float_value = initial_value;
    tree.parameters.push_back(std::move(parameter));
    return tree.parameters.size() - 1U;
}

std::size_t add_bool_parameter(AnimationBlendTree& tree, std::string name, bool initial_value) {
    if (const auto existing = find_parameter_index(tree, name)) {
        auto& parameter = tree.parameters[*existing];
        parameter.type = BlendTreeParameterType::kBool;
        parameter.bool_value = initial_value;
        parameter.float_value = initial_value ? 1.0F : 0.0F;
        parameter.event_value = false;
        return *existing;
    }

    BlendTreeParameter parameter;
    parameter.name = std::move(name);
    parameter.type = BlendTreeParameterType::kBool;
    parameter.bool_value = initial_value;
    parameter.float_value = initial_value ? 1.0F : 0.0F;
    tree.parameters.push_back(std::move(parameter));
    return tree.parameters.size() - 1U;
}

std::size_t add_event_parameter(AnimationBlendTree& tree, std::string name) {
    if (const auto existing = find_parameter_index(tree, name)) {
        auto& parameter = tree.parameters[*existing];
        parameter.type = BlendTreeParameterType::kEvent;
        parameter.event_value = false;
        parameter.float_value = 0.0F;
        parameter.bool_value = false;
        return *existing;
    }

    BlendTreeParameter parameter;
    parameter.name = std::move(name);
    parameter.type = BlendTreeParameterType::kEvent;
    tree.parameters.push_back(std::move(parameter));
    return tree.parameters.size() - 1U;
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
        blend->weight = std::clamp(weight, weight_min, weight_max);
    }
}

void set_additive_blend_weight(AnimationBlendTree& tree, std::size_t node, float weight) noexcept {
    if (!node_index_valid(tree, node)) {
        return;
    }
    auto& variant = tree.nodes[node].data;
    if (auto* blend = std::get_if<BlendTreeAdditiveNode>(&variant)) {
        blend->weight = std::clamp(weight, weight_min, weight_max);
    }
}

void bind_linear_blend_weight(AnimationBlendTree& tree, std::size_t node, std::size_t parameter) noexcept {
    if (!node_index_valid(tree, node)) {
        return;
    }
    auto& variant = tree.nodes[node].data;
    if (auto* blend = std::get_if<BlendTreeLinearBlendNode>(&variant)) {
        const auto* entry = parameter_at(tree, parameter);
        if (entry != nullptr && entry->type == BlendTreeParameterType::kFloat) {
            blend->weight_parameter = parameter;
        } else {
            blend->weight_parameter = invalid_index;
        }
    }
}

void bind_additive_blend_weight(AnimationBlendTree& tree, std::size_t node, std::size_t parameter) noexcept {
    if (!node_index_valid(tree, node)) {
        return;
    }
    auto& variant = tree.nodes[node].data;
    if (auto* blend = std::get_if<BlendTreeAdditiveNode>(&variant)) {
        const auto* entry = parameter_at(tree, parameter);
        if (entry != nullptr && entry->type == BlendTreeParameterType::kFloat) {
            blend->weight_parameter = parameter;
        } else {
            blend->weight_parameter = invalid_index;
        }
    }
}

bool set_float_parameter(AnimationBlendTree& tree, std::size_t parameter, float value) noexcept {
    if (!std::isfinite(value)) {
        return false;
    }
    if (auto* entry = parameter_at(tree, parameter); entry != nullptr && entry->type == BlendTreeParameterType::kFloat) {
        entry->float_value = value;
        return true;
    }
    return false;
}

bool set_bool_parameter(AnimationBlendTree& tree, std::size_t parameter, bool value) noexcept {
    if (auto* entry = parameter_at(tree, parameter); entry != nullptr && entry->type == BlendTreeParameterType::kBool) {
        entry->bool_value = value;
        entry->float_value = value ? 1.0F : 0.0F;
        return true;
    }
    return false;
}

bool trigger_event_parameter(AnimationBlendTree& tree, std::size_t parameter) noexcept {
    if (auto* entry = parameter_at(tree, parameter); entry != nullptr && entry->type == BlendTreeParameterType::kEvent) {
        entry->event_value = true;
        return true;
    }
    return false;
}

bool consume_event_parameter(AnimationBlendTree& tree, std::size_t parameter) noexcept {
    if (auto* entry = parameter_at(tree, parameter); entry != nullptr && entry->type == BlendTreeParameterType::kEvent) {
        const bool triggered = entry->event_value;
        entry->event_value = false;
        return triggered;
    }
    return false;
}

bool set_float_parameter(AnimationBlendTree& tree, std::string_view name, float value) noexcept {
    if (const auto index = find_parameter_index(tree, name)) {
        return set_float_parameter(tree, *index, value);
    }
    return false;
}

bool set_bool_parameter(AnimationBlendTree& tree, std::string_view name, bool value) noexcept {
    if (const auto index = find_parameter_index(tree, name)) {
        return set_bool_parameter(tree, *index, value);
    }
    return false;
}

bool trigger_event_parameter(AnimationBlendTree& tree, std::string_view name) noexcept {
    if (const auto index = find_parameter_index(tree, name)) {
        return trigger_event_parameter(tree, *index);
    }
    return false;
}

bool consume_event_parameter(AnimationBlendTree& tree, std::string_view name) noexcept {
    if (const auto index = find_parameter_index(tree, name)) {
        return consume_event_parameter(tree, *index);
    }
    return false;
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
            if (blend->weight_parameter != invalid_index) {
                const auto* entry = parameter_at(tree, blend->weight_parameter);
                if (entry == nullptr || entry->type != BlendTreeParameterType::kFloat) {
                    return false;
                }
            }
        } else if (const auto* additive = std::get_if<BlendTreeAdditiveNode>(&node.data)) {
            if (!node_index_valid(tree, additive->base) || !node_index_valid(tree, additive->additive)) {
                return false;
            }
            if (!std::isfinite(additive->weight)) {
                return false;
            }
            if (additive->weight_parameter != invalid_index) {
                const auto* entry = parameter_at(tree, additive->weight_parameter);
                if (entry == nullptr || entry->type != BlendTreeParameterType::kFloat) {
                    return false;
                }
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
        pose = blend_linear(lhs, rhs, resolved_blend_weight(tree, *blend));
    } else if (const auto* additive = std::get_if<BlendTreeAdditiveNode>(&node)) {
        const auto base = evaluate_node(tree, additive->base, cache);
        const auto delta = evaluate_node(tree, additive->additive, cache);
        pose = blend_additive(base, delta, resolved_additive_weight(tree, *additive));
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
