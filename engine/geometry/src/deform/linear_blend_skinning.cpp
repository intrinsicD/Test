#include "engine/geometry/deform/linear_blend_skinning.hpp"

#include <stdexcept>

#include "engine/geometry/api.hpp"
#include "engine/math/transform.hpp"

namespace engine::geometry::deform
{
    namespace
    {
        [[nodiscard]] math::vec3 transform_vertex(const math::Transform<float>& transform,
                                                  const math::vec3& position) noexcept
        {
            return math::transform_point(transform, position);
        }
    } // namespace

    void apply_linear_blend_skinning(const animation::RigBinding& binding,
                                     std::span<const math::Transform<float>> skinning_transforms,
                                     SurfaceMesh& mesh)
    {
        if (skinning_transforms.size() < binding.joints.size())
        {
            throw std::invalid_argument("skinning_transforms span is too small for joint count");
        }

        if (mesh.rest_positions.empty())
        {
            mesh.positions.clear();
            mesh.normals.clear();
            update_bounds(mesh);
            return;
        }

        mesh.positions.resize(mesh.rest_positions.size());

        for (std::size_t vertex_index = 0; vertex_index < mesh.rest_positions.size(); ++vertex_index)
        {
            const auto& rest_position = mesh.rest_positions[vertex_index];
            math::vec3 skinned_position{0.0F, 0.0F, 0.0F};
            float accumulated_weight = 0.0F;

            if (vertex_index < binding.vertices.size())
            {
                const auto& influences = binding.vertices[vertex_index];
                for (std::uint8_t influence_index = 0; influence_index < influences.influence_count; ++influence_index)
                {
                    const auto& influence = influences.influences[influence_index];
                    if (influence.joint >= skinning_transforms.size())
                    {
                        continue;
                    }
                    skinned_position += influence.weight *
                        transform_vertex(skinning_transforms[influence.joint], rest_position);
                    accumulated_weight += influence.weight;
                }
            }

            if (accumulated_weight <= 0.0F)
            {
                skinned_position = rest_position;
            }

            mesh.positions[vertex_index] = skinned_position;
        }

        recompute_vertex_normals(mesh);
        update_bounds(mesh);
    }
} // namespace engine::geometry::deform

