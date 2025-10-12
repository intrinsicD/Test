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
                : RenderPass("ForwardGeometry"), color_(color), depth_(depth)
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

        const auto color = graph.create_resource("ForwardColor");
        const auto depth = graph.create_resource("ForwardDepth");

        graph.add_pass(std::make_unique<ForwardGeometryPass>(color, depth));
        graph.compile();

        RenderExecutionContext context{resources, materials, RenderView{scene}, scheduler, device_resources,
                                       encoders};
        graph.execute(context);
    }
}
