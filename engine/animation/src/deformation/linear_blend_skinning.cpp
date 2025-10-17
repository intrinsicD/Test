#include "engine/animation/deformation/linear_blend_skinning.hpp"

#include <stdexcept>

#include "engine/math/transform.hpp"

namespace engine::animation::skinning
{
    namespace
    {
        [[nodiscard]] JointPose joint_pose_from_binding(const AnimationRigPose& pose,
                                                         std::string_view joint_name) noexcept
        {
            if (const auto* resolved = pose.find(joint_name))
            {
                return *resolved;
            }
            return JointPose{};
        }

        [[nodiscard]] math::Transform<float> to_transform(const JointPose& pose) noexcept
        {
            math::Transform<float> transform{};
            transform.scale = pose.scale;
            transform.rotation = pose.rotation;
            transform.translation = pose.translation;
            return transform;
        }
    } // namespace

    bool validate_binding(const RigBinding& binding) noexcept
    {
        if (binding.joints.empty())
        {
            return false;
        }

        const auto joint_count = binding.joints.size();
        for (const auto& vertex : binding.vertices)
        {
            for (std::uint8_t influence_index = 0; influence_index < vertex.influence_count; ++influence_index)
            {
                const auto joint_index = vertex.influences[influence_index].joint;
                if (joint_index >= joint_count)
                {
                    return false;
                }
            }
            if (!vertex.weights_normalized())
            {
                return false;
            }
        }

        return true;
    }

    void build_global_joint_transforms(const RigBinding& binding,
                                       const AnimationRigPose& pose,
                                       std::span<math::Transform<float>> out_global,
                                       const math::vec3& root_translation)
    {
        if (out_global.size() < binding.joints.size())
        {
            throw std::invalid_argument("out_global span is too small for joint count");
        }

        for (std::size_t joint_index = 0; joint_index < binding.joints.size(); ++joint_index)
        {
            const auto& joint = binding.joints[joint_index];
            const JointPose local_pose = joint_pose_from_binding(pose, joint.name);
            math::Transform<float> local_transform = to_transform(local_pose);

            if (joint.parent == RigBinding::kInvalidIndex)
            {
                local_transform.translation += root_translation;
                out_global[joint_index] = local_transform;
            }
            else if (joint.parent < joint_index && joint.parent < binding.joints.size())
            {
                out_global[joint_index] = math::combine(out_global[joint.parent], local_transform);
            }
            else
            {
                out_global[joint_index] = local_transform;
            }
        }
    }

    void build_skinning_transforms(const RigBinding& binding,
                                   std::span<const math::Transform<float>> global_transforms,
                                   std::span<math::Transform<float>> out_skinning)
    {
        if (out_skinning.size() < binding.joints.size())
        {
            throw std::invalid_argument("out_skinning span is too small for joint count");
        }
        if (global_transforms.size() < binding.joints.size())
        {
            throw std::invalid_argument("global_transforms span is too small for joint count");
        }

        for (std::size_t joint_index = 0; joint_index < binding.joints.size(); ++joint_index)
        {
            out_skinning[joint_index] = math::combine(global_transforms[joint_index],
                                                      binding.joints[joint_index].inverse_bind_pose);
        }
    }
} // namespace engine::animation::skinning

