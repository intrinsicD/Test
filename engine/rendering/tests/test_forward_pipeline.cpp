#include <algorithm>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "engine/rendering/components.hpp"
#include "engine/rendering/forward_pipeline.hpp"
#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"

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
    engine::scene::Scene scene;
    const auto mesh_entity = scene.create_entity();
    auto& mesh_world =
        scene.registry().emplace<engine::scene::components::WorldTransform>(mesh_entity.id());
    mesh_world.value.translation = engine::math::Vector<float, 3>{1.0F, 2.0F, 3.0F};
    scene.registry().emplace<engine::rendering::components::RenderGeometry>(
        mesh_entity.id(),
        engine::rendering::components::RenderGeometry::from_mesh(
            engine::assets::MeshHandle{std::string{"mesh"}},
            engine::assets::MaterialHandle{std::string{"mesh_material"}}));

    const auto graph_entity = scene.create_entity();
    auto& graph_world =
        scene.registry().emplace<engine::scene::components::WorldTransform>(graph_entity.id());
    graph_world.value.translation = engine::math::Vector<float, 3>{-1.0F, 0.5F, 4.0F};
    scene.registry().emplace<engine::rendering::components::RenderGeometry>(
        graph_entity.id(),
        engine::rendering::components::RenderGeometry::from_graph(
            engine::assets::GraphHandle{std::string{"graph"}},
            engine::assets::MaterialHandle{std::string{"graph_material"}}));

    const auto cloud_entity = scene.create_entity();
    auto& cloud_world =
        scene.registry().emplace<engine::scene::components::WorldTransform>(cloud_entity.id());
    cloud_world.value.translation = engine::math::Vector<float, 3>{0.0F, -3.0F, -1.0F};
    scene.registry().emplace<engine::rendering::components::RenderGeometry>(
        cloud_entity.id(),
        engine::rendering::components::RenderGeometry::from_point_cloud(
            engine::assets::PointCloudHandle{std::string{"cloud"}},
            engine::assets::MaterialHandle{std::string{"cloud_material"}}));

    engine::rendering::MaterialSystem materials;
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        engine::assets::MaterialHandle{std::string{"mesh_material"}},
        engine::assets::ShaderHandle{std::string{"mesh_shader"}}
    });
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        engine::assets::MaterialHandle{std::string{"graph_material"}},
        engine::assets::ShaderHandle{std::string{"graph_shader"}}
    });
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        engine::assets::MaterialHandle{std::string{"cloud_material"}},
        engine::assets::ShaderHandle{std::string{"cloud_shader"}}
    });

    engine::rendering::FrameGraph graph;
    engine::rendering::ForwardPipeline pipeline;
    RecordingProvider provider;

    pipeline.render(scene, provider, materials, graph);

    ASSERT_EQ(graph.execution_order().size(), 1);  // NOLINT
    const auto& events = graph.resource_events();
    ASSERT_EQ(events.size(), 4);  // NOLINT

    EXPECT_EQ(events[0].resource_name, "ForwardColor");
    EXPECT_EQ(events[0].type, engine::rendering::ResourceEvent::Type::Acquire);

    EXPECT_EQ(events[1].resource_name, "ForwardDepth");
    EXPECT_EQ(events[1].type, engine::rendering::ResourceEvent::Type::Acquire);

    EXPECT_EQ(events[2].resource_name, "ForwardColor");
    EXPECT_EQ(events[2].type, engine::rendering::ResourceEvent::Type::Release);

    EXPECT_EQ(events[3].resource_name, "ForwardDepth");
    EXPECT_EQ(events[3].type, engine::rendering::ResourceEvent::Type::Release);

    ASSERT_EQ(provider.meshes.size(), 1);  // NOLINT
    EXPECT_EQ(provider.meshes.front().id(), std::string{"mesh"});

    ASSERT_EQ(provider.graphs.size(), 1);  // NOLINT
    EXPECT_EQ(provider.graphs.front().id(), std::string{"graph"});

    ASSERT_EQ(provider.point_clouds.size(), 1);  // NOLINT
    EXPECT_EQ(provider.point_clouds.front().id(), std::string{"cloud"});

    ASSERT_EQ(provider.materials.size(), 3);  // NOLINT
    EXPECT_TRUE(std::find(provider.materials.begin(), provider.materials.end(),
                          engine::assets::MaterialHandle{std::string{"mesh_material"}})
                != provider.materials.end());
    EXPECT_TRUE(std::find(provider.materials.begin(), provider.materials.end(),
                          engine::assets::MaterialHandle{std::string{"graph_material"}})
                != provider.materials.end());
    EXPECT_TRUE(std::find(provider.materials.begin(), provider.materials.end(),
                          engine::assets::MaterialHandle{std::string{"cloud_material"}})
                != provider.materials.end());

    ASSERT_EQ(provider.shaders.size(), 3);  // NOLINT
    EXPECT_TRUE(std::find(provider.shaders.begin(), provider.shaders.end(),
                          engine::assets::ShaderHandle{std::string{"mesh_shader"}})
                != provider.shaders.end());
    EXPECT_TRUE(std::find(provider.shaders.begin(), provider.shaders.end(),
                          engine::assets::ShaderHandle{std::string{"graph_shader"}})
                != provider.shaders.end());
    EXPECT_TRUE(std::find(provider.shaders.begin(), provider.shaders.end(),
                          engine::assets::ShaderHandle{std::string{"cloud_shader"}})
                != provider.shaders.end());
}
