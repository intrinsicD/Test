#pragma once

#include <cstddef>
#include <string>
#include <string_view>
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

    [[nodiscard]] constexpr std::string_view to_string(HierarchyIssueType type) noexcept
    {
        switch (type)
        {
        case HierarchyIssueType::Cycle:
            return "cycle";
        case HierarchyIssueType::DanglingParent:
            return "dangling_parent";
        case HierarchyIssueType::MissingParentHierarchy:
            return "missing_parent_hierarchy";
        case HierarchyIssueType::NonFiniteLocalTransform:
            return "non_finite_local_transform";
        case HierarchyIssueType::NonFiniteWorldTransform:
            return "non_finite_world_transform";
        case HierarchyIssueType::TransformMismatch:
            return "transform_mismatch";
        }

        return "unknown";
    }

    [[nodiscard]] HierarchyValidationReport validate_hierarchy(
        const entt::registry& registry,
        const HierarchyValidationOptions& options = {});

    [[nodiscard]] HierarchyValidationReport validate_hierarchy(
        const Scene& scene,
        const HierarchyValidationOptions& options = {});
} // namespace engine::scene::validation