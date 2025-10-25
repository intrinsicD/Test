#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>

#include "runtime_dispatch_demo/queue_assignment.hpp"

namespace samples = engine::compute::samples;

TEST(RuntimeDispatchQueueAssignment, DeterministicAcrossInvocations)
{
    constexpr std::array<std::string_view, 4> categories{
        "animation.evaluate",
        "physics.integrate",
        "geometry.deform",
        "custom.kernel"
    };

    for (const auto category : categories)
    {
        const auto first = samples::deterministic_queue_index(category, 7U);
        const auto second = samples::deterministic_queue_index(category, 7U);
        EXPECT_EQ(first, second);
    }
}

TEST(RuntimeDispatchQueueAssignment, RespectsQueueCountBounds)
{
    EXPECT_EQ(samples::deterministic_queue_index("animation", 0U), 0U);
    EXPECT_EQ(samples::deterministic_queue_index("animation", 1U), 0U);

    const auto index = samples::deterministic_queue_index("geometry", 3U);
    EXPECT_LT(index, 3U);
}

TEST(RuntimeDispatchQueueAssignment, CaseInsensitiveHashing)
{
    const auto lower = samples::deterministic_queue_index("physics", 5U);
    const auto upper = samples::deterministic_queue_index("PHYSICS", 5U);
    EXPECT_EQ(lower, upper);
}

