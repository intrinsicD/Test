#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <entt/entity/entity.hpp>

namespace engine::scene
{
    class Scene;
}

namespace engine::scene::components
{
    struct Hierarchy;
}

namespace engine::scene::validation
{
    enum class HierarchyIssueType
    {
        Cycle,
        DanglingParent,
        MissingParentHierarchy,
        NonFiniteLocalTransform,
        NonFiniteWorldTransform,
        TransformMismatch,
    };

    struct HierarchyValidationIssue
    {
        entt::entity entity{entt::null};
        entt::entity related{entt::null};
        HierarchyIssueType type{HierarchyIssueType::Cycle};
        std::string message{};
    };

    struct HierarchyValidationMetrics
    {
        std::size_t issue_count{0};
        std::size_t cycle_count{0};
        std::size_t dangling_parent_count{0};
        std::size_t missing_parent_hierarchy_count{0};
        std::size_t non_finite_transform_count{0};
        std::size_t transform_mismatch_count{0};
    };

    struct HierarchyValidationOptions
    {
        float transform_tolerance{1e-4F};
        bool skip_dirty_entities{true};
    };

    struct HierarchyValidationReport
    {
        HierarchyValidationMetrics metrics{};
        std::vector<HierarchyValidationIssue> issues{};

        [[nodiscard]] bool ok() const noexcept
        {
            return metrics.issue_count == 0U;
        }
    };

    [[nodiscard]] HierarchyValidationReport validate_hierarchy(
        const entt::registry& registry,
        const HierarchyValidationOptions& options = {});

    [[nodiscard]] HierarchyValidationReport validate_hierarchy(
        const Scene& scene,
        const HierarchyValidationOptions& options = {});
} // namespace engine::scene::validation
