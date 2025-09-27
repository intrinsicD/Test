#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "engine/geometry/property_registry.hpp"

namespace geo = engine::geometry;

TEST(PropertyRegistry, AddGetAndDefaults)
{
    geo::PropertyRegistry registry;
    auto weights_opt = registry.add<float>("weight", 1.5f);
    ASSERT_EQ(weights_opt.has_value(), true);
    auto weights = *weights_opt;
    EXPECT_EQ(registry.size(), 0u);
    EXPECT_EQ(registry.property_count(), 1u);

    registry.resize(3);
    ASSERT_EQ(weights.vector().size(), 3u);
    EXPECT_FLOAT_EQ(weights[0], 1.5f);
    EXPECT_FLOAT_EQ(weights[2], 1.5f);

    weights.vector()[1] = 2.0f;
    registry.push_back();
    EXPECT_EQ(registry.size(), 4u);
    EXPECT_FLOAT_EQ(weights[1], 2.0f);
    EXPECT_FLOAT_EQ(weights[3], 1.5f);

    const auto duplicate = registry.add<float>("weight", 9.0f);
    EXPECT_TRUE(!duplicate.has_value());

    auto labels_opt = registry.add<std::string>("label", std::string{"unset"});
    ASSERT_EQ(labels_opt.has_value(), true);
    auto labels = *labels_opt;
    EXPECT_EQ(labels.vector().size(), registry.size());
    EXPECT_EQ(labels[3], "unset");

    labels.vector()[0] = "first";
    labels.vector()[1] = "second";
    registry.swap(0, 1);
    EXPECT_EQ(labels[0], "second");
    EXPECT_EQ(labels[1], "first");

    const geo::PropertyRegistry& const_registry = registry;
    auto const_labels = const_registry.get<std::string>("label");
    ASSERT_EQ(const_labels.has_value(), true);
    EXPECT_EQ((*const_labels)[0], "second");

    auto maybe_missing = const_registry.get<float>("missing");
    EXPECT_TRUE(!maybe_missing.has_value());

    auto weights_again = registry.get_or_add<float>("weight", 3.0f);
    EXPECT_TRUE(static_cast<bool>(weights_again));
    EXPECT_EQ(weights_again.id(), weights.id());
    EXPECT_FLOAT_EQ(weights_again[0], 2.0f);
    EXPECT_FLOAT_EQ(weights_again[1], 1.5f);

    auto ids = registry.get_or_add<int>("id", 7);
    EXPECT_EQ(ids.vector().size(), registry.size());
    EXPECT_TRUE(std::all_of(ids.vector().begin(), ids.vector().end(), [](int v) { return v == 7; }));

    EXPECT_TRUE(registry.contains("id"));
    auto id_lookup = registry.find("id");
    ASSERT_EQ(id_lookup.has_value(), true);
    EXPECT_TRUE(registry.remove(*id_lookup));
    EXPECT_TRUE(!registry.contains("id"));

    EXPECT_TRUE(registry.remove(weights_again));
    EXPECT_TRUE(!static_cast<bool>(weights_again));
    EXPECT_TRUE(!registry.contains("weight"));
    EXPECT_TRUE(!registry.get<float>("weight"));

    registry.clear();
    EXPECT_EQ(registry.size(), 0u);
    EXPECT_EQ(registry.property_count(), 0u);
}

