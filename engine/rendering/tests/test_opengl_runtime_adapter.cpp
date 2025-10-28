#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "engine/assets/handles.hpp"
#include "engine/geometry/api.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/backend/opengl/runtime_adapter.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/forward_pipeline.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

namespace
{
    engine::geometry::SurfaceMesh make_test_triangle()
    {
        engine::geometry::SurfaceMesh mesh{};
        mesh.positions = {
            {0.0F, 0.0F, 0.0F},
            {1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
        };
        mesh.indices = {0, 1, 2};
        return mesh;
    }
}

TEST(OpenGLRuntimeSubmission, ExecutesForwardPipelineDraw)
{
    using namespace engine::rendering;
    using namespace engine::rendering::backend::opengl;

    auto resolver = [](const engine::assets::MeshHandle&) -> std::optional<engine::geometry::SurfaceMesh>
    {
        return make_test_triangle();
    };

    OpenGLRuntimeSubmission submission(resolver);
    MaterialSystem materials;
    FrameGraph graph;
    ForwardPipeline pipeline;

    auto context = submission.make_context(materials, graph, &pipeline);

    engine::scene::Scene scene;
    auto entity = scene.registry().create();
    auto& transform = scene.registry().emplace<engine::scene::components::WorldTransform>(entity);
    transform.value = engine::math::Transform<float>::Identity();

    engine::assets::MeshHandle mesh_handle{std::string{"runtime.mesh"}};
    engine::assets::MeshHandle::pool_handle_type mesh_raw{};
    mesh_raw.index = 1U;
    mesh_raw.generation = 1U;
    mesh_handle.bind(mesh_raw);

    engine::assets::MaterialHandle material_handle{std::string{"runtime.material"}};
    engine::assets::MaterialHandle::pool_handle_type material_raw{};
    material_raw.index = 2U;
    material_raw.generation = 1U;
    material_handle.bind(material_raw);

    engine::assets::ShaderHandle shader_handle{std::string{"runtime.shader"}};
    engine::assets::ShaderHandle::pool_handle_type shader_raw{};
    shader_raw.index = 3U;
    shader_raw.generation = 1U;
    shader_handle.bind(shader_raw);

    materials.register_material(MaterialSystem::MaterialRecord{material_handle, shader_handle});

    scene.registry().emplace<components::RenderGeometry>(
        entity,
        components::RenderGeometry::from_mesh(mesh_handle, material_handle));

    pipeline.render(scene, context);

    ASSERT_EQ(submission.scheduler.submissions().size(), 1U); // NOLINT
    const auto& recorded_submission = submission.scheduler.submissions().front();
    EXPECT_EQ(recorded_submission.pass_name, "ForwardGeometry");
    ASSERT_EQ(recorded_submission.draw_commands.size(), 1U); // NOLINT
    EXPECT_EQ(submission.command_stream.draw_call_count(), 1U);
    EXPECT_EQ(submission.render_resources.loaded_mesh_count(), 1U);
}
