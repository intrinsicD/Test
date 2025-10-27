#include "engine/rendering/pipeline/research_baseline.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "engine/assets/validation.hpp"
#include "engine/rendering/command_encoder.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/pipeline/research_baseline_telemetry.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

namespace engine::rendering
{
    namespace
    {
        constexpr std::string_view kFinalColorName{"Research.FinalColor"};
        constexpr std::string_view kDepthName{"Research.Depth"};
        constexpr std::string_view kGBufferAlbedoName{"Research.GBuffer.Albedo"};
        constexpr std::string_view kGBufferNormalName{"Research.GBuffer.Normal"};
        constexpr std::string_view kGBufferMaterialName{"Research.GBuffer.Material"};
        constexpr std::string_view kDebugNormalsName{"Research.Debug.Normals"};
        constexpr std::string_view kDebugUvName{"Research.Debug.UV"};
        constexpr std::string_view kDebugMaterialName{"Research.Debug.Material"};
        constexpr std::string_view kDebugLightVolumeName{"Research.Debug.LightVolume"};

        struct GeometryOutputs
        {
            FrameGraphResourceHandle primary{};
            FrameGraphResourceHandle depth{};
            std::optional<FrameGraphResourceHandle> normals;
            std::optional<FrameGraphResourceHandle> material;
        };

        enum class GeometryMode
        {
            Forward,
            Deferred,
        };

        class ResearchGeometryPass final : public RenderPass
        {
        public:
            ResearchGeometryPass(GeometryMode mode, GeometryOutputs outputs)
                : RenderPass(mode == GeometryMode::Deferred ? "Research.GBuffer" : "Research.ForwardGeometry",
                              QueueType::Graphics,
                              PassPhase::Geometry,
                              ValidationSeverity::Error),
                  outputs_(outputs)
            {
            }

            void setup(FrameGraphPassBuilder& builder) override
            {
                builder.write(outputs_.primary);
                builder.write(outputs_.depth);
                if (outputs_.normals.has_value())
                {
                    builder.write(outputs_.normals.value());
                }
                if (outputs_.material.has_value())
                {
                    builder.write(outputs_.material.value());
                }
            }

            void execute(FrameGraphPassExecutionContext& context) override
            {
                const auto start_time = std::chrono::steady_clock::now();

                auto& scene = context.render.view.scene;
                auto& registry = scene.registry();

                using engine::rendering::components::RenderGeometry;
                using engine::scene::components::WorldTransform;

                auto view = registry.view<WorldTransform, RenderGeometry>();
                draw_commands_.clear();

                for (auto [entity, world, geometry] : view.each())
                {
                    (void)entity;
                    if (const auto* mesh = geometry.mesh(); mesh != nullptr && !mesh->empty())
                    {
                        if (!engine::assets::validate_handle(*mesh, "ResearchGeometryPass::require_mesh"))
                        {
                            continue;
                        }
                        context.render.resources.require_mesh(*mesh);
                    }
                    else if (const auto* graph = geometry.graph(); graph != nullptr && !graph->empty())
                    {
                        if (!engine::assets::validate_handle(*graph, "ResearchGeometryPass::require_graph"))
                        {
                            continue;
                        }
                        context.render.resources.require_graph(*graph);
                    }
                    else if (const auto* point_cloud = geometry.point_cloud();
                             point_cloud != nullptr && !point_cloud->empty())
                    {
                        if (!engine::assets::validate_handle(*point_cloud, "ResearchGeometryPass::require_point_cloud"))
                        {
                            continue;
                        }
                        context.render.resources.require_point_cloud(*point_cloud);
                    }

                    if (!geometry.material.empty())
                    {
                        if (!engine::assets::validate_handle(geometry.material, "ResearchGeometryPass::material"))
                        {
                            continue;
                        }
                        context.render.materials.ensure_material_loaded(geometry.material, context.render.resources);
                    }

                    draw_commands_.push_back(GeometryDrawCommand{geometry.geometry(), geometry.material, world.value});
                }

                auto& encoder = context.command_encoder();
                for (const auto& command : draw_commands_)
                {
                    encoder.draw_geometry(command);
                }

                const auto end_time = std::chrono::steady_clock::now();
                const auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
                ResearchBaselineTelemetry::instance().record_pass(
                    name(), phase(), static_cast<std::uint64_t>(draw_commands_.size()), duration.count());
            }

        private:
            GeometryOutputs outputs_;
            std::vector<GeometryDrawCommand> draw_commands_{};
        };

        class ResearchLightingPass final : public RenderPass
        {
        public:
            ResearchLightingPass(FrameGraphResourceHandle lighting_output,
                                 std::optional<FrameGraphResourceHandle> albedo,
                                 std::optional<FrameGraphResourceHandle> normals,
                                 std::optional<FrameGraphResourceHandle> material,
                                 FrameGraphResourceHandle depth)
                : RenderPass("Research.LightingComposite",
                              QueueType::Graphics,
                              PassPhase::Lighting,
                              ValidationSeverity::Info),
                  lighting_output_(lighting_output),
                  albedo_(albedo),
                  normals_(normals),
                  material_(material),
                  depth_(depth)
            {
            }

            void setup(FrameGraphPassBuilder& builder) override
            {
                if (albedo_.has_value())
                {
                    builder.read(albedo_.value());
                }
                if (normals_.has_value())
                {
                    builder.read(normals_.value());
                }
                if (material_.has_value())
                {
                    builder.read(material_.value());
                }
                builder.read(depth_);
                builder.write(lighting_output_);
            }

            void execute(FrameGraphPassExecutionContext& context) override
            {
                (void)context;
                const auto start_time = std::chrono::steady_clock::now();
                const auto end_time = std::chrono::steady_clock::now();
                const auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
                ResearchBaselineTelemetry::instance().record_pass(
                    name(), phase(), 0U, duration.count());
            }

        private:
            FrameGraphResourceHandle lighting_output_{};
            std::optional<FrameGraphResourceHandle> albedo_;
            std::optional<FrameGraphResourceHandle> normals_;
            std::optional<FrameGraphResourceHandle> material_;
            FrameGraphResourceHandle depth_{};
        };

        class DebugOverlayPass final : public RenderPass
        {
        public:
            DebugOverlayPass(std::string name, FrameGraphResourceHandle source, FrameGraphResourceHandle output)
                : RenderPass(std::move(name), QueueType::Graphics, PassPhase::PostProcess, ValidationSeverity::Info),
                  source_(source),
                  output_(output)
            {
            }

            void setup(FrameGraphPassBuilder& builder) override
            {
                builder.read(source_);
                builder.write(output_);
            }

            void execute(FrameGraphPassExecutionContext& context) override
            {
                (void)context;
                const auto start_time = std::chrono::steady_clock::now();
                const auto end_time = std::chrono::steady_clock::now();
                const auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
                ResearchBaselineTelemetry::instance().record_pass(
                    name(), phase(), 0U, duration.count());
            }

        private:
            FrameGraphResourceHandle source_{};
            FrameGraphResourceHandle output_{};
        };

        FrameGraphResourceDescriptor make_color_descriptor(std::string name,
                                                            const ResearchBaselineOptions& options,
                                                            ResourceFormat format,
                                                            ResourceUsage usage,
                                                            ResourceState final_state)
        {
            FrameGraphResourceDescriptor descriptor{};
            descriptor.name = std::move(name);
            descriptor.format = format;
            descriptor.dimension = ResourceDimension::Texture2D;
            descriptor.usage = usage;
            descriptor.initial_state = ResourceState::ColorAttachment;
            descriptor.final_state = final_state;
            descriptor.width = options.width;
            descriptor.height = options.height;
            descriptor.depth = 1;
            descriptor.array_layers = 1;
            descriptor.mip_levels = 1;
            descriptor.sample_count = ResourceSampleCount::Count1;
            return descriptor;
        }

        FrameGraphResourceDescriptor make_depth_descriptor(std::string name,
                                                            const ResearchBaselineOptions& options)
        {
            FrameGraphResourceDescriptor descriptor{};
            descriptor.name = std::move(name);
            descriptor.format = ResourceFormat::Depth24Stencil8;
            descriptor.dimension = ResourceDimension::Texture2D;
            descriptor.usage = ResourceUsage::DepthStencilAttachment;
            descriptor.initial_state = ResourceState::DepthStencilAttachment;
            descriptor.final_state = ResourceState::DepthStencilAttachment;
            descriptor.width = options.width;
            descriptor.height = options.height;
            descriptor.depth = 1;
            descriptor.array_layers = 1;
            descriptor.mip_levels = 1;
            descriptor.sample_count = ResourceSampleCount::Count1;
            return descriptor;
        }
    } // namespace

    ResearchBaselineResources configure_research_baseline(FrameGraph& graph,
                                                           const ResearchBaselineOptions& options)
    {
        graph.reset();

        ResearchBaselineResources resources{};
        resources.lighting_output = graph.create_resource(
            make_color_descriptor(std::string{kFinalColorName}, options,
                                   ResourceFormat::Rgba16f,
                                   ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead,
                                   ResourceState::ShaderRead));
        resources.depth = graph.create_resource(
            make_depth_descriptor(std::string{kDepthName}, options));

        const bool deferred = options.shading_mode == ResearchShadingMode::Deferred;
        GeometryOutputs geometry_outputs{};
        geometry_outputs.depth = resources.depth;

        ResearchBaselineTelemetry::instance().set_shading_mode(options.shading_mode);
        ResearchBaselineTelemetry::instance().set_overlays(options);

        if (deferred)
        {
            geometry_outputs.primary = graph.create_resource(
                make_color_descriptor(std::string{kGBufferAlbedoName}, options,
                                       ResourceFormat::Rgba16f,
                                       ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead,
                                       ResourceState::ShaderRead));
            resources.gbuffer_albedo = geometry_outputs.primary;

            geometry_outputs.normals = graph.create_resource(
                make_color_descriptor(std::string{kGBufferNormalName}, options,
                                       ResourceFormat::Rgba16f,
                                       ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead,
                                       ResourceState::ShaderRead));
            resources.gbuffer_normals = geometry_outputs.normals;

            geometry_outputs.material = graph.create_resource(
                make_color_descriptor(std::string{kGBufferMaterialName}, options,
                                       ResourceFormat::Rgba8Unorm,
                                       ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead,
                                       ResourceState::ShaderRead));
            resources.gbuffer_material = geometry_outputs.material;
        }
        else
        {
            geometry_outputs.primary = resources.lighting_output;
        }

        auto geometry_pass = std::make_unique<ResearchGeometryPass>(
            deferred ? GeometryMode::Deferred : GeometryMode::Forward, geometry_outputs);
        graph.add_pass(std::move(geometry_pass));

        if (deferred)
        {
            auto lighting_pass = std::make_unique<ResearchLightingPass>(
                resources.lighting_output,
                resources.gbuffer_albedo,
                resources.gbuffer_normals,
                resources.gbuffer_material,
                resources.depth);
            graph.add_pass(std::move(lighting_pass));
        }

        const auto add_overlay = [&](bool enabled, std::string_view name,
                                     std::optional<FrameGraphResourceHandle>& handle_slot) {
            if (!enabled)
            {
                return;
            }

            auto descriptor = make_color_descriptor(std::string{name}, options,
                                                     ResourceFormat::Rgba16f,
                                                     ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead,
                                                     ResourceState::ShaderRead);
            auto handle = graph.create_resource(std::move(descriptor));
            handle_slot = handle;

            auto pass = std::make_unique<DebugOverlayPass>(std::string{name}, resources.lighting_output, handle);
            graph.add_pass(std::move(pass));
        };

        add_overlay(options.enable_normals_overlay, kDebugNormalsName, resources.debug_normals_overlay);
        add_overlay(options.enable_uv_overlay, kDebugUvName, resources.debug_uv_overlay);
        add_overlay(options.enable_material_overlay, kDebugMaterialName, resources.debug_material_overlay);
        add_overlay(options.enable_light_volume_overlay, kDebugLightVolumeName, resources.debug_light_volume_overlay);

        return resources;
    }
}
