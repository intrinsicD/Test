#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "engine/math/math.hpp"
#include "rigging/rig_binding.hpp"

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

struct BlendTreeClipNode {
    AnimationController controller{};
};

enum class BlendTreeParameterType {
    kFloat,
    kBool,
    kEvent,
};

struct BlendTreeParameter {
    std::string name;
    BlendTreeParameterType type{BlendTreeParameterType::kFloat};
    float float_value{0.0F};
    bool bool_value{false};
    bool event_value{false};
};

struct BlendTreeLinearBlendNode {
    std::size_t lhs{std::numeric_limits<std::size_t>::max()};
    std::size_t rhs{std::numeric_limits<std::size_t>::max()};
    float weight{0.5F};
    std::size_t weight_parameter{std::numeric_limits<std::size_t>::max()};
};

struct BlendTreeAdditiveNode {
    std::size_t base{std::numeric_limits<std::size_t>::max()};
    std::size_t additive{std::numeric_limits<std::size_t>::max()};
    float weight{1.0F};
    std::size_t weight_parameter{std::numeric_limits<std::size_t>::max()};
};

struct BlendTreeNode {
    std::variant<BlendTreeClipNode, BlendTreeLinearBlendNode, BlendTreeAdditiveNode> data;
};

struct AnimationBlendTree {
    std::vector<BlendTreeNode> nodes;
    std::size_t root{std::numeric_limits<std::size_t>::max()};
    std::vector<BlendTreeParameter> parameters;
};

[[nodiscard]] ENGINE_ANIMATION_API std::string_view module_name() noexcept;

ENGINE_ANIMATION_API void sort_keyframes(JointTrack& track);

struct ClipValidationError {
    std::string message;
    std::string joint_name;
    std::size_t track_index{std::numeric_limits<std::size_t>::max()};
    std::size_t keyframe_index{std::numeric_limits<std::size_t>::max()};
};

[[nodiscard]] ENGINE_ANIMATION_API std::vector<ClipValidationError> validate_clip(const AnimationClip& clip);

ENGINE_ANIMATION_API void write_clip_json(const AnimationClip& clip,
                                          std::ostream& stream,
                                          bool pretty = true);

[[nodiscard]] ENGINE_ANIMATION_API AnimationClip read_clip_json(std::istream& stream);

ENGINE_ANIMATION_API void save_clip_json(const AnimationClip& clip,
                                         const std::filesystem::path& path,
                                         bool pretty = true);

[[nodiscard]] ENGINE_ANIMATION_API AnimationClip load_clip_json(const std::filesystem::path& path);

[[nodiscard]] ENGINE_ANIMATION_API JointPose sample_track(const JointTrack& track, double time);

[[nodiscard]] ENGINE_ANIMATION_API JointPose sample_clip(
    const AnimationClip& clip,
    std::string_view joint,
    double time);

ENGINE_ANIMATION_API void advance_controller(AnimationController& controller, double dt) noexcept;

[[nodiscard]] ENGINE_ANIMATION_API AnimationRigPose evaluate_controller(const AnimationController& controller);

[[nodiscard]] ENGINE_ANIMATION_API AnimationController make_linear_controller(AnimationClip clip);

[[nodiscard]] ENGINE_ANIMATION_API AnimationClip make_default_clip();

[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_clip_node(AnimationBlendTree& tree, AnimationClip clip);

[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_controller_node(AnimationBlendTree& tree,
                                                                   AnimationController controller);

[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_linear_blend_node(AnimationBlendTree& tree,
                                                                     std::size_t lhs,
                                                                     std::size_t rhs,
                                                                     float weight);
[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_additive_blend_node(AnimationBlendTree& tree,
                                                                      std::size_t base,
                                                                      std::size_t additive,
                                                                      float weight);
[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_float_parameter(AnimationBlendTree& tree,
                                                                  std::string name,
                                                                  float initial_value = 0.0F);
[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_bool_parameter(AnimationBlendTree& tree,
                                                                 std::string name,
                                                                 bool initial_value = false);
[[nodiscard]] ENGINE_ANIMATION_API std::size_t add_event_parameter(AnimationBlendTree& tree,
                                                                  std::string name);

ENGINE_ANIMATION_API void set_blend_tree_root(AnimationBlendTree& tree, std::size_t node) noexcept;

ENGINE_ANIMATION_API void set_linear_blend_weight(AnimationBlendTree& tree, std::size_t node, float weight) noexcept;
ENGINE_ANIMATION_API void set_additive_blend_weight(AnimationBlendTree& tree, std::size_t node, float weight) noexcept;

ENGINE_ANIMATION_API void bind_linear_blend_weight(AnimationBlendTree& tree,
                                                  std::size_t node,
                                                  std::size_t parameter) noexcept;
ENGINE_ANIMATION_API void bind_additive_blend_weight(AnimationBlendTree& tree,
                                                    std::size_t node,
                                                    std::size_t parameter) noexcept;

ENGINE_ANIMATION_API bool set_float_parameter(AnimationBlendTree& tree,
                                             std::size_t parameter,
                                             float value) noexcept;

ENGINE_ANIMATION_API bool set_bool_parameter(AnimationBlendTree& tree,
                                            std::size_t parameter,
                                            bool value) noexcept;

ENGINE_ANIMATION_API bool trigger_event_parameter(AnimationBlendTree& tree,
                                                 std::size_t parameter) noexcept;

ENGINE_ANIMATION_API bool consume_event_parameter(AnimationBlendTree& tree,
                                                 std::size_t parameter) noexcept;

ENGINE_ANIMATION_API bool set_float_parameter(AnimationBlendTree& tree,
                                             std::string_view name,
                                             float value) noexcept;

ENGINE_ANIMATION_API bool set_bool_parameter(AnimationBlendTree& tree,
                                            std::string_view name,
                                            bool value) noexcept;

ENGINE_ANIMATION_API bool trigger_event_parameter(AnimationBlendTree& tree,
                                                 std::string_view name) noexcept;

ENGINE_ANIMATION_API bool consume_event_parameter(AnimationBlendTree& tree,
                                                 std::string_view name) noexcept;

ENGINE_ANIMATION_API void advance_blend_tree(AnimationBlendTree& tree, double dt) noexcept;

[[nodiscard]] ENGINE_ANIMATION_API bool blend_tree_valid(const AnimationBlendTree& tree) noexcept;

[[nodiscard]] ENGINE_ANIMATION_API AnimationRigPose evaluate_blend_tree(const AnimationBlendTree& tree);

}  // namespace engine::animation

extern "C" ENGINE_ANIMATION_API const char* engine_animation_module_name() noexcept;
