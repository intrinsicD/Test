#include "engine/rendering/selection_outline_strategy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>

#include "engine/assets/validation.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/rendering/command_encoder.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

namespace engine::rendering
{
    namespace
    {
        using scene::selection::SelectionEngine;
        using scene::selection::visualization::MultiSelectionMode;
        using scene::selection::visualization::OcclusionMode;
        using scene::selection::visualization::OutlineConfig;
        using scene::selection::visualization::OutlineQuality;
        using scene::selection::visualization::ThicknessMode;

        struct CameraUniforms
        {
            engine::math::mat4 view{engine::math::identity_matrix<float, 4>()};
            engine::math::mat4 projection{engine::math::identity_matrix<float, 4>()};
            engine::math::vec3 position{0.0F, 0.0F, 0.0F};
        };

        [[nodiscard]] CameraUniforms resolve_camera_uniforms(entt::registry& registry)
        {
            CameraUniforms uniforms{};
            auto cameras = registry.view<engine::rendering::Camera>();
            for (auto entity : cameras)
            {
                const auto& camera = cameras.get<engine::rendering::Camera>(entity);
                uniforms.view = camera.view;
                uniforms.projection = camera.projection;

                if (registry.any_of<engine::scene::components::WorldTransform>(entity))
                {
                    uniforms.position =
                        registry.get<engine::scene::components::WorldTransform>(entity).value.translation;
                }
                else
                {
                    uniforms.position = camera.transform().translation;
                }
                break;
            }

            return uniforms;
        }

        [[nodiscard]] float clamp_positive(float value, float min_value)
        {
            return std::max(value, min_value);
        }

        class GeometryInflationOutlinePass final : public RenderPass
        {
        public:
            GeometryInflationOutlinePass(FrameGraphResourceHandle color,
                                         FrameGraphResourceHandle depth,
                                         SelectionEngine** engine,
                                         OutlineConfig* config,
                                         bool* enabled,
                                         bool force_screen_space)
                : RenderPass("Selection.Outline", QueueType::Graphics, PassPhase::PostProcess,
                             ValidationSeverity::Info),
                  color_(color),
                  depth_(depth),
                  engine_(engine),
                  config_(config),
                  enabled_(enabled),
                  force_screen_space_(force_screen_space)
            {
            }

            void setup(FrameGraphPassBuilder& builder) override
            {
                builder.read(depth_);
                builder.write(color_);
            }

            void execute(FrameGraphPassExecutionContext& context) override
            {
                if (engine_ == nullptr || config_ == nullptr || enabled_ == nullptr)
                {
                    return;
                }

                if (!*enabled_)
                {
                    return;
                }

                auto* selection_engine = *engine_;
                if (selection_engine == nullptr)
                {
                    return;
                }

                const auto selection = selection_engine->ordered_selection();
                if (selection.empty())
                {
                    return;
                }

                auto& scene = context.render.view.scene;
                auto& registry = scene.registry();

                const auto camera_uniforms = resolve_camera_uniforms(registry);
                draw_commands_.clear();
                draw_commands_.reserve(selection.size());

                for (std::size_t index = 0; index < selection.size(); ++index)
                {
                    const auto entity = selection[index].hit.entity;
                    if (!registry.valid(entity)
                        || !registry.all_of<scene::components::WorldTransform, components::RenderGeometry>(entity))
                    {
                        continue;
                    }

                    const auto& world = registry.get<scene::components::WorldTransform>(entity).value;
                    const auto& geometry = registry.get<components::RenderGeometry>(entity);

                    if (const auto* mesh = geometry.mesh(); mesh != nullptr && !mesh->empty())
                    {
                        if (!assets::validate_handle(*mesh, "SelectionOutlinePass::mesh"))
                        {
                            continue;
                        }
                        context.render.resources.require_mesh(*mesh);
                    }
                    else if (const auto* graph = geometry.graph(); graph != nullptr && !graph->empty())
                    {
                        if (!assets::validate_handle(*graph, "SelectionOutlinePass::graph"))
                        {
                            continue;
                        }
                        context.render.resources.require_graph(*graph);
                    }
                    else if (const auto* point_cloud = geometry.point_cloud();
                             point_cloud != nullptr && !point_cloud->empty())
                    {
                        if (!assets::validate_handle(*point_cloud, "SelectionOutlinePass::point_cloud"))
                        {
                            continue;
                        }
                        context.render.resources.require_point_cloud(*point_cloud);
                    }

                    if (!geometry.material.empty())
                    {
                        if (!assets::validate_handle(geometry.material, "SelectionOutlinePass::material"))
                        {
                            continue;
                        }
                        context.render.materials.ensure_material_loaded(geometry.material, context.render.resources);
                    }

                    auto command = GeometryDrawCommand{geometry.geometry(), geometry.material, world};
                    command.view_matrix = camera_uniforms.view;
                    command.projection_matrix = camera_uniforms.projection;
                    command.camera_position = camera_uniforms.position;
                    command.transform = inflate_transform(world, camera_uniforms.position);
                    command.has_color_override = true;
                    command.color_override = resolve_color(index, selection.size());
                    command.alpha_override = resolve_alpha();

                    draw_commands_.push_back(command);
                }

                auto& encoder = context.command_encoder();
                for (const auto& command : draw_commands_)
                {
                    encoder.draw_geometry(command);
                }
            }

        private:
            [[nodiscard]] float compute_scale(float distance) const
            {
                if (config_ == nullptr)
                {
                    return 1.0F;
                }

                const auto& cfg = *config_;
                float thickness = cfg.style.thickness * 0.01F;

                const auto mode = force_screen_space_ ? ThicknessMode::ScreenSpaceUniform : cfg.thickness_mode;
                switch (mode)
                {
                case ThicknessMode::WorldSpaceUniform:
                    thickness = cfg.world_space_thickness;
                    break;
                case ThicknessMode::DepthWeighted:
                    thickness = cfg.depth_weight_factor * distance;
                    break;
                case ThicknessMode::HybridAdaptive:
                {
                    const float t = std::clamp(
                        (distance - cfg.near_distance) / std::max(cfg.far_distance - cfg.near_distance, 1.0F), 0.0F,
                        1.0F);
                    const float screen = cfg.style.thickness * 0.01F;
                    const float world = cfg.world_space_thickness + cfg.depth_weight_factor * distance;
                    thickness = screen + (world - screen) * t;
                    break;
                }
                case ThicknessMode::ScreenSpaceUniform:
                default:
                    break;
                }

                const float min_scale = cfg.min_screen_thickness * 0.001F;
                const float max_scale = cfg.max_screen_thickness * 0.1F;
                const float clamped = std::clamp(thickness, min_scale, max_scale);
                return clamp_positive(1.0F + clamped, 1.0F);
            }

            [[nodiscard]] engine::math::vec3 resolve_color(std::size_t index, std::size_t total) const
            {
                if (config_ == nullptr)
                {
                    return engine::math::vec3{1.0F, 0.5F, 0.0F};
                }

                const auto& cfg = *config_;

                const auto gradient_color = [&](float t)
                {
                    t = std::clamp(t, 0.0F, 1.0F);
                    return engine::math::lerp(cfg.style.color, cfg.style.gradient_end_color, t);
                };

                switch (cfg.multi_selection_mode)
                {
                case MultiSelectionMode::ColorCoded:
                case MultiSelectionMode::PriorityOrdered:
                case MultiSelectionMode::Grouped:
                {
                    if (total <= 1)
                    {
                        return gradient_color(0.0F);
                    }
                    const float t = static_cast<float>(index) / static_cast<float>(total - 1);
                    return gradient_color(t);
                }
                case MultiSelectionMode::Merged:
                case MultiSelectionMode::Distinct:
                default:
                    return cfg.style.color;
                }
            }

            [[nodiscard]] float resolve_alpha() const
            {
                if (config_ == nullptr)
                {
                    return 1.0F;
                }

                if (config_->occlusion_mode == OcclusionMode::Disabled)
                {
                    return config_->style.alpha;
                }
                return std::min(config_->style.alpha, config_->occluded_style.alpha);
            }

            [[nodiscard]] engine::math::Transform<float> inflate_transform(
                const engine::math::Transform<float>& transform,
                const engine::math::vec3& camera_position) const
            {
                const float distance = engine::math::length(transform.translation - camera_position);
                const float scale = compute_scale(distance);

                engine::math::Transform<float> inflated = transform;
                inflated.scale[0] *= scale;
                inflated.scale[1] *= scale;
                inflated.scale[2] *= scale;
                return inflated;
            }

            FrameGraphResourceHandle color_{};
            FrameGraphResourceHandle depth_{};
            SelectionEngine** engine_{nullptr};
            OutlineConfig* config_{nullptr};
            bool* enabled_{nullptr};
            bool force_screen_space_{false};
            std::vector<GeometryDrawCommand> draw_commands_{};
        };

        class JumpFloodOutlineStrategy final : public SelectionOutlineStrategy
        {
        public:
            [[nodiscard]] bool supports_mode(ThicknessMode) const override
            {
                return true;
            }

            [[nodiscard]] bool supports_quality(OutlineQuality quality) const override
            {
                return quality != OutlineQuality::Fast;
            }

            [[nodiscard]] const char* name() const override
            {
                return "JumpFlood";
            }

            void add_pass(FrameGraph& graph, const OutlineContext& context) override
            {
                if (!context.color.valid() || !context.depth.valid())
                {
                    return;
                }

                auto pass = std::make_unique<GeometryInflationOutlinePass>(context.color, context.depth,
                                                                           context.selection_engine, context.config,
                                                                           context.enabled, false);
                graph.add_pass(std::move(pass));
            }
        };

        class EdgeDetectionOutlineStrategy final : public SelectionOutlineStrategy
        {
        public:
            [[nodiscard]] bool supports_mode(ThicknessMode mode) const override
            {
                return mode == ThicknessMode::ScreenSpaceUniform || mode == ThicknessMode::HybridAdaptive
                    || mode == ThicknessMode::DepthWeighted;
            }

            [[nodiscard]] bool supports_quality(OutlineQuality quality) const override
            {
                return quality == OutlineQuality::Fast || quality == OutlineQuality::Balanced;
            }

            [[nodiscard]] const char* name() const override
            {
                return "EdgeDetection";
            }

            void add_pass(FrameGraph& graph, const OutlineContext& context) override
            {
                if (!context.color.valid() || !context.depth.valid())
                {
                    return;
                }

                auto pass = std::make_unique<GeometryInflationOutlinePass>(context.color, context.depth,
                                                                           context.selection_engine, context.config,
                                                                           context.enabled, true);
                graph.add_pass(std::move(pass));
            }
        };

        using StrategyFactory = std::unique_ptr<SelectionOutlineStrategy> (*)();

        std::unique_ptr<SelectionOutlineStrategy> make_jump_flood_strategy()
        {
            return std::make_unique<JumpFloodOutlineStrategy>();
        }

        std::unique_ptr<SelectionOutlineStrategy> make_edge_detection_strategy()
        {
            return std::make_unique<EdgeDetectionOutlineStrategy>();
        }

        [[nodiscard]] std::vector<std::pair<std::string_view, StrategyFactory>>& registry()
        {
            static std::vector<std::pair<std::string_view, StrategyFactory>> instance{
                {"JumpFlood", &make_jump_flood_strategy},
                {"EdgeDetection", &make_edge_detection_strategy},
            };
            return instance;
        }

        [[nodiscard]] std::unique_ptr<SelectionOutlineStrategy> try_make(std::string_view name)
        {
            for (const auto& entry : registry())
            {
                if (entry.first == name)
                {
                    return entry.second();
                }
            }
            return nullptr;
        }
    } // namespace

    std::unique_ptr<SelectionOutlineStrategy> SelectionOutlineStrategyFactory::create(OutlineQuality quality,
                                                                                      ThicknessMode mode)
    {
        const std::array<std::string_view, 2> preferred = {
            quality == OutlineQuality::Fast ? std::string_view{"EdgeDetection"} : std::string_view{"JumpFlood"},
            quality == OutlineQuality::Fast ? std::string_view{"JumpFlood"} : std::string_view{"EdgeDetection"},
        };

        for (const auto& name : preferred)
        {
            if (auto candidate = try_make(name))
            {
                if (candidate->supports_quality(quality) && candidate->supports_mode(mode))
                {
                    return candidate;
                }
            }
        }

        for (const auto& entry : registry())
        {
            if (auto candidate = entry.second())
            {
                if (candidate->supports_quality(quality) && candidate->supports_mode(mode))
                {
                    return candidate;
                }
            }
        }

        return nullptr;
    }

    std::unique_ptr<SelectionOutlineStrategy> SelectionOutlineStrategyFactory::create_by_name(std::string_view name)
    {
        return try_make(name);
    }

    std::vector<std::string> SelectionOutlineStrategyFactory::available_strategies()
    {
        std::vector<std::string> names;
        names.reserve(registry().size());
        for (const auto& entry : registry())
        {
            names.emplace_back(entry.first);
        }
        return names;
    }
} // namespace engine::rendering
