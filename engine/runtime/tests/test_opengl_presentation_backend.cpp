#include <gtest/gtest.h>

#include "engine/assets/handles.hpp"
#include "engine/assets/validation.hpp"
#include "engine/geometry/api.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"
#include "engine/rendering/components.hpp"
#include "engine/runtime/api.hpp"
#include "engine/runtime/render_submission.hpp"

namespace
{
    struct ScopedHandleValidators
    {
        ScopedHandleValidators()
        {
            auto& registry = engine::assets::HandleValidatorRegistry::instance();
            mesh = registry.register_mesh_validator([](const engine::assets::MeshHandle&) { return true; });
            material = registry.register_material_validator([](const engine::assets::MaterialHandle&) { return true; });
            shader = registry.register_shader_validator([](const engine::assets::ShaderHandle&) { return true; });
        }

        [[maybe_unused]] std::shared_ptr<void> mesh{};
        [[maybe_unused]] std::shared_ptr<void> material{};
        [[maybe_unused]] std::shared_ptr<void> shader{};
    };
} // namespace

#if ENGINE_ENABLE_RENDERING

TEST(RuntimePresentationBackend, OpenGLBackendExecutesFrameGraph)
{
    ScopedHandleValidators handle_validators;

    engine::runtime::RuntimeHostDependencies deps{};
    engine::assets::MeshHandle mesh_handle{std::string{"runtime.mesh"}};
    engine::assets::MeshHandle::pool_handle_type mesh_raw{};
    mesh_raw.index = 0U;
    mesh_raw.generation = 1U;
    mesh_handle.bind(mesh_raw);

    engine::assets::MaterialHandle material_handle{std::string{"runtime.material"}};
    engine::assets::MaterialHandle::pool_handle_type material_raw{};
    material_raw.index = 0U;
    material_raw.generation = 1U;
    material_handle.bind(material_raw);

    deps.render_geometry = engine::rendering::components::RenderGeometry::from_mesh(
        mesh_handle,
        material_handle);
    deps.renderable_name = "runtime.renderable";

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    host.tick(0.016);

    engine::rendering::backend::opengl::OpenGLPresentationBackend backend(
        [&host, mesh_handle](const engine::assets::MeshHandle& handle)
        -> std::optional<engine::geometry::SurfaceMesh>
        {
            if (handle == mesh_handle)
            {
                return host.current_mesh();
            }
            return std::nullopt;
        });

    engine::assets::ShaderHandle shader_handle{std::string{"runtime.shader"}};
    engine::assets::ShaderHandle::pool_handle_type shader_raw{};
    shader_raw.index = 0U;
    shader_raw.generation = 1U;
    shader_handle.bind(shader_raw);
    backend.material_system().register_material(
        engine::rendering::MaterialSystem::MaterialRecord{material_handle, shader_handle});

    engine::rendering::RuntimePresentationContext presentation_context{host, 0.016};
    presentation_context.submit_render_graph = &engine::runtime::submit_render_graph;
    backend.present(presentation_context);

    const auto& device_resources = backend.submission().device_resources;
    const auto& acquired = device_resources.acquired();
    ASSERT_EQ(acquired.size(), 2U); // NOLINT
    EXPECT_EQ(acquired[0].info.name, "ForwardColor");
    EXPECT_EQ(acquired[1].info.name, "ForwardDepth");

    const auto& scheduler_submissions = backend.submission().scheduler.submissions();
    ASSERT_EQ(scheduler_submissions.size(), 1U); // NOLINT
    EXPECT_EQ(scheduler_submissions.front().pass_name, "ForwardGeometry");

    const auto& diagnostics = host.diagnostics();
    EXPECT_FALSE(diagnostics.frame_graph_serialization.empty());
    ASSERT_EQ(diagnostics.frame_graph_events.size(), 4U); // NOLINT
    EXPECT_EQ(diagnostics.frame_graph_events[0].resource_name, "ForwardColor");
    EXPECT_EQ(diagnostics.frame_graph_events[1].resource_name, "ForwardDepth");
    ASSERT_EQ(diagnostics.command_encoder_stats.size(), 1U); // NOLINT
    EXPECT_EQ(diagnostics.command_encoder_stats.front().draw_count, 1U);
    EXPECT_EQ(diagnostics.command_encoder_stats.front().dispatch_count, 0U);
}

TEST(RuntimePresentationBackend, OpenGLBackendConfiguresRetentionFrames)
{
    ScopedHandleValidators handle_validators;

    engine::rendering::backend::opengl::OpenGLPresentationBackend backend(
        [](const engine::assets::MeshHandle&) -> std::optional<engine::geometry::SurfaceMesh> {
            return std::nullopt;
        },
        nullptr,
        3);

    EXPECT_EQ(backend.resource_retention_frames(), 3U);

    backend.set_resource_retention_frames(5);
    EXPECT_EQ(backend.resource_retention_frames(), 5U);
}

#endif // ENGINE_ENABLE_RENDERING

