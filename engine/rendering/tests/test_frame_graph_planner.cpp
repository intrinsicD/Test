#include "engine/rendering/frame_graph_planner.hpp"
#include "engine/rendering/frame_graph_registry.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <memory>
#include <sstream>
#include <utility>

namespace engine::rendering
{
    namespace
    {
        class TestNode final : public INode
        {
        public:
            explicit TestNode(NodeDescriptor descriptor)
                : descriptor_{std::move(descriptor)}
            {
            }

            [[nodiscard]] const NodeDescriptor& Reflect() const override
            {
                return descriptor_;
            }

            void Compile(NodeContext&) override
            {
            }

            void Execute(NodeContext&) override
            {
            }

        private:
            NodeDescriptor descriptor_{};
        };

        [[nodiscard]] NodeFactoryDescriptor make_factory(NodeDescriptor descriptor)
        {
            NodeFactoryDescriptor factory{};
            factory.descriptor = descriptor;
            factory.factory = [descriptor = std::move(descriptor)]() mutable -> std::unique_ptr<INode>
            {
                return std::make_unique<TestNode>(descriptor);
            };
            return factory;
        }
    } // namespace

    TEST(FrameGraphPlanner, PlansNodesWithDependencyResolution)
    {
        FrameGraphNodeRegistry registry{};

        NodeDescriptor gbuffer{};
        gbuffer.id = "render.gbuffer";
        ResourceDesc color{};
        color.name = "gbuffer.color";
        color.format = ResourceFormat::Rgba16f;
        color.dimension = ResourceDimension::Texture2D;
        color.width = 1920;
        color.height = 1080;
        gbuffer.creates.push_back(color);
        ResourceDesc depth{};
        depth.name = "gbuffer.depth";
        depth.format = ResourceFormat::Depth24Stencil8;
        depth.dimension = ResourceDimension::Texture2D;
        depth.width = 1920;
        depth.height = 1080;
        gbuffer.creates.push_back(depth);
        registry.register_builtin(make_factory(gbuffer));

        NodeDescriptor lighting{};
        lighting.id = "render.lighting";
        ResourceUse color_read{};
        color_read.name = "gbuffer.color";
        color_read.state = ResourceState::ShaderRead;
        lighting.reads.push_back(color_read);
        ResourceUse depth_read{};
        depth_read.name = "gbuffer.depth";
        depth_read.state = ResourceState::ShaderRead;
        lighting.reads.push_back(depth_read);
        ResourceDesc lighting_output{};
        lighting_output.name = "lighting.color";
        lighting_output.format = ResourceFormat::Rgba16f;
        lighting_output.dimension = ResourceDimension::Texture2D;
        lighting_output.width = 1920;
        lighting_output.height = 1080;
        lighting.creates.push_back(lighting_output);
        registry.register_builtin(make_factory(lighting));

        NodeDescriptor present{};
        present.id = "render.present";
        ResourceUse final_color{};
        final_color.name = "lighting.color";
        final_color.state = ResourceState::ShaderRead;
        present.reads.push_back(final_color);
        ResourceUse swapchain_write{};
        swapchain_write.name = "swapchain";
        swapchain_write.access = resources::Access::Write;
        swapchain_write.state = ResourceState::Present;
        present.writes.push_back(swapchain_write);
        registry.register_builtin(make_factory(present));

        FrameGraphPlanner planner{registry};
        FrameGraphPlanner::PlanRequest request{};
        request.nodes = {"render.gbuffer", "render.lighting", "render.present"};
        ResourceDesc swapchain{};
        swapchain.name = "swapchain";
        swapchain.format = ResourceFormat::Rgba8Unorm;
        swapchain.dimension = ResourceDimension::Texture2D;
        swapchain.width = 1920;
        swapchain.height = 1080;
        swapchain.transient = false;
        request.external_resources.push_back(swapchain);

        auto plan = planner.plan(request);

        ASSERT_EQ(plan.passes().size(), 3U);
        EXPECT_EQ(plan.passes()[0].descriptor.id, "render.gbuffer");
        EXPECT_EQ(plan.passes()[1].descriptor.id, "render.lighting");
        EXPECT_EQ(plan.passes()[2].descriptor.id, "render.present");

        auto color_index = plan.find_resource("gbuffer.color");
        ASSERT_TRUE(color_index.has_value());
        const auto& color_resource = plan.resources()[*color_index];
        EXPECT_TRUE(color_resource.transient);
        EXPECT_FALSE(color_resource.external);

        auto swapchain_index = plan.find_resource("swapchain");
        ASSERT_TRUE(swapchain_index.has_value());
        const auto& swapchain_resource = plan.resources()[*swapchain_index];
        EXPECT_TRUE(swapchain_resource.external);
        EXPECT_FALSE(swapchain_resource.transient);
        EXPECT_EQ(swapchain_resource.alias, std::numeric_limits<std::size_t>::max());
    }

    TEST(FrameGraphPlanner, EmitsGraphvizDotForPlan)
    {
        FrameGraphNodeRegistry registry{};

        NodeDescriptor gbuffer{};
        gbuffer.id = "render.gbuffer";
        ResourceDesc color{};
        color.name = "gbuffer.color";
        color.format = ResourceFormat::Rgba16f;
        color.dimension = ResourceDimension::Texture2D;
        color.width = 1920;
        color.height = 1080;
        gbuffer.creates.push_back(color);
        ResourceDesc depth{};
        depth.name = "gbuffer.depth";
        depth.format = ResourceFormat::Depth24Stencil8;
        depth.dimension = ResourceDimension::Texture2D;
        depth.width = 1920;
        depth.height = 1080;
        gbuffer.creates.push_back(depth);
        registry.register_builtin(make_factory(gbuffer));

        NodeDescriptor lighting{};
        lighting.id = "render.lighting";
        ResourceUse color_read{};
        color_read.name = "gbuffer.color";
        color_read.state = ResourceState::ShaderRead;
        lighting.reads.push_back(color_read);
        ResourceUse depth_read{};
        depth_read.name = "gbuffer.depth";
        depth_read.state = ResourceState::ShaderRead;
        lighting.reads.push_back(depth_read);
        ResourceDesc lighting_output{};
        lighting_output.name = "lighting.color";
        lighting_output.format = ResourceFormat::Rgba16f;
        lighting_output.dimension = ResourceDimension::Texture2D;
        lighting_output.width = 1920;
        lighting_output.height = 1080;
        lighting.creates.push_back(lighting_output);
        registry.register_builtin(make_factory(lighting));

        NodeDescriptor present{};
        present.id = "render.present";
        ResourceUse final_color{};
        final_color.name = "lighting.color";
        final_color.state = ResourceState::ShaderRead;
        present.reads.push_back(final_color);
        ResourceUse swapchain_write{};
        swapchain_write.name = "swapchain";
        swapchain_write.access = resources::Access::Write;
        swapchain_write.state = ResourceState::Present;
        present.writes.push_back(swapchain_write);
        registry.register_builtin(make_factory(present));

        FrameGraphPlanner planner{registry};
        FrameGraphPlanner::PlanRequest request{};
        request.nodes = {"render.gbuffer", "render.lighting", "render.present"};
        ResourceDesc swapchain{};
        swapchain.name = "swapchain";
        swapchain.format = ResourceFormat::Rgba8Unorm;
        swapchain.dimension = ResourceDimension::Texture2D;
        swapchain.width = 1920;
        swapchain.height = 1080;
        swapchain.transient = false;
        request.external_resources.push_back(swapchain);

        const auto plan = planner.plan(request);
        const auto dot = plan.to_dot();

        const auto contains = [&dot](std::string_view needle)
        {
            EXPECT_NE(dot.find(needle), std::string::npos) << "Missing substring: " << needle;
        };

        contains("digraph FrameGraphPlan");
        contains("pass0 [label=\"render.gbuffer\\n[Graphics]");
        contains("pass1 [label=\"render.lighting\\n[Graphics]");
        contains("pass2 [label=\"render.present\\n[Graphics]");
        contains("resource0 [label=\"swapchain\\nTexture2D Rgba8Unorm\\n1920x1080\\nExternal");
        contains("resource1 [label=\"gbuffer.color\\nTexture2D Rgba16f\\n1920x1080\\nTransient");
        contains("resource2 [label=\"gbuffer.depth\\nTexture2D Depth24Stencil8\\n1920x1080\\nTransient");
        contains("resource3 [label=\"lighting.color\\nTexture2D Rgba16f\\n1920x1080\\nTransient");
        contains("pass0 -> resource1 [label=\"create\"]");
        contains("pass0 -> resource2 [label=\"create\"]");
        contains("pass1 -> resource3 [label=\"create\"]");
        contains("resource1 -> pass1 [label=\"read\"]");
        contains("resource2 -> pass1 [label=\"read\"]");
        contains("pass2 -> resource0 [label=\"write\"]");
        contains("resource3 -> pass2 [label=\"read\"]");
    }

    TEST(FrameGraphPlanner, RejectsMissingResourceProducer)
    {
        FrameGraphNodeRegistry registry{};
        NodeDescriptor consumer{};
        consumer.id = "render.consumer";
        ResourceUse missing{};
        missing.name = "undefined.resource";
        consumer.reads.push_back(missing);
        registry.register_builtin(make_factory(consumer));

        FrameGraphPlanner planner{registry};
        FrameGraphPlanner::PlanRequest request{};
        request.nodes = {"render.consumer"};

        EXPECT_THROW(planner.plan(request), std::runtime_error);
    }

    TEST(FrameGraphPlanner, ReusesTransientAllocationsWhenLifetimesDisjoint)
    {
        FrameGraphNodeRegistry registry{};

        NodeDescriptor prepass{};
        prepass.id = "render.prepass";
        ResourceDesc prepass_depth{};
        prepass_depth.name = "prepass.depth";
        prepass_depth.format = ResourceFormat::Rgba16f;
        prepass_depth.dimension = ResourceDimension::Texture2D;
        prepass_depth.width = 1024;
        prepass_depth.height = 1024;
        prepass.creates.push_back(prepass_depth);
        registry.register_builtin(make_factory(prepass));

        NodeDescriptor lighting{};
        lighting.id = "render.lighting";
        ResourceUse depth_read{};
        depth_read.name = "prepass.depth";
        depth_read.state = ResourceState::ShaderRead;
        lighting.reads.push_back(depth_read);
        registry.register_builtin(make_factory(lighting));

        NodeDescriptor blur{};
        blur.id = "render.blur";
        ResourceDesc blur_temp{};
        blur_temp.name = "postprocess.temp";
        blur_temp.format = ResourceFormat::Rgba16f;
        blur_temp.dimension = ResourceDimension::Texture2D;
        blur_temp.width = 1024;
        blur_temp.height = 1024;
        blur.creates.push_back(blur_temp);
        registry.register_builtin(make_factory(blur));

        FrameGraphPlanner planner{registry};
        FrameGraphPlanner::PlanRequest request{};
        request.nodes = {"render.prepass", "render.lighting", "render.blur"};

        auto plan = planner.plan(request);

        auto depth_index = plan.find_resource("prepass.depth");
        auto blur_index = plan.find_resource("postprocess.temp");
        ASSERT_TRUE(depth_index.has_value());
        ASSERT_TRUE(blur_index.has_value());

        const auto& depth_resource = plan.resources()[*depth_index];
        const auto& blur_resource = plan.resources()[*blur_index];

        ASSERT_NE(depth_resource.alias, std::numeric_limits<std::size_t>::max());
        EXPECT_EQ(depth_resource.alias, blur_resource.alias);
    }
}
