#pragma once

#include <span>

#include "engine/animation/rigging/rig_binding.hpp"
#include "engine/geometry/api.hpp"
#include "engine/math/transform.hpp"

namespace engine::geometry::deform
{
    void apply_linear_blend_skinning(const animation::RigBinding& binding,
                                     std::span<const math::Transform<float>> skinning_transforms,
                                     SurfaceMesh& mesh);
}

