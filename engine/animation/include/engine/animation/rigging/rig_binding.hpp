#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/math/transform.hpp"

namespace engine::animation {

struct RigJoint {
    std::string name;
    std::uint32_t parent{std::numeric_limits<std::uint32_t>::max()};
    math::Transform<float> inverse_bind_pose{math::Transform<float>::Identity()};
};

struct VertexInfluence {
    std::uint16_t joint{0};
    float weight{0.0F};
};

struct VertexBinding {
    static constexpr std::size_t kMaxInfluences = 4;

    std::array<VertexInfluence, kMaxInfluences> influences{};
    std::uint8_t influence_count{0};

    constexpr void clear() noexcept {
        influence_count = 0;
        influences = {};
    }

    [[nodiscard]] bool add_influence(std::uint16_t joint_index, float weight) noexcept {
        if (weight <= 0.0F) {
            return false;
        }

        if (influence_count < kMaxInfluences) {
            influences[influence_count++] = VertexInfluence{joint_index, weight};
            return true;
        }

        auto begin = influences.begin();
        auto end = begin + influence_count;
        auto smallest = std::min_element(begin, end, [](const VertexInfluence& lhs, const VertexInfluence& rhs) {
            return lhs.weight < rhs.weight;
        });
        if (smallest == end || weight <= smallest->weight) {
            return false;
        }
        *smallest = VertexInfluence{joint_index, weight};
        return true;
    }

    void normalize_weights() noexcept {
        if (influence_count == 0) {
            return;
        }

        float sum = 0.0F;
        for (std::uint8_t i = 0; i < influence_count; ++i) {
            sum += influences[i].weight;
        }

        if (sum <= 0.0F) {
            clear();
            return;
        }

        const float inv = 1.0F / sum;
        for (std::uint8_t i = 0; i < influence_count; ++i) {
            influences[i].weight *= inv;
        }
    }

    [[nodiscard]] bool weights_normalized(float epsilon = 1.0e-4F) const noexcept {
        if (influence_count == 0) {
            return true;
        }

        float sum = 0.0F;
        for (std::uint8_t i = 0; i < influence_count; ++i) {
            sum += influences[i].weight;
        }
        return std::abs(sum - 1.0F) <= epsilon;
    }
};

struct RigBinding {
    static constexpr std::uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max();

    std::vector<RigJoint> joints;
    std::vector<VertexBinding> vertices;

    [[nodiscard]] constexpr bool empty() const noexcept { return joints.empty() && vertices.empty(); }

    void resize_vertices(std::size_t count) {
        const std::size_t previous_size = vertices.size();
        vertices.resize(count);
        for (std::size_t i = previous_size; i < vertices.size(); ++i) {
            vertices[i].clear();
        }
    }

    [[nodiscard]] std::optional<std::uint32_t> find_joint_index(std::string_view name) const noexcept {
        for (std::uint32_t i = 0; i < joints.size(); ++i) {
            if (joints[i].name == name) {
                return i;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool set_vertex_influences(std::size_t vertex_index,
                                             std::span<const VertexInfluence> influences_to_assign) {
        if (vertex_index >= vertices.size()) {
            return false;
        }
        if (influences_to_assign.size() > VertexBinding::kMaxInfluences) {
            return false;
        }

        VertexBinding binding{};
        for (const auto& influence : influences_to_assign) {
            if (influence.joint >= joints.size()) {
                return false;
            }
            if (!binding.add_influence(influence.joint, influence.weight)) {
                return false;
            }
        }
        binding.normalize_weights();
        vertices[vertex_index] = binding;
        return true;
    }

    [[nodiscard]] bool normalized(float epsilon = 1.0e-4F) const noexcept {
        return std::all_of(vertices.begin(), vertices.end(), [epsilon](const VertexBinding& binding) {
            return binding.weights_normalized(epsilon);
        });
    }
};

}  // namespace engine::animation

