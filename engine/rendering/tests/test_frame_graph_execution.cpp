#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "command_encoder_test_utils.hpp"
#include "scheduler_test_utils.hpp"

#include "engine/assets/handles.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/frame_graph_planner.hpp"
#include "engine/rendering/frame_graph_registry.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/rendering/runtime_submission.hpp"
#include "engine/scene/scene.hpp"

namespace engine::rendering
{
    namespace
    {
        struct ResourceSnapshot
        {
            std::string name;
            ResourceUsage usage{ResourceUsage::None};
            ResourceState initial_state{ResourceState::Undefined};
            ResourceState final_state{ResourceState::Undefined};
        };

        class RecordingNode final : public INode
        {
        public:
            using ExecuteFn = std::function<void(NodeContext&)>;

            RecordingNode(NodeDescriptor descriptor,
                          std::vector<std::string>& compile_order,
                          std::vector<std::string>& execute_order,
                          std::vector<QueueType>& queues,
                          std::vector<ResourceSnapshot>& snapshots,
                          ExecuteFn execute)
                : descriptor_(std::move(descriptor))
                , compile_order_(compile_order)
                , execute_order_(execute_order)
                , queues_(queues)
                , snapshots_(snapshots)
                , execute_(std::move(execute))
            {
            }

            [[nodiscard]] const NodeDescriptor& Reflect() const override
            {
                return descriptor_;
            }

            void Compile(NodeContext& context) override
            {
                compile_order_.push_back(std::string{descriptor_.id});
                snapshots_.push_back(ResourceSnapshot{std::string{context.descriptor().id}});

                const auto record_info = [&](std::string_view name)
                {
                    const auto& info = context.resource_info(name);
                    ResourceSnapshot snapshot{};
                    snapshot.name = std::string{info.name};
                    snapshot.usage = info.usage;
                    snapshot.initial_state = info.initial_state;
                    snapshot.final_state = info.final_state;
                    snapshots_.push_back(std::move(snapshot));
                };

                for (const auto& create : descriptor_.creates)
                {
                    record_info(create.name);
                }
                for (const auto& read : descriptor_.reads)
                {
                    record_info(read.name);
                }
                for (const auto& write : descriptor_.writes)
                {
                    record_info(write.name);
                }
            }

            void Execute(NodeContext& context) override
            {
                execute_order_.push_back(std::string{descriptor_.id});
                queues_.push_back(context.queue());
                if (execute_)
                {
                    execute_(context);
                }
            }

        private:
            NodeDescriptor descriptor_{};
            std::vector<std::string>& compile_order_;
            std::vector<std::string>& execute_order_;
            std::vector<QueueType>& queues_;
            std::vector<ResourceSnapshot>& snapshots_;
            ExecuteFn execute_{};
        };

        [[nodiscard]] NodeFactoryDescriptor make_factory(NodeDescriptor descriptor,
                                                          std::vector<std::string>& compile_order,
                                                          std::vector<std::string>& execute_order,
                                                          std::vector<QueueType>& queues,
                                                          std::vector<ResourceSnapshot>& snapshots,
                                                          RecordingNode::ExecuteFn execute_fn)
        {
            NodeFactoryDescriptor factory{};
            factory.descriptor = descriptor;
            factory.factory = [descriptor = std::move(descriptor), &compile_order, &execute_order, &queues,
                                  &snapshots, execute_fn]() mutable -> std::unique_ptr<INode>
            {
                return std::make_unique<RecordingNode>(descriptor, compile_order, execute_order, queues,
                    snapshots, execute_fn);
            };
            return factory;
        }
    } // namespace

    TEST(FrameGraphExecution, ExecutesPlannedPassesAndRecordsTelemetry)
    {
        FrameGraphNodeRegistry registry{};

        std::vector<std::string> compile_order;
        std::vector<std::string> execute_order;
        std::vector<QueueType> queues;
        std::vector<ResourceSnapshot> snapshots;

        NodeDescriptor gbuffer{};
        gbuffer.id = "render.gbuffer";
        ResourceDesc color{};
        color.name = "gbuffer.color";
        color.kind = ResourceKind::Texture;
        color.format = ResourceFormat::Rgba16f;
        color.dimension = ResourceDimension::Texture2D;
        color.width = 1280;
        color.height = 720;
        gbuffer.creates.push_back(color);
        ResourceDesc depth{};
        depth.name = "gbuffer.depth";
        depth.kind = ResourceKind::Texture;
        depth.format = ResourceFormat::Depth24Stencil8;
        depth.dimension = ResourceDimension::Texture2D;
        depth.width = 1280;
        depth.height = 720;
        gbuffer.creates.push_back(depth);
        ResourceUse color_write{};
        color_write.name = "gbuffer.color";
        color_write.stage = resources::PipelineStage::Graphics;
        color_write.access = resources::Access::Write;
        color_write.state = ResourceState::ColorAttachment;
        gbuffer.writes.push_back(color_write);
        ResourceUse depth_write{};
        depth_write.name = "gbuffer.depth";
        depth_write.stage = resources::PipelineStage::Graphics;
        depth_write.access = resources::Access::Write;
        depth_write.state = ResourceState::DepthStencilAttachment;
        gbuffer.writes.push_back(depth_write);

        NodeDescriptor present{};
        present.id = "render.present";
        ResourceUse color_read{};
        color_read.name = "gbuffer.color";
        color_read.stage = resources::PipelineStage::Graphics;
        color_read.access = resources::Access::Read;
        color_read.state = ResourceState::ShaderRead;
        present.reads.push_back(color_read);
        ResourceUse swapchain_write{};
        swapchain_write.name = "swapchain";
        swapchain_write.stage = resources::PipelineStage::Graphics;
        swapchain_write.access = resources::Access::Write;
        swapchain_write.state = ResourceState::Present;
        present.writes.push_back(swapchain_write);

        registry.register_builtin(make_factory(gbuffer, compile_order, execute_order, queues, snapshots,
            [](NodeContext& context)
            {
                GeometryDrawCommand draw{};
                context.command_encoder().draw_geometry(draw);
            }));
        registry.register_builtin(make_factory(present, compile_order, execute_order, queues, snapshots,
            [](NodeContext& context)
            {
                GeometryDrawCommand draw{};
                context.command_encoder().draw_geometry(draw);
            }));

        FrameGraphPlanner planner{registry};
        FrameGraphPlanner::PlanRequest request{};
        request.nodes = {"render.gbuffer", "render.present"};
        ResourceDesc swapchain{};
        swapchain.name = "swapchain";
        swapchain.kind = ResourceKind::Texture;
        swapchain.format = ResourceFormat::Rgba8Unorm;
        swapchain.dimension = ResourceDimension::Texture2D;
        swapchain.width = 1280;
        swapchain.height = 720;
        swapchain.transient = false;
        request.external_resources.push_back(swapchain);

        auto plan = planner.plan(request);

        MaterialSystem materials;
        scene::Scene scene;
        FrameGraph frame_graph;
        class RecordingProvider final : public RenderResourceProvider
        {
        public:
            void require_mesh(const engine::assets::MeshHandle&) override {}
            void require_graph(const engine::assets::GraphHandle&) override {}
            void require_point_cloud(const engine::assets::PointCloudHandle&) override {}
            void require_material(const engine::assets::MaterialHandle&) override {}
            void require_shader(const engine::assets::ShaderHandle&) override {}
        } resource_provider;

        resources::RecordingGpuResourceProvider device_provider{};
        tests::RecordingScheduler scheduler;
        RecordingCommandEncoderProvider encoders;

        RuntimeSubmissionContext submission{
            resource_provider,
            materials,
            device_provider,
            scheduler,
            encoders,
            nullptr,
            frame_graph,
            nullptr,
        };

        auto execution = submission.make_execution_context(scene);
        FrameGraphPlanner::Plan::ExecutionTelemetry telemetry{};
        plan.execute(execution, &telemetry);

        ASSERT_EQ(compile_order.size(), 2U);
        EXPECT_EQ(compile_order[0], "render.gbuffer");
        EXPECT_EQ(compile_order[1], "render.present");
        ASSERT_EQ(execute_order.size(), 2U);
        EXPECT_EQ(execute_order[0], "render.gbuffer");
        EXPECT_EQ(execute_order[1], "render.present");

        ASSERT_EQ(queues.size(), 2U);
        EXPECT_EQ(queues[0], QueueType::Graphics);
        EXPECT_EQ(queues[1], QueueType::Graphics);

        const auto color_snapshot = std::find_if(snapshots.begin(), snapshots.end(),
            [](const ResourceSnapshot& snapshot) { return snapshot.name == "gbuffer.color"; });
        ASSERT_NE(color_snapshot, snapshots.end());
        EXPECT_TRUE(has_flag(color_snapshot->usage, ResourceUsage::ColorAttachment));
        EXPECT_TRUE(has_flag(color_snapshot->usage, ResourceUsage::ShaderRead));
        EXPECT_EQ(color_snapshot->initial_state, ResourceState::ColorAttachment);
        EXPECT_EQ(color_snapshot->final_state, ResourceState::ShaderRead);

        EXPECT_EQ(device_provider.frames_begun(), 1U);
        EXPECT_EQ(device_provider.frames_completed(), 1U);
        EXPECT_EQ(device_provider.acquired().size(), 2U);
        EXPECT_EQ(device_provider.released().size(), 2U);
        EXPECT_EQ(telemetry.transient_acquires, 2U);
        EXPECT_EQ(telemetry.transient_releases, 2U);
        EXPECT_EQ(telemetry.submissions, 2U);

        ASSERT_EQ(scheduler.submissions.size(), 2U);
        EXPECT_EQ(scheduler.submissions[1].begin_barriers.size(), 1U);
        EXPECT_EQ(scheduler.submissions[1].begin_barriers.front().destination_access, resources::Access::Read);

        const auto& completed_encoders = encoders.completed_encoders();
        ASSERT_EQ(completed_encoders.size(), 2U);
        EXPECT_EQ(completed_encoders[0]->geometry_draws().size(), 1U);
        EXPECT_EQ(completed_encoders[1]->geometry_draws().size(), 1U);
    }
} // namespace engine::rendering
