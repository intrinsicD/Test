#pragma once

#include <span>

#include "engine/animation/api.hpp"
#include "engine/animation/rigging/rig_binding.hpp"
#include "engine/math/transform.hpp"

namespace engine::animation::skinning
{
    [[nodiscard]] bool validate_binding(const RigBinding& binding) noexcept;

    void build_global_joint_transforms(const RigBinding& binding,
                                       const AnimationRigPose& pose,
                                       std::span<math::Transform<float>> out_global,
                                       const math::vec3& root_translation = math::vec3{0.0F, 0.0F, 0.0F});

    void build_skinning_transforms(const RigBinding& binding,
                                   std::span<const math::Transform<float>> global_transforms,
                                   std::span<math::Transform<float>> out_skinning);
}

