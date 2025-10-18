#include "engine/rendering/forward_pipeline.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "engine/rendering/components.hpp"
#include "engine/rendering/command_encoder.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

namespace engine::rendering
{
    namespace
    {
        class ForwardGeometryPass final : public RenderPass
        {
        public:
            ForwardGeometryPass(FrameGraphResourceHandle color, FrameGraphResourceHandle depth)
                : RenderPass("ForwardGeometry",
                              QueueType::Graphics,
                              PassPhase::Geometry,
                              ValidationSeverity::Error),
                  color_(color),
                  depth_(depth)
            {
            }

            void setup(FrameGraphPassBuilder& builder) override
            {
                builder.write(color_);
                builder.write(depth_);
            }

            void execute(FrameGraphPassExecutionContext& context) override
            {
                auto& scene = context.render.view.scene;
                auto& registry = scene.registry();

                using engine::rendering::components::RenderGeometry;
                using engine::scene::components::WorldTransform;

                auto view = registry.view<WorldTransform, RenderGeometry>();
                draw_commands_.clear();

                for (auto [entity, world, geometry] : view.each())
                {
                    if (const auto* mesh = geometry.mesh(); mesh != nullptr && !mesh->empty())
                    {
                        context.render.resources.require_mesh(*mesh);
                    }
                    else if (const auto* graph = geometry.graph(); graph != nullptr && !graph->empty())
                    {
                        context.render.resources.require_graph(*graph);
                    }
                    else if (const auto* point_cloud = geometry.point_cloud();
                             point_cloud != nullptr && !point_cloud->empty())
                    {
                        context.render.resources.require_point_cloud(*point_cloud);
                    }

                    if (!geometry.material.empty())
                    {
                        context.render.materials.ensure_material_loaded(geometry.material, context.render.resources);
                    }

                    draw_commands_.push_back(
                        GeometryDrawCommand{geometry.geometry(), geometry.material, world.value});
                }

                auto& encoder = context.command_encoder();
                for (const auto& command : draw_commands_)
                {
                    encoder.draw_geometry(command);
                }
            }

            [[nodiscard]] const std::vector<GeometryDrawCommand>& draw_commands() const noexcept
            {
                return draw_commands_;
            }

        private:
            FrameGraphResourceHandle color_;
            FrameGraphResourceHandle depth_;
            std::vector<GeometryDrawCommand> draw_commands_{};
        };
    } // namespace

    void ForwardPipeline::render(scene::Scene& scene, RenderResourceProvider& resources,
                                 MaterialSystem& materials, resources::IGpuResourceProvider& device_resources,
                                 IGpuScheduler& scheduler, CommandEncoderProvider& encoders, FrameGraph& graph)
    {
        graph.reset();

        FrameGraphResourceDescriptor color_desc{};
        color_desc.name = "ForwardColor";
        color_desc.format = ResourceFormat::Rgba16f;
        color_desc.dimension = ResourceDimension::Texture2D;
        color_desc.usage = ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead;
        color_desc.initial_state = ResourceState::ColorAttachment;
        color_desc.final_state = ResourceState::ShaderRead;
        color_desc.width = 1280;
        color_desc.height = 720;
        color_desc.depth = 1;
        color_desc.array_layers = 1;
        color_desc.mip_levels = 1;
        color_desc.sample_count = ResourceSampleCount::Count1;
        const auto color = graph.create_resource(std::move(color_desc));

        FrameGraphResourceDescriptor depth_desc{};
        depth_desc.name = "ForwardDepth";
        depth_desc.format = ResourceFormat::Depth24Stencil8;
        depth_desc.dimension = ResourceDimension::Texture2D;
        depth_desc.usage = ResourceUsage::DepthStencilAttachment;
        depth_desc.initial_state = ResourceState::DepthStencilAttachment;
        depth_desc.final_state = ResourceState::DepthStencilAttachment;
        depth_desc.width = 1280;
        depth_desc.height = 720;
        depth_desc.depth = 1;
        depth_desc.array_layers = 1;
        depth_desc.mip_levels = 1;
        depth_desc.sample_count = ResourceSampleCount::Count1;
        const auto depth = graph.create_resource(std::move(depth_desc));

        graph.add_pass(std::make_unique<ForwardGeometryPass>(color, depth));
        graph.compile();

        RenderExecutionContext context{resources, materials, RenderView{scene}, scheduler, device_resources,
                                       encoders};
        graph.execute(context);
    }
}
