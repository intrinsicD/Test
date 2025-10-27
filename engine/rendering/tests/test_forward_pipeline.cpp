#include <algorithm>
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "engine/rendering/components.hpp"
#include "engine/rendering/forward_pipeline.hpp"
#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/assets/validation.hpp"
#include "command_encoder_test_utils.hpp"
#include "scheduler_test_utils.hpp"

namespace
{
    class RecordingProvider final : public engine::rendering::RenderResourceProvider
    {
    public:
        void require_mesh(const engine::assets::MeshHandle& handle) override
        {
            meshes.push_back(handle);
        }

        void require_graph(const engine::assets::GraphHandle& handle) override
        {
            graphs.push_back(handle);
        }

        void require_point_cloud(const engine::assets::PointCloudHandle& handle) override
        {
            point_clouds.push_back(handle);
        }

        void require_material(const engine::assets::MaterialHandle& handle) override
        {
            materials.push_back(handle);
        }

        void require_shader(const engine::assets::ShaderHandle& handle) override
        {
            shaders.push_back(handle);
        }

        std::vector<engine::assets::MeshHandle> meshes;
        std::vector<engine::assets::GraphHandle> graphs;
        std::vector<engine::assets::PointCloudHandle> point_clouds;
        std::vector<engine::assets::MaterialHandle> materials;
        std::vector<engine::assets::ShaderHandle> shaders;
    };
}

TEST(ForwardPipeline, RequestsResourcesForVisibleRenderables)
{
    auto& validators = engine::assets::HandleValidatorRegistry::instance();
    [[maybe_unused]] auto mesh_validator = validators.register_mesh_validator([](const engine::assets::MeshHandle&)
    {
        return true;
    });
    [[maybe_unused]] auto graph_validator = validators.register_graph_validator([](const engine::assets::GraphHandle&)
    {
        return true;
    });
    [[maybe_unused]] auto cloud_validator = validators.register_point_cloud_validator(
        [](const engine::assets::PointCloudHandle&) { return true; });
    [[maybe_unused]] auto material_validator = validators.register_material_validator(
        [](const engine::assets::MaterialHandle&) { return true; });
    [[maybe_unused]] auto shader_validator = validators.register_shader_validator(
        [](const engine::assets::ShaderHandle&) { return true; });

    engine::assets::MeshHandle mesh_handle{std::string{"mesh"}};
    engine::assets::MeshHandle::pool_handle_type mesh_raw{};
    mesh_raw.index = 0U;
    mesh_raw.generation = 1U;
    mesh_handle.bind(mesh_raw);
    engine::assets::MaterialHandle mesh_material{std::string{"mesh_material"}};
    engine::assets::MaterialHandle::pool_handle_type mesh_material_raw{};
    mesh_material_raw.index = 0U;
    mesh_material_raw.generation = 1U;
    mesh_material.bind(mesh_material_raw);

    engine::scene::Scene scene;
    const auto mesh_entity = scene.create_entity();
    auto& mesh_world =
        scene.registry().emplace<engine::scene::components::WorldTransform>(mesh_entity.id());
    mesh_world.value.translation = engine::math::Vector < float, 3 >
    {
        1.0F, 2.0F, 3.0F
    };
    scene.registry().emplace<engine::rendering::components::RenderGeometry>(
        mesh_entity.id(),
        engine::rendering::components::RenderGeometry::from_mesh(mesh_handle, mesh_material));

    engine::assets::GraphHandle graph_handle{std::string{"graph"}};
    engine::assets::GraphHandle::pool_handle_type graph_raw{};
    graph_raw.index = 1U;
    graph_raw.generation = 1U;
    graph_handle.bind(graph_raw);
    engine::assets::MaterialHandle graph_material{std::string{"graph_material"}};
    engine::assets::MaterialHandle::pool_handle_type graph_material_raw{};
    graph_material_raw.index = 1U;
    graph_material_raw.generation = 1U;
    graph_material.bind(graph_material_raw);

    const auto graph_entity = scene.create_entity();
    auto& graph_world =
        scene.registry().emplace<engine::scene::components::WorldTransform>(graph_entity.id());
    graph_world.value.translation = engine::math::Vector < float, 3 >
    {
        -1.0F, 0.5F, 4.0F
    };
    scene.registry().emplace<engine::rendering::components::RenderGeometry>(
        graph_entity.id(),
        engine::rendering::components::RenderGeometry::from_graph(graph_handle, graph_material));

    engine::assets::PointCloudHandle cloud_handle{std::string{"cloud"}};
    engine::assets::PointCloudHandle::pool_handle_type cloud_raw{};
    cloud_raw.index = 2U;
    cloud_raw.generation = 1U;
    cloud_handle.bind(cloud_raw);
    engine::assets::MaterialHandle cloud_material{std::string{"cloud_material"}};
    engine::assets::MaterialHandle::pool_handle_type cloud_material_raw{};
    cloud_material_raw.index = 2U;
    cloud_material_raw.generation = 1U;
    cloud_material.bind(cloud_material_raw);

    const auto cloud_entity = scene.create_entity();
    auto& cloud_world =
        scene.registry().emplace<engine::scene::components::WorldTransform>(cloud_entity.id());
    cloud_world.value.translation = engine::math::Vector < float, 3 >
    {
        0.0F, -3.0F, -1.0F
    };
    scene.registry().emplace<engine::rendering::components::RenderGeometry>(
        cloud_entity.id(),
        engine::rendering::components::RenderGeometry::from_point_cloud(cloud_handle, cloud_material));

    engine::rendering::MaterialSystem materials;
    engine::assets::ShaderHandle mesh_shader{std::string{"mesh_shader"}};
    engine::assets::ShaderHandle::pool_handle_type mesh_shader_raw{};
    mesh_shader_raw.index = 0U;
    mesh_shader_raw.generation = 1U;
    mesh_shader.bind(mesh_shader_raw);
    engine::assets::ShaderHandle graph_shader{std::string{"graph_shader"}};
    engine::assets::ShaderHandle::pool_handle_type graph_shader_raw{};
    graph_shader_raw.index = 1U;
    graph_shader_raw.generation = 1U;
    graph_shader.bind(graph_shader_raw);
    engine::assets::ShaderHandle cloud_shader{std::string{"cloud_shader"}};
    engine::assets::ShaderHandle::pool_handle_type cloud_shader_raw{};
    cloud_shader_raw.index = 2U;
    cloud_shader_raw.generation = 1U;
    cloud_shader.bind(cloud_shader_raw);

    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{mesh_material, mesh_shader});
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{graph_material, graph_shader});
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{cloud_material, cloud_shader});

    engine::rendering::FrameGraph graph;
    engine::rendering::ForwardPipeline pipeline;
    RecordingProvider provider;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider;
    engine::rendering::tests::RecordingScheduler scheduler;
    engine::rendering::tests::RecordingCommandEncoderProvider command_encoders;
    engine::rendering::RuntimeSubmissionContext submission{
        provider,
        materials,
        device_provider,
        scheduler,
        command_encoders,
        graph,
        nullptr,
    };

    pipeline.render(scene, submission);

    ASSERT_EQ(scheduler.submissions.size(), 1); // NOLINT
    EXPECT_EQ(scheduler.submissions.front().pass_name, "ForwardGeometry");
    EXPECT_EQ(scheduler.submissions.front().queue, engine::rendering::QueueType::Graphics);

    ASSERT_EQ(scheduler.pass_metadata.size(), 1); // NOLINT
    const auto& pass_record = scheduler.pass_metadata.front();
    EXPECT_EQ(pass_record.pass_name, "ForwardGeometry");
    EXPECT_EQ(pass_record.preferred_queue, engine::rendering::QueueType::Graphics);
    EXPECT_EQ(pass_record.phase, engine::rendering::PassPhase::Geometry);
    EXPECT_EQ(pass_record.validation, engine::rendering::ValidationSeverity::Error);

    ASSERT_EQ(command_encoders.begin_records.size(), 1); // NOLINT
    EXPECT_EQ(command_encoders.begin_records.front().pass_name, "ForwardGeometry");
    EXPECT_EQ(command_encoders.begin_records.front().queue, engine::rendering::QueueType::Graphics);
    ASSERT_EQ(command_encoders.completed_encoders.size(), 1); // NOLINT
    const auto& recorded_encoder = *command_encoders.completed_encoders.front();
    ASSERT_EQ(recorded_encoder.draws.size(), 3); // NOLINT
    bool saw_mesh = false;
    bool saw_graph = false;
    bool saw_point_cloud = false;
    for (const auto& draw : recorded_encoder.draws)
    {
        if (std::holds_alternative<engine::assets::MeshHandle>(draw.geometry))
        {
            saw_mesh = true;
            EXPECT_EQ(std::get<engine::assets::MeshHandle>(draw.geometry).id(), std::string{"mesh"});
            EXPECT_EQ(draw.material.id(), std::string{"mesh_material"});
            EXPECT_EQ(draw.transform.translation,
                      (engine::math::Vector < float, 3 >
            {
                1.0F, 2.0F, 3.0F
            }
            )
            )
            ;
        }
        else if (std::holds_alternative<engine::assets::GraphHandle>(draw.geometry))
        {
            saw_graph = true;
            EXPECT_EQ(std::get<engine::assets::GraphHandle>(draw.geometry).id(), std::string{"graph"});
            EXPECT_EQ(draw.material.id(), std::string{"graph_material"});
            EXPECT_EQ(draw.transform.translation,
                      (engine::math::Vector < float, 3 >
            {
                -1.0F, 0.5F, 4.0F
            }
            )
            )
            ;
        }
        else if (std::holds_alternative<engine::assets::PointCloudHandle>(draw.geometry))
        {
            saw_point_cloud = true;
            EXPECT_EQ(std::get<engine::assets::PointCloudHandle>(draw.geometry).id(), std::string{"cloud"});
            EXPECT_EQ(draw.material.id(), std::string{"cloud_material"});
            EXPECT_EQ(draw.transform.translation,
                      (engine::math::Vector < float, 3 >
            {
                0.0F, -3.0F, -1.0F
            }
            )
            )
            ;
        }
    }
    EXPECT_TRUE(saw_mesh);
    EXPECT_TRUE(saw_graph);
    EXPECT_TRUE(saw_point_cloud);

    ASSERT_EQ(graph.execution_order().size(), 1); // NOLINT
    const auto& events = graph.resource_events();
    ASSERT_EQ(events.size(), 4); // NOLINT

    EXPECT_EQ(events[0].resource_name, "ForwardColor");
    EXPECT_EQ(events[0].type, engine::rendering::ResourceEvent::Type::Acquire);

    EXPECT_EQ(events[1].resource_name, "ForwardDepth");
    EXPECT_EQ(events[1].type, engine::rendering::ResourceEvent::Type::Acquire);

    EXPECT_EQ(events[2].resource_name, "ForwardColor");
    EXPECT_EQ(events[2].type, engine::rendering::ResourceEvent::Type::Release);

    EXPECT_EQ(events[3].resource_name, "ForwardDepth");
    EXPECT_EQ(events[3].type, engine::rendering::ResourceEvent::Type::Release);

    ASSERT_EQ(provider.meshes.size(), 1); // NOLINT
    EXPECT_EQ(provider.meshes.front().id(), std::string{"mesh"});

    ASSERT_EQ(provider.graphs.size(), 1); // NOLINT
    EXPECT_EQ(provider.graphs.front().id(), std::string{"graph"});

    ASSERT_EQ(provider.point_clouds.size(), 1); // NOLINT
    EXPECT_EQ(provider.point_clouds.front().id(), std::string{"cloud"});

    ASSERT_EQ(provider.materials.size(), 3); // NOLINT
    EXPECT_TRUE(std::find(provider.materials.begin(), provider.materials.end(),
                          engine::assets::MaterialHandle{std::string{"mesh_material"}})
        != provider.materials.end());
    EXPECT_TRUE(std::find(provider.materials.begin(), provider.materials.end(),
                          engine::assets::MaterialHandle{std::string{"graph_material"}})
        != provider.materials.end());
    EXPECT_TRUE(std::find(provider.materials.begin(), provider.materials.end(),
                          engine::assets::MaterialHandle{std::string{"cloud_material"}})
        != provider.materials.end());

    ASSERT_EQ(provider.shaders.size(), 3); // NOLINT
    EXPECT_TRUE(std::find(provider.shaders.begin(), provider.shaders.end(),
                          engine::assets::ShaderHandle{std::string{"mesh_shader"}})
        != provider.shaders.end());
    EXPECT_TRUE(std::find(provider.shaders.begin(), provider.shaders.end(),
                          engine::assets::ShaderHandle{std::string{"graph_shader"}})
        != provider.shaders.end());
    EXPECT_TRUE(std::find(provider.shaders.begin(), provider.shaders.end(),
                          engine::assets::ShaderHandle{std::string{"cloud_shader"}})
        != provider.shaders.end());

    EXPECT_EQ(device_provider.frames_begun(), 1U);
    EXPECT_EQ(device_provider.frames_completed(), 1U);
    ASSERT_EQ(device_provider.acquired().size(), 2); // NOLINT
    ASSERT_EQ(device_provider.released().size(), 2); // NOLINT
}