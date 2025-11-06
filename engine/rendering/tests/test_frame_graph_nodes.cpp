#include "engine/rendering/frame_graph_node.hpp"

#include <gtest/gtest.h>

namespace engine::rendering
{
    TEST(FrameGraphNodeTypes, ResourceDescDefaults)
    {
        ResourceDesc desc{};
        EXPECT_TRUE(desc.name.empty());
        EXPECT_EQ(desc.kind, ResourceKind::Texture);
        EXPECT_EQ(desc.format, ResourceFormat::Unknown);
        EXPECT_EQ(desc.dimension, ResourceDimension::Texture2D);
        EXPECT_EQ(desc.width, 1U);
        EXPECT_EQ(desc.height, 1U);
        EXPECT_EQ(desc.depth, 1U);
        EXPECT_EQ(desc.array_layers, 1U);
        EXPECT_EQ(desc.mip_levels, 1U);
        EXPECT_EQ(desc.sample_count, ResourceSampleCount::Count1);
        EXPECT_TRUE(desc.transient);
    }

    TEST(FrameGraphNodeTypes, ResourceUseDefaults)
    {
        ResourceUse use{};
        EXPECT_TRUE(use.name.empty());
        EXPECT_EQ(use.stage, resources::PipelineStage::Graphics);
        EXPECT_EQ(use.access, resources::Access::Read);
        EXPECT_EQ(use.state, ResourceState::Undefined);
        EXPECT_TRUE(use.is_read_only());
        EXPECT_FALSE(use.is_write());
    }

    TEST(FrameGraphNodeDescriptor, FindersInspectDeclarations)
    {
        NodeDescriptor descriptor{};
        descriptor.id = "test.node";

        ResourceDesc color{};
        color.name = "color";
        color.format = ResourceFormat::Rgba16f;
        descriptor.creates.push_back(color);

        ResourceUse history_read{};
        history_read.name = "history";
        history_read.state = ResourceState::ShaderRead;
        descriptor.reads.push_back(history_read);

        ResourceUse normal_write{};
        normal_write.name = "normal";
        normal_write.access = resources::Access::Write;
        normal_write.state = ResourceState::ShaderWrite;
        descriptor.writes.push_back(normal_write);

        EXPECT_TRUE(descriptor.declares_resource("color"));
        EXPECT_TRUE(descriptor.declares_resource("history"));
        EXPECT_TRUE(descriptor.declares_resource("normal"));
        EXPECT_FALSE(descriptor.declares_resource("depth"));

        const auto* create_desc = descriptor.find_created("color");
        ASSERT_NE(create_desc, nullptr);
        EXPECT_EQ(create_desc->format, ResourceFormat::Rgba16f);

        const auto* read_use = descriptor.find_read("history");
        ASSERT_NE(read_use, nullptr);
        EXPECT_EQ(read_use->state, ResourceState::ShaderRead);
        EXPECT_TRUE(read_use->is_read_only());

        const auto* write_use = descriptor.find_write("normal");
        ASSERT_NE(write_use, nullptr);
        EXPECT_EQ(write_use->state, ResourceState::ShaderWrite);
        EXPECT_TRUE(write_use->is_write());
        EXPECT_FALSE(write_use->is_read_only());
    }
}
